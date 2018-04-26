#include "resource_table_pru.h"
#include <pru_cfg.h>
#include <stdint.h>
#include <string.h>

#if PRU_ID == 1
#define P8_28 10
#define P8_42 5
#define P8_44 3
#define P8_46 1
#define CLK P8_42
#define SDO P8_44
#define LATCH P8_46
//#define ENABLE P8_28
#elif PRU_ID == 0
#define P8_11 15
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

inline void write_byte_to_spi(uint8_t byte)
{
    uint8_t bit = 0, i = 0;
    // Fill the 8 bit shift register
    for (i = 0; i < 8; i++) {
        __R30 &= ~(1 << CLK); // clear
        bit = (byte >> i) & 0x01;
        __R30 = (bit << SDO);
        __delay_cycles(1);
        __R30 |= 1 << CLK; //set
        __delay_cycles(1);
    }
    // Latches the register to the 8 parallel outputs
    __R30 |= 1 << LATCH; // set
}

inline void write_short_to_spi(uint16_t s)
{
    uint8_t bits = 0, i = 0;
    // Fill the 8 bit shift register
    for (i = 0; i < 8; i++) {
        __R30 &= ~(1 << CLK); // clear
        bits = (s >> i) & 0x0101;
        __R30 = (bits << SDO);
        __delay_cycles(1);
        __R30 |= 1 << CLK; //set
        __delay_cycles(1);
    }
    // Latches the register to the 8 parallel outputs
    __R30 |= 1 << LATCH; // set
}

void main(void)
{
    uint8_t* shared_memory = (uint8_t*)0x10000;
    volatile uint8_t* flag_pru = &shared_memory[PRU_ID];

    // Wait for a frame from the ARM
    *flag_pru = 1;
    while (*flag_pru == 1) {}

    const uint32_t frame_buffer_size = shared_memory[2] * 3 * 16; // 16 strips
    const uint8_t* frame[2] = {
        ((uint8_t*)shared_memory) + 4,
        ((uint8_t*)shared_memory) + 4 + frame_buffer_size
    };
    const uint32_t start = PRU_ID ? 0 : 1;

    uint32_t j = 0, k = 0;

    while (1) {
        k = !k;

        for (j = start; j < frame_buffer_size; j += 2) {
            write_byte_to_spi(0xFF); //230ns
            __delay_cycles(4); // 20ns

            write_byte_to_spi(frame[k][j]); //230ns
            __delay_cycles(24); // 120ns

            write_byte_to_spi(0x00); //230ns
            __delay_cycles(4); // 20ns
        }

        // The 50 us reset code needed by led strips.
        __R30 = 0;
        __delay_cycles(12000);

        // Wait for the ARM to be ready for the next frame
        *flag_pru = 1;
        while (*flag_pru == 1) {}
    }
}
