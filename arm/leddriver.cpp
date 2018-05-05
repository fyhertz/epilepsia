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

led_driver::led_driver(const int strip_length, const int strip_count)
    : strip_count_(strip_count) // 8, 16 or 32
    , strip_length_(strip_length) // has to be a multiple of 4
    , bytes_per_strip_(strip_length_ * 3)
    , frame_buffer_size_(bytes_per_strip_ * strip_count)
{
    if (strip_length % 4 != 0) {
        fprintf(stderr, "The length of your strips has to be a multiple of 4\n");
        std::exit(EXIT_FAILURE);
    }

    if (strip_count != 8 && strip_count != 16 && strip_count != 32) {
        fprintf(stderr, "Invalid number of strips\n");
        std::exit(EXIT_FAILURE);
    }

    // PRU shared mem = 12kB
    if (frame_buffer_size_ > 12 * 1024 - 4) {
        fprintf(stderr, "Not enough memory\n");
        std::exit(EXIT_FAILURE);
    }

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

    flag_pru_[0] = &shared_memory_[0];
    flag_pru_[1] = &shared_memory_[1];
    shared_memory_[2] = strip_length_ & 0xFF;
    shared_memory_[3] = strip_count_;
    frame_ = reinterpret_cast<uint32_t*>(&shared_memory_[4]);

    printf("Strip count: %d\n", strip_count_);
    printf("Strip length: %d\n", strip_length_);
    printf("Frame buffer size: %d\n", frame_buffer_size_);

    update_lut();
}

led_driver::~led_driver()
{
    close(mem_fd_);
}

void led_driver::clear()
{
    uint8_t buf[frame_buffer_size_];
    memset(buf, 0, frame_buffer_size_);
    commit_frame_buffer(buf, frame_buffer_size_);
}

void led_driver::commit_frame_buffer(uint8_t* buffer, int len)
{
    // RGB to GRB, gamma correction and brightness adjustment
    for (auto i = 0; i < frame_buffer_size_; i += 3) {
        uint8_t p = buffer[i];
        buffer[i] = lut_[0][buffer[i + 1]];
        buffer[i + 1] = lut_[0][p];
        buffer[i + 2] = lut_[0][buffer[i + 2]];
    }

    // Every two lines of the display is wired upside-down
    for (auto i = bytes_per_strip_ / 2; i < frame_buffer_size_; i += bytes_per_strip_) {
        for (auto j = 0; j < bytes_per_strip_ / 4; j += 3) {
            uint8_t p = buffer[i + j];
            buffer[i + j] = buffer[i + bytes_per_strip_ / 2 - 1 - j - 2];
            buffer[i + bytes_per_strip_ / 2 - 1 - j - 2] = p;

            p = buffer[i + j + 1];
            buffer[i + j + 1] = buffer[i + bytes_per_strip_ / 2 - 1 - j - 1];
            buffer[i + bytes_per_strip_ / 2 - 1 - j - 1] = p;

            p = buffer[i + j + 2];
            buffer[i + j + 2] = buffer[i + bytes_per_strip_ / 2 - 1 - j];
            buffer[i + bytes_per_strip_ / 2 - 1 - j] = p;
        }
    }

    uint32_t tmp[frame_buffer_size_ / 4];
    uint32_t* in = reinterpret_cast<uint32_t*>(buffer);
    uint32_t out[frame_buffer_size_ / 4];

    if (len < frame_buffer_size_) {
        memcpy(tmp, in, len);
        in = tmp;
    }

    if (strip_count_ == 8) {
        remap_bits<uint8_t>(in, out, bytes_per_strip_ / 4);
    } else if (strip_count_ == 16) {
        remap_bits<uint16_t>(in, out, bytes_per_strip_ / 4);
    } else {
        remap_bits<uint32_t>(in, out, bytes_per_strip_ / 4);
    }

    wait_for_pru();
    memcpy(frame_, out, frame_buffer_size_); // 280us for 5760 bytes
}

void led_driver::wait_for_pru()
{
    // Wait for both PRU to be ready for the next frame
    while (*flag_pru_[0] == 0 || *flag_pru_[1] == 0) {
        usleep(10);
    }
    *flag_pru_[0] = 0;
    *flag_pru_[1] = 0;
}

template <typename T>
void led_driver::remap_bits(uint32_t* in, uint32_t* out, const int len)
{
    constexpr const uint32_t mask = sizeof(T) == 4 ? 0x00000001 : sizeof(T) == 2 ? 0x00010001 : 0x01010101;

    for (auto i = 0, ii = 0; i < len; i++, ii += 32) {
        for (size_t l = 0; l < sizeof(T) * 8; l += 8) {
            for (auto j = 0; j < 8; j++) {
                uint32_t m = 0;
                for (size_t k = 0, kk = 0; k < sizeof(T) * 8; k++, kk += len) {
                    uint32_t n = in[i + kk];
                    m |= (((n >> (7 + l - j)) & mask) << (sizeof(T) * 8 - 1 - k));
                }
                for (size_t k = 0; k < 32; k += sizeof(T) * 8) {
                    reinterpret_cast<T*>(out)[ii + j + l + k] = (m >> k) & static_cast<T>(0xFFFFFFFF);
                }
            }
        }
    }
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