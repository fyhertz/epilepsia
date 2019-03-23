/*
 * Copyright (C) 2018-2019 Simon Guigui
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
#include "settings.hpp"
#include <spdlog/spdlog.h>
#include <chrono>
#include <clara.hpp>
#include <iostream>
#include <signal.h>

volatile sig_atomic_t done = 0;

void estimate_frame_rate()
{
    static auto start = std::chrono::steady_clock::now();
    static uint32_t counter{ 0 };
    auto elapsed = std::chrono::steady_clock::now() - start;

    if (std::chrono::duration<double, std::milli>(elapsed).count() > 1000) {
        spdlog::debug("Frame rate: {}", counter);
        start = std::chrono::steady_clock::now();
        counter = 0;
    }
    counter++;
}

int main(int argc, char* argv[])
{
    bool help = false;
    std::string file = "epilepsia.json";

    auto cli = clara::Help(help)
        | clara::Opt(file, "filename")
              ["-c"]["--conf"]("Path to configuration file");

    auto parser = cli.parse(clara::Args(argc, argv));
    if (!parser) {
        spdlog::error("Error in command line: {}", parser.errorMessage());
        exit(EXIT_FAILURE);
    }

    if (help) {
        std::cout << cli << std::endl;
        exit(EXIT_SUCCESS);
    }

    epilepsia::settings settings(file);
    epilepsia::opc_server server(settings.server_ports);
    epilepsia::led_driver display(settings.driver);

    signal(SIGINT, [](int signum) {
        done = 1;
    });

    server.set_handler<epilepsia::opc_command::set_pixels>([&](uint8_t channel, uint16_t length, uint8_t* pixels) {
        display.commit_frame_buffer(pixels, length);
        estimate_frame_rate();
    });

    server.set_handler<epilepsia::opc_command::system_exclusive>([&](uint8_t channel, uint16_t length, uint8_t* data) {
        if (length == 2) {
            switch (data[0]) {

            // Change brightness
            case 0x00:
                settings.driver.brightness = data[1] / 255.0f;
                break;

            // Enable/disable dithering
            case 0x01:
                settings.driver.dithering = data[1];
                break;
            }

            // Save new settings
            settings.dump_settings();
        }
    });

    if (!server.start()) {
        std::exit(EXIT_FAILURE);
    }

    while (!done) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    server.stop();
    display.clear();

    return 0;
}
