#include <stdint.h>
#include <pru_cfg.h>
#include <string.h>
#include "resource_table_pru.h"

volatile register uint32_t __R30;
volatile register uint32_t __R31;

const int LEDS_PER_STRIP = 120;
const int STRIP_COUNT = 16;
const int FRAME_BUFFER_SIZE = LEDS_PER_STRIP*3*STRIP_COUNT;

#if PRU_NUM == 1
const uint32_t PRU_ID = 1;
//const uint32_t P8_28 = 10;
const uint32_t P8_42 = 5;
const uint32_t P8_44 = 3;
const uint32_t P8_46 = 1;
const uint32_t CLK = P8_42;
const uint32_t SDO = P8_44;
const uint32_t LATCH = P8_46;
//const uint32_t ENABLE = P8_28;
const uint32_t START = 0;
#elif PRU_NUM == 0
const uint32_t PRU_ID = 0;
//const uint32_t P9_25 = 7;
const uint32_t P9_27 = 5;
const uint32_t P9_29 = 1;
const uint32_t P9_31 = 0;
const uint32_t CLK = P9_27;
const uint32_t SDO = P9_29;
const uint32_t LATCH = P9_31;
//const uint32_t ENABLE = P9_25;
const uint32_t START = FRAME_BUFFER_SIZE/2;
#endif

inline void write_byte_to_spi(uint8_t byte) {
    uint8_t bit = 0, i = 0;
    // Fill the 8 bit shift register
    for (i=0;i<8;i++) {
        __R30 &= ~(1<<CLK); // clear
        bit = ( byte >> i ) & 0x01;
        __R30 = (bit<<SDO);
        __delay_cycles(1);
        __R30 |= 1<<CLK; //set
        __delay_cycles(1);
    }
    // Latches the register to the 8 parallel outputs
    __R30 |= 1<<LATCH; // set
}

void main(void)
{

    /* Clear SYSCFG[STANDBY_INIT] to enable OCP master port */
    //CT_CFG.SYSCFG_bit.STANDBY_INIT = 0;

    volatile const uint32_t ONES = 0xFF;
    volatile const  uint32_t ZEROS = 0x00;

    uint8_t *shared_memory = (uint8_t*) 0x10000;
    volatile uint8_t *flag_pru  = &shared_memory[PRU_ID];

    const uint8_t *frame[2] = {
        &shared_memory[2],
        &shared_memory[2+FRAME_BUFFER_SIZE]
    };

    uint32_t j = 0, k = 0;

    // Wait for a frame from the CPU
    *flag_pru = 1;
    while(*flag_pru == 1) {}

    while (1) {
        k = !k;

        for (j=START;j<START+FRAME_BUFFER_SIZE/2;j++) {
            write_byte_to_spi(ONES); //230ns
            __delay_cycles(4); // 20ns

            write_byte_to_spi(frame[k][j]); //230ns
            __delay_cycles(24); // 120ns

            write_byte_to_spi(ZEROS); //230ns
            __delay_cycles(4); // 20ns
        }

        // The 50 us reset code needed by led strips.
        __R30 = 0;
        __delay_cycles(15000);

        // Wait for CPU to be ready for the next frame
        *flag_pru = 1;
        while(*flag_pru == 1) {}

    }

}

