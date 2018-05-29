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

#ifndef EPILEPSIAPRUDRIVER_H
#define EPILEPSIAPRUDRIVER_H

#include <cstdint>
#include <algorithm>

namespace epilepsia {

/**
 * Handle communication with the PRUs.
 * Manage a lock to syncronize the PRUs with the ARM.
 */
class pru_driver {
public:
    pru_driver(pru_driver const&) = delete;
    pru_driver(pru_driver&&) = delete;
    pru_driver& operator=(pru_driver const&) = delete;
    pru_driver& operator=(pru_driver&&) = delete;

    explicit pru_driver(const int pru_count);
    ~pru_driver();

    /**
     * Tell the PRUs how to parse the incomming frames.
     */
    void write_config(const int strip_length, const int strip_count);

    /**
     * Block until the PRU(s) is/are ready to read a new frame,
     * unlock the PRU(s) and write the new frame to the shared memory.
     */
    template <typename T>
    void write_frame(T buffer, const int len)
    {
        block_until_ready();
        std::copy_n(buffer, len, reinterpret_cast<T>(frame_)); // 280us for 5760 bytes
    }

private:
    void block_until_ready();
    void halt();
    bool ready() const;

    const int pru_count_;
    int mem_fd_;
    uint8_t* shared_memory_;
    uint16_t* flag_pru_;
    uint32_t* frame_;
};
}

#endif // EPILEPSIAPRUDRIVER_H
