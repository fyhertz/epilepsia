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

#include "prudriver.hpp"
#include <cstdint>
#include <vector>

namespace epilepsia {

class led_driver {
public:
    led_driver(led_driver const&) = delete;
    led_driver(led_driver&&) = delete;
    led_driver& operator=(led_driver const&) = delete;
    led_driver& operator=(led_driver&&) = delete;

    led_driver(const int strip_length, const int strip_count);
    ~led_driver();

    void commit_frame_buffer(uint8_t* buffer, int len);
    void clear();

    void set_brightness(const float brightness);
    void set_dithering(const bool dithering);

private:
    void update_lut();

    template <typename T>
    static void remap_bits(uint32_t* in, uint32_t* out, int len);

    template <bool dithering>
    void update_buffer(uint8_t* buffer);

    const int strip_count_;
    const int strip_length_;
    const int bytes_per_strip_;
    const int frame_buffer_size_;

    int lut_[256];
    float brightness_{ 0.1f };
    bool dithering_{ true };
    std::vector<int> residual_;
    pru_driver pru_driver_;
};
}

#endif // EPILEPSIADRIVER_H
