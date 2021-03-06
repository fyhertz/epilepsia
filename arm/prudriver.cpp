/*
 * Copyright (C) 2018-2019 Simon Guigui
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
#include <fstream>
#include <iostream>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

namespace epilepsia {

pru_driver::pru_driver(const int strip_length, const int strip_count)
    : pru_count_(strip_count == 32 ? 2 : 1)
{
    mem_fd_ = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd_ < 0) {
        spdlog::critical("Failed to open /dev/mem {}", strerror(errno));
        std::exit(EXIT_FAILURE);
    }

    // Address of the PRUs shared memory on a am335 soc
    shared_memory_ = static_cast<uint8_t*>(mmap(0, 0x00003000, PROT_WRITE | PROT_READ, MAP_SHARED, mem_fd_, 0x4A300000 + 0x00010000));
    if (shared_memory_ == NULL) {
        spdlog::critical("Failed to map the device {}", strerror(errno));
        close(mem_fd_);
        std::exit(EXIT_FAILURE);
    }

    // The PRUs need to know the size of the frame buffer
    shared_memory_[2] = strip_length & 0xFF;
    shared_memory_[3] = strip_count;

    // Load firmware and start PRU 0
    write_rproc_sysfs(0, "firmware", "am335x-epilepsia-pru0-fw");
    write_rproc_sysfs(0, "state", "start");

    // Load firmware and start PRU 1
    if (pru_count_ == 2) {
        write_rproc_sysfs(1, "firmware", "am335x-epilepsia-pru1-fw");
        write_rproc_sysfs(1, "state", "start");
    }

    flag_pru_ = reinterpret_cast<uint16_t*>(shared_memory_);
    frame_ = reinterpret_cast<uint32_t*>(&shared_memory_[4]);
}

pru_driver::~pru_driver()
{
    halt();
    close(mem_fd_);
    write_rproc_sysfs(0, "state", "stop");
    if (pru_count_ == 2)
        write_rproc_sysfs(1, "state", "stop");
}

void pru_driver::write_rproc_sysfs(int pru_id, const char* filename, const char* value)
{
    using namespace std::string_literals;
    auto path = "/sys/class/remoteproc/remoteproc"s + std::to_string(pru_id + 1) + "/" + filename;
    std::ofstream f;
    f.open(path);
    if (!f.fail()) {
        f << value;
        f.close();
    } else {
        spdlog::critical("Could not write \"{}\" to \"{}\"", value, path);
        std::exit(EXIT_FAILURE);
    }
}

void pru_driver::block_until_ready()
{
    int n = 0;
    // Wait for PRU(s) to be ready for the next frame
    while (!ready()) {
        nanosleep((const struct timespec[]){ { 0, 15000L } }, NULL);
        if (n++ > 10000) {
            // PRU(s) not running...
            spdlog::critical("PRU(s) not running");
            std::exit(EXIT_FAILURE);
        }
    }
    *flag_pru_ = 0;
}

void pru_driver::halt()
{
    spdlog::info("Waiting for PRUs...");
    // We wait 200 ms, enough to be sure that PRU 0 or (PRU 0 and PRU 1) is/are waiting.
    nanosleep((const struct timespec[]){ { 0, 200000000L } }, NULL);
    shared_memory_[3] = 0xFF;
    if (!ready()) {
        spdlog::warn("PRU(s) not running");
    } else {
        spdlog::info("OK");
    }
    *flag_pru_ = 0;
}

inline bool pru_driver::ready() const
{
    return pru_count_ == 2 ? *flag_pru_ == 0x0101 : *flag_pru_ > 0;
}
}
