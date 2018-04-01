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

#include <stdint.h>
#include <math.h>
#include <thread>

struct ColorLookupTable {

    constexpr ColorLookupTable(float brightness, bool dithering) : v(), dithering(dithering) {

        const uint8_t gamma8[] = {
                0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
                1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
                2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
                5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
               10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
               17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
               25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
               37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
               51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
               69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
               90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
              115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
              144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
              177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
              215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };

        for (auto i=0;i<256;i++) {
            v[0][i] = std::ceil(gamma8[i]*brightness);
            v[1][i] = dithering ? std::floor(gamma8[i]*brightness) : v[0][i];
        }
    }

    constexpr const uint8_t* operator [](int i) {
        return &v[i][0];
    }

    uint8_t v[2][256];
    bool dithering;

};

class LedDisplay
{
public:

    static const int bytes_per_strip = 120*3;  // 3 bytes per led, 120 leds
    static const int frame_buffer_size = bytes_per_strip*16; // 16 strips

    static LedDisplay& getInstance() {
        static LedDisplay instance;
        return instance;
    }

    uint8_t* getFrameBuffer();
    void start();
    void stop();
    void commitFrameBuffer();
    void clear();
    void printFrameRate();
    void setBrightness(float brightness, bool dithering);

    LedDisplay(LedDisplay const&) = delete;
    LedDisplay(LedDisplay&&) = delete;
    LedDisplay& operator=(LedDisplay const&) = delete;
    LedDisplay& operator=(LedDisplay &&) = delete;

protected:

    LedDisplay();
    ~LedDisplay();

private:

    void openSharedMem();
    void closeSharedMem();
    void swapPruBuffers();
    void remapBits();

    int mem_fd_;
    int current_buffer = 0;
    uint8_t *shared_memory_;
    uint8_t *flag_pru_[2];
    uint8_t *frame_[2];
    uint8_t frame_buffer_[frame_buffer_size];
    ColorLookupTable lookup_table_ = ColorLookupTable(0.1f, true);

};

#endif // LEDDISPLAY_H
