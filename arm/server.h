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

class server {
public:
    using Handler = std::function<void(uint8_t, uint16_t, uint8_t*)>;

    server(std::initializer_list<uint16_t> ports);

    server(server const&) = delete;
    server& operator=(server const&) = delete;

    bool start();
    void stop();

    template <OpcCommand command, typename T>
    void set_handler(T&& handler) noexcept
    {
        handlers_[command == OpcCommand::set_pixels ? 0 : 1] = handler;
    }

private:
    void run();
    bool listen();
    void call_handler(uint16_t payload_len, uint8_t *opc_packet);

    class Client {
    public:
        Client(int& fd_, server& server)
            : fd(fd_), server_(server) {}

        bool read();

    private:
        bool handle_opc();
        bool handle_websocket_handshake();
        bool handle_websocket_data();

        enum class ClientState {
            new_connection,
            websocket_handshake,
            websocket,
            opc
        };
        ClientState state{ ClientState::new_connection };

        int fd;
        std::array<uint8_t, (1 << 16) + 4> buffer;
        std::array<uint8_t, 4> masking_key_;
        size_t received{ 0 };
        uint16_t payload_length{ 0 };
        server& server_;
    };

    std::thread thread_;
    std::map<int, Client> clients_;
    std::vector<uint16_t> ports_;
    std::vector<int> listen_socks_;
    std::atomic<bool> running_{ false };
    std::array<Handler, 2> handlers_;
};

} // namespace epilepsia

#endif // EPILEPSIASERVER_H