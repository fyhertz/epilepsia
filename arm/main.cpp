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

#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "leddriver.hpp"
#include "opcserver.hpp"
#include <chrono>
#include <iostream>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

volatile sig_atomic_t done = 0;

void estimate_frame_rate()
{
    static auto start = std::chrono::steady_clock::now();
    static uint32_t counter{ 0 };
    auto elapsed = std::chrono::steady_clock::now() - start;

    if (std::chrono::duration<double, std::milli>(elapsed).count() > 1000) {
        std::cout << "Frame rate: "<< counter << std::endl;
        start = std::chrono::steady_clock::now();
        counter = 0;
    }
    counter++;
}

int main(int argc, char* argv[])
{

    epilepsia::opc_server server{ 7890 };
    epilepsia::led_driver display(120, 16);

    display.set_brightness(0.1f);
    display.set_dithering(true);

    signal(SIGINT, [](int signum) {
        std::cout << "Done!" << std::endl;
        done = 1;
    });

    server.set_handler<epilepsia::opc_command::set_pixels>([&](uint8_t channel, uint16_t length, uint8_t* pixels) {
        display.commit_frame_buffer(pixels, length);
        estimate_frame_rate();
    });

    server.set_handler<epilepsia::opc_command::system_exclusive>([&](uint8_t channel, uint16_t length, uint8_t* data) {
        if (length == 2 && data[0] == 0x00) {
            float brightness = data[1] / 255.0f;
            display.set_brightness(brightness);
        } else if (length == 2 && data[0] == 0x01) {
            bool dithering = data[1];
            display.set_dithering(dithering);
        }
    });

    server.start();

    while (!done) {
        sleep(1);
    }

    server.stop();
    display.clear();

    return 0;
}
