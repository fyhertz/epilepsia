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

#ifndef LEDDISPLAY_H
#define LEDDISPLAY_H

#include <math.h>
#include <stdint.h>
#include <thread>

namespace epilepsia {

class LedDisplay {
public:
    static const int bytes_per_strip = 120 * 3; // 3 bytes per led, 120 leds
    static const int frame_buffer_size = bytes_per_strip * 16; // 16 strips

    static LedDisplay& get_instance()
    {
        static LedDisplay instance;
        return instance;
    }

    LedDisplay(LedDisplay const&) = delete;
    LedDisplay(LedDisplay&&) = delete;
    LedDisplay& operator=(LedDisplay const&) = delete;
    LedDisplay& operator=(LedDisplay&&) = delete;

    uint8_t* get_frame_buffer();
    void commit_frame_buffer();
    void clear();

    void set_brightness(const float brightness);
    void set_dithering(const bool dithering);

protected:
    LedDisplay();
    ~LedDisplay();

private:
    void swap_pru_buffers();
    void remap_bits();
    void update_lut();

    int mem_fd_;
    int current_buffer = 0;
    uint8_t* shared_memory_;
    uint8_t* flag_pru_[2];
    uint8_t* frame_[2];
    uint8_t frame_buffer_[frame_buffer_size];
    uint8_t lut_[2][256];
    float brightness_{ 0.1f };
    bool dithering_{ true };
};
}

#endif // LEDDISPLAY_H
