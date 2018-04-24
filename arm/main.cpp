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
#include "leddriver.h"
#include "server.h"
#include <iostream>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

volatile sig_atomic_t done = 0;

int main(int argc, char* argv[])
{

    epilepsia::server server{ 7890, 8080 };
    epilepsia::led_driver display;

    display.set_brightness(0.1f);
    display.set_dithering(true);

    signal(SIGINT, [](int signum) {
        std::cout << "Done!" << std::endl;
        done = 1;
    });

    server.set_handler<epilepsia::OpcCommand::set_pixels>([&](uint8_t channel, uint16_t length, uint8_t* pixels) {
        auto* fb = display.get_frame_buffer();
        memcpy(fb, pixels, (length > 60 * 32 * 3) ? 60 * 32 * 3 : length);
        display.commit_frame_buffer();
    });

    server.set_handler<epilepsia::OpcCommand::system_exclusive>([&](uint8_t channel, uint16_t length, uint8_t* data) {
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
