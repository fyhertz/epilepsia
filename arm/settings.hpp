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

#ifndef EPILEPSIASETTINGS_H
#define EPILEPSIASETTINGS_H

#include "leddriver.hpp"
#include <string>
#include <vector>

namespace epilepsia {

class settings {
public:
    explicit settings(const std::string& file = "/etc/epilepsia/epilepsia.json");

    void load_settings();
    void dump_settings();

    std::vector<uint16_t> server_ports;
    led_driver_settings driver;

private:
    std::string file_;
};

} // namespace epilepsia

#endif