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
#include "leddisplay.h"
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

    epilepsia::Server server{ 7890 };
    LedDisplay& display = LedDisplay::getInstance();

    display.setBrightness(0.1f, true);

    signal(SIGINT, [](int signum) {
        std::cout << "Done!" << std::endl;
        done = 1;
    });

    server.set_handler<epilepsia::OpcCommand::set_pixels>([&](uint8_t channel, uint16_t length, uint8_t* pixels) {
        auto* fb = display.getFrameBuffer();
        memcpy(fb, pixels, (length > 60 * 32 * 3) ? 60 * 32 * 3 : length);
        display.commitFrameBuffer();
    });

    server.start();

    while (!done) {
        sleep(1);
    }

    server.stop();
    display.clear();

    return 0;
}
