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

#include "settings.hpp"
#include <spdlog/spdlog.h>
#include <fstream>
#include <iomanip>
#include <json.hpp>

namespace epilepsia {

void to_json(nlohmann::json& j, const epilepsia::settings& conf)
{
    j = nlohmann::json{
        { "server", {
            {"ports", conf.server_ports } 
            } },
        { "strips", {
            { "length", conf.driver.strip_length },
            { "count", conf.driver.strip_count }
            } },
        { "leds", {
            { "zigzag", conf.driver.zigzag },
            { "dithering", conf.driver.dithering },
            { "brightness", conf.driver.brightness } } }
    };
}

settings::settings(std::string file)
    : file_(file)
{
    load_settings();
}

void settings::load_settings()
{
    std::ifstream i(file_);

    if (!i.good()) {
        spdlog::error("Failed to open \"{}\".", file_);
        std::exit(EXIT_FAILURE);
    }

    // Parse json config file
    nlohmann::json j;
    i >> j;
    
    const nlohmann::json& j1 = j.at("server");
    const nlohmann::json& j2 = j.at("strips");
    const nlohmann::json& j3 = j.at("leds");

    server_ports = j1.at("ports").get<std::vector<uint16_t>>();

    driver = {
        j2.at("length").get<int>(),
        j2.at("count").get<int>(),
        j3.at("zigzag").get<bool>(),
        j3.at("dithering").get<bool>(),
        j3.at("brightness").get<float>()
    };
}

void settings::dump_settings()
{
    std::ofstream o(file_);
    nlohmann::json j = *this;
    o << std::setw(4) << j << std::endl;
}

} // namespace epilepsia