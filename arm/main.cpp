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
#include <stdint.h>
#include <signal.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "opcsource.h"
#include "leddisplay.h"

using namespace std;

volatile sig_atomic_t done = 0;

int main(int argc, char *argv[]) {

    LedDisplay& display = LedDisplay::getInstance();
    opc::Source source;

    display.setBrightness(0.05f, true);

    signal(SIGINT, [](int signum){
        printf("done!\n");
        done = 1;
    });

    source.setHandler([&](uint8_t channel, uint16_t count, opc::Pixel* pixels){
        auto* p = reinterpret_cast<uint8_t*>(pixels);
        auto* fb = display.getFrameBuffer();
        memcpy(fb, p, (count>60*32) ? 60*32*3 : count*3);
        display.commitFrameBuffer();
    });

    source.start();

    while (!done) {
        sleep(1);
    }

    source.stop();
    display.clear();

    return 0;
}
