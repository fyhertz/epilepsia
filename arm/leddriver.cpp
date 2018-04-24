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

#include "leddriver.h"
#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

namespace epilepsia {

led_driver::led_driver()
{
    mem_fd_ = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd_ < 0) {
        printf("Failed to open /dev/mem (%s)\n", strerror(errno));
        std::exit(EXIT_FAILURE);
    }

    // Address of the PRUs shared memory in a am335 soc
    shared_memory_ = static_cast<uint8_t*>(mmap(0, 0x00003000, PROT_WRITE | PROT_READ, MAP_SHARED, mem_fd_, 0x4A300000 + 0x00010000));
    if (shared_memory_ == NULL) {
        printf("Failed to map the device (%s)\n", strerror(errno));
        close(mem_fd_);
        std::exit(EXIT_FAILURE);
    }

    //memset(shared_memory_, 0, 12*1024); // reset shared memory

    flag_pru_[0] = &shared_memory_[0];
    flag_pru_[1] = &shared_memory_[1];
    frame_[0] = &shared_memory_[2];
    frame_[1] = &shared_memory_[2 + frame_buffer_size];

    update_lut();
}

led_driver::~led_driver()
{
    close(mem_fd_);
}

uint8_t* led_driver::get_frame_buffer()
{
    return frame_buffer_;
}

void led_driver::clear()
{
    memset(frame_buffer_, 0, frame_buffer_size);
    commit_frame_buffer();
}

void led_driver::commit_frame_buffer()
{
    remap_bits();
    swap_pru_buffers();
    remap_bits();
    swap_pru_buffers();
}

void led_driver::swap_pru_buffers()
{
    current_buffer = !current_buffer;
    // Wait for both PRU to be ready for the next frame
    while (*flag_pru_[0] == 0 || *flag_pru_[1] == 0) {
        usleep(10);
    }
    *flag_pru_[0] = 0;
    *flag_pru_[1] = 0;
}

void led_driver::remap_bits()
{
    uint8_t* frame = frame_[current_buffer];
    uint8_t tmp1[frame_buffer_size];
    uint8_t tmp2[frame_buffer_size];
    uint8_t p;

    memcpy(tmp1, frame_buffer_, frame_buffer_size);

    // RGB to GRB, gamma correction and brightness adjustment
    for (auto i = 0; i < frame_buffer_size; i += 3) {
        p = tmp1[i];
        tmp1[i] = lut_[current_buffer][tmp1[i + 1]];
        tmp1[i + 1] = lut_[current_buffer][p];
        tmp1[i + 2] = lut_[current_buffer][tmp1[i + 2]];
    }

    // Every two lines of the display is wired upside-down
    for (auto i = bytes_per_strip / 2; i < frame_buffer_size; i += bytes_per_strip) {
        for (auto j = 0; j < bytes_per_strip / 4; j += 3) {
            p = tmp1[i + j];
            tmp1[i + j] = tmp1[i + bytes_per_strip / 2 - 1 - j - 2];
            tmp1[i + bytes_per_strip / 2 - 1 - j - 2] = p;

            p = tmp1[i + j + 1];
            tmp1[i + j + 1] = tmp1[i + bytes_per_strip / 2 - 1 - j - 1];
            tmp1[i + bytes_per_strip / 2 - 1 - j - 1] = p;

            p = tmp1[i + j + 2];
            tmp1[i + j + 2] = tmp1[i + bytes_per_strip / 2 - 1 - j];
            tmp1[i + bytes_per_strip / 2 - 1 - j] = p;
        }
    }

    // We reorder the bits of the buffer for the
    // serial to parallel shift registers
    {
        uint32_t m = 0, n = 0;
        uint32_t* tmp1r = reinterpret_cast<uint32_t*>(tmp1);

        for (auto l = 0, ll = 0; l < frame_buffer_size / 4; l += frame_buffer_size / 8, ll += frame_buffer_size / 2) {
            for (auto i = 0, ii = 0; i < bytes_per_strip / 4; i++, ii += 32) {
                for (auto j = 0; j < 8; j++) {
                    m = 0;
                    for (auto k = 0, kk = 0; k < 8; k++, kk += bytes_per_strip / 4) {
                        n = tmp1r[l + i + kk];
                        m |= (((n >> (7 - j)) & 0x01010101) << (7 - k));
                    }
                    tmp2[ll + ii + j] = (m >> 0) & 0xFF;
                    tmp2[ll + ii + 8 + j] = (m >> 8) & 0xFF;
                    tmp2[ll + ii + 16 + j] = (m >> 16) & 0xFF;
                    tmp2[ll + ii + 24 + j] = (m >> 24) & 0xFF;
                }
            }
        }
    }

    memcpy(frame, tmp2, frame_buffer_size);
}

void led_driver::set_brightness(const float brightness)
{
    if (brightness_ != brightness) {
        brightness_ = brightness;
        update_lut();
    }
}

void led_driver::set_dithering(const bool dithering)
{
    if (dithering_ != dithering) {
        dithering_ = dithering;
        update_lut();
    }
}

void led_driver::update_lut()
{
    static const std::array<uint8_t, 256> gamma8{
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2,
        2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5,
        5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10,
        10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
        17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
        25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
        37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
        51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
        69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
        90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
        115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
        144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
        177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
        215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255
    };

    for (auto i = 0; i < 256; i++) {
        lut_[0][i] = std::ceil(gamma8[i] * brightness_);
        lut_[1][i] = dithering_ ? std::floor(gamma8[i] * brightness_) : lut_[0][i];
    }
}
}