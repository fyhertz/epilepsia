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
#include <fstream>
#include <iostream>
#include <json.hpp>
#include <signal.h>

volatile sig_atomic_t done = 0;

struct config {
    std::vector<uint16_t> ports;
    int strip_length;
    int strip_count;
    float brightness;
    bool dithering;
};

void from_json(const nlohmann::json& j, config& conf)
{
    conf.ports = j.at("ports").get<std::vector<uint16_t>>();
    conf.strip_length = j.at("strip_length").get<int>();
    conf.strip_count = j.at("strip_count").get<int>();
    conf.brightness = j.at("brightness").get<float>();
    conf.dithering = j.at("dithering").get<bool>();
};

void to_json(nlohmann::json& j, const config& conf)
{
    j = nlohmann::json{
        { "ports", conf.ports },
        { "strip_length", conf.strip_length },
        { "strip_count", conf.strip_count },
        { "brightness", conf.brightness },
        { "dithering", conf.dithering }
    };
}

config read_conf(const std::string& file)
{
    std::ifstream i(file);

    if (!i.good()) {
        std::cerr << "Failed to open \""
                  << file
                  << "\"." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // Parse json config file
    nlohmann::json j;
    i >> j;
    config conf = j;

    return conf;
}

void write_conf(const std::string& file, const config& conf)
{
    std::ofstream o(file);
    nlohmann::json j = conf;
    o << std::setw(4) << j << std::endl;
}

void estimate_frame_rate()
{
    static auto start = std::chrono::steady_clock::now();
    static uint32_t counter{ 0 };
    auto elapsed = std::chrono::steady_clock::now() - start;

    if (std::chrono::duration<double, std::milli>(elapsed).count() > 1000) {
        std::cout << "Frame rate: " << counter << std::endl;
        start = std::chrono::steady_clock::now();
        counter = 0;
    }
    counter++;
}

int main(int argc, char* argv[])
{
    std::string config_file = "epilepsia.json";
    config conf = read_conf(config_file);

    epilepsia::opc_server server(conf.ports);
    epilepsia::led_driver display(conf.strip_length, conf.strip_count);

    display.set_brightness(conf.brightness);
    display.set_dithering(conf.dithering);

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
                conf.brightness = data[1] / 255.0f;
                display.set_brightness(conf.brightness);
                break;

            // Enable/disable dithering
            case 0x01:
                conf.dithering = data[1];
                display.set_dithering(conf.dithering);
                break;
            }

            write_conf(config_file, conf);
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
