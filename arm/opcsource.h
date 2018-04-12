/*
 * Copyright (C) 2013 Ka-Ping Yee
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

#ifndef OPCSOURCE_H
#define OPCSOURCE_H

#include <stdint.h>
#include <thread>
#include <functional>
#include <atomic>

namespace opc {

// Default port for the OPC server
static const int DEFAULT_PORT = 7890;

// OPC command codes
static const int SET_PIXELS = 0;

// Maximum number of pixels in one message
static const int MAX_PIXELS_PER_MESSAGE = ((1 << 16) / 3);

struct Pixel { uint8_t r, g, b; };

using Handler = std::function<void(uint8_t, uint16_t, Pixel*)>;

class Source
{

public:
    Source(const int port = DEFAULT_PORT);

    Source(Source const&) = delete;
    Source& operator =(Source const&) = delete;

    void setHandler(Handler&& handler);
    bool start();
    void stop();
    bool listen();
    uint8_t receive(uint32_t timeout_ms);
    void reset();
    void close();

protected:
    std::atomic<bool> running_{false};
    void run();

private:
    int listen_sock_;
    int sock_;
    uint16_t port_;
    uint16_t header_length_;
    uint8_t header_[4];
    uint16_t payload_length_;
    uint8_t payload_[1 << 16];
    std::thread thread_;
    Handler handler_;

};

}
#endif // OPCSOURCE_H
