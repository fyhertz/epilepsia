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

#include "prudriver.hpp"
#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

namespace epilepsia {

pru_driver::pru_driver(const int pru_count)
    : pru_count_(pru_count)
{

    mem_fd_ = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd_ < 0) {
        fprintf(stderr, "Failed to open /dev/mem (%s)\n", strerror(errno));
        std::exit(EXIT_FAILURE);
    }

    // Address of the PRUs shared memory on a am335 soc
    shared_memory_ = static_cast<uint8_t*>(mmap(0, 0x00003000, PROT_WRITE | PROT_READ, MAP_SHARED, mem_fd_, 0x4A300000 + 0x00010000));
    if (shared_memory_ == NULL) {
        fprintf(stderr, "Failed to map the device (%s)\n", strerror(errno));
        close(mem_fd_);
        std::exit(EXIT_FAILURE);
    }

    flag_pru_ = reinterpret_cast<uint16_t*>(shared_memory_);
    frame_ = reinterpret_cast<uint32_t*>(&shared_memory_[4]);
}

pru_driver::~pru_driver()
{
    halt();
    close(mem_fd_);
}

void pru_driver::write_config(const int strip_length, const int strip_count)
{
    shared_memory_[2] = strip_length & 0xFF;
    shared_memory_[3] = strip_count;
}

void pru_driver::block_until_ready()
{
    int n = 0;
    // Wait for PRU(s) to be ready for the next frame
    while (!ready()) {
        nanosleep((const struct timespec[]){ { 0, 15000L } }, NULL);
        if (n++ > 10000) {
            // PRU(s) not running... we discard the frame
            break;
        }
    }
    *flag_pru_ = 0;
}

void pru_driver::halt()
{
    printf("Waiting for PRUs... ");
    // We wait 200 ms, enough to be sure that PRU 0 or (PRU 0 and PRU 1) is/are waiting.
    nanosleep((const struct timespec[]){ { 0, 200000000L } }, NULL);
    shared_memory_[3] = 0xFF;
    if (!ready()) {
        printf("PRU(s) not running\n");
    } else {
        printf("OK\n");
    }
    *flag_pru_ = 0;
}

inline bool pru_driver::ready() const
{
    return pru_count_ == 2 ? *flag_pru_ == 0x0101 : *flag_pru_ > 0;
}
}
