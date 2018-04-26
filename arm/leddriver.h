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

#ifndef EPILEPSIADRIVER_H
#define EPILEPSIADRIVER_H

#include <math.h>
#include <stdint.h>
#include <thread>

namespace epilepsia {

class led_driver {
public:

    led_driver(led_driver const&) = delete;
    led_driver(led_driver&&) = delete;
    led_driver& operator=(led_driver const&) = delete;
    led_driver& operator=(led_driver&&) = delete;

    led_driver(const size_t strip_length);
    ~led_driver();

    uint8_t* get_frame_buffer();
    void commit_frame_buffer();
    void clear();

    void set_brightness(const float brightness);
    void set_dithering(const bool dithering);

private:
    void swap_pru_buffers();
    void remap_bits();
    void update_lut();

    const int strip_length_;
    const int bytes_per_strip_;
    const int frame_buffer_size_;

    int mem_fd_;
    int current_buffer = 0;
    uint8_t* shared_memory_;
    uint8_t* flag_pru_[2];
    uint8_t* frame_[2];
    uint8_t frame_buffer_[12*1024];
    uint8_t lut_[2][256];
    float brightness_{ 0.1f };
    bool dithering_{ true };
};
}

#endif // EPILEPSIADRIVER_H
