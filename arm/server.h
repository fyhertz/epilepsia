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

#ifndef EPILEPSIASERVER_H
#define EPILEPSIASERVER_H

#include <atomic>
#include <functional>
#include <initializer_list>
#include <map>
#include <stdint.h>
#include <thread>
#include <vector>

namespace epilepsia {

enum class OpcCommand {
    set_pixels = 0,
    system_exclusive = 0xFF
};

class Server {
public:
    using Handler = std::function<void(uint8_t, uint16_t, uint8_t*)>;

    Server(std::initializer_list<uint16_t> ports);

    Server(Server const&) = delete;
    Server& operator=(Server const&) = delete;

    bool start() noexcept;
    void stop() noexcept;

    template <OpcCommand command, typename T>
    void set_handler(T&& handler) noexcept
    {
        handlers_[command == OpcCommand::set_pixels ? 0 : 1] = handler;
    }

private:
    struct Client {
        std::string sin_addr;
        size_t received{ 0 };
        uint16_t total_length{ 0 };
        uint8_t buffer[(1 << 16) + 4];
    };

    std::thread thread_;
    std::map<int, Client> clients_;
    std::vector<uint16_t> ports_;
    std::vector<int> listen_socks_;
    std::atomic<bool> running_{ false };
    std::array<Handler, 2> handlers_;

    void run();
    bool listen();
    bool read_from_client(int client);
};

} // namespace epilepsia

#endif // EPILEPSIASERVER_H