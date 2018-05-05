/*
 * Copyright (C) 2018 Simon Guigui
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "resource_table_pru.h"
#include <pru_cfg.h>
#include <stdint.h>
#include <string.h>

#if PRU_ID == 1
#define P8_28 10 // Wired to SDI of second shift register
#define P8_42 5
#define P8_43 2
#define P8_46 1
#define CLK P8_42
#define SDO P8_43
#define LATCH P8_46
//#define ENABLE P8_28
#elif PRU_ID == 0
#define P8_11 15 // Wired to SDI of second shift register
#define P9_25 7
#define P9_27 5
#define P9_29 1
#define P9_31 0
#define CLK P9_27
#define SDO P9_25
#define LATCH P9_31
//#define ENABLE P9_29
#endif

volatile register uint32_t __R30;
volatile register uint32_t __R31;

uint8_t* shared_memory = reinterpret_cast<uint8_t*>(0x10000);

template <class U>
inline void write_to_spi(const U out)
{
    U bit = 0, i = 0;
    // Fill shift register(s)
    for (i = 0; i < 8; i++) {
        __R30 &= ~(1 << CLK); // clear
        bit = (out >> i) & static_cast<U>(0x0101);
        __R30 = (bit << SDO);
        __delay_cycles(1);
        __R30 |= 1 << CLK; // set
        __delay_cycles(1);
    }

    // Latches the register to the 8 parallel outputs
    __R30 |= 1 << LATCH; // set
}

template <class T, class U, const int strip_count>
inline void write_frame(const int step)
{
    const int frame_buffer_size = shared_memory[2] * 3 * strip_count;
    const U* frame_buffer = reinterpret_cast<const U*>(shared_memory + 4);
    volatile T* flag_pru = reinterpret_cast<T*>(shared_memory + PRU_ID);

    for (int i = PRU_ID; i < frame_buffer_size / sizeof(U); i += step) {
        write_to_spi(static_cast<U>(0xFFFF)); //230ns
        __delay_cycles(4); // 20ns

        write_to_spi(frame_buffer[i]); //230ns
        __delay_cycles(24); // 120ns

        write_to_spi(static_cast<U>(0x0000)); //230ns
        __delay_cycles(4); // 20ns
    }

    // Wait for the ARM to be ready for the next frame
    *flag_pru = static_cast<T>(0x0101);
    while (*flag_pru);
}

void main(void)
{

    while (1) {
        uint8_t strip_count = shared_memory[3];

        if (strip_count == 8 && PRU_ID == 0) {
            // 8 strips
            write_frame<uint16_t, uint8_t, 8>(1);
        } else if (strip_count == 16 && PRU_ID == 0) {
            // 16 strips
            write_frame<uint16_t, uint16_t, 16>(1);
        } else if (strip_count == 32) {
            // 32 strips
            write_frame<uint8_t, uint16_t, 32>(2);
        } else {
            __halt();
        }

        // The 50 us reset code needed by led strips.
        __R30 = 0;
        __delay_cycles(10000);
    }
}
