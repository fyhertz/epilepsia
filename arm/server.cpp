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

#include "server.h"
#include "sha1.h"
#include <algorithm>
#include <arpa/inet.h>
#include <errno.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace epilepsia {

server::server(std::initializer_list<uint16_t> ports)
    : ports_(ports)
{
}

bool server::start()
{
    if (!running_) {
        if (listen()) {
            running_ = true;
            thread_ = std::thread(&server::run, this);
            return true;
        }
        return false;
    }
    return true;
}

void server::stop()
{
    if (running_) {
        running_ = false;
        thread_.join();
    }
}

bool server::listen()
{
    // Create a server socket for each requested port.
    for (auto& port : ports_) {
        sockaddr_in address;
        int one = 1;

        int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        address.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(sock, (sockaddr*)&address, sizeof(address)) != 0) {
            fprintf(stderr, "Could not bind to port %d: ", port);
        } else if (::listen(sock, 0) != 0) {
            fprintf(stderr, "Could not listen on port %d: ", port);
        } else {
            printf("Listening on port %d\n", port);
            listen_socks_.push_back(sock);
        }
    }

    if (listen_socks_.empty())
        return false;
    return true;
}

void server::run()
{
    uint32_t timeout_ms = 2000;
    fd_set active_fd_set, read_fd_set;
    sockaddr_in clientname;
    timeval timeout;
    socklen_t address_len = sizeof(clientname);
    char buffer[64];

    // Initialize the set of active sockets.
    FD_ZERO(&active_fd_set);
    for (auto& sock : listen_socks_)
        FD_SET(sock, &active_fd_set);

    while (running_) {
        // Block until input arrives on one or more active sockets.
        read_fd_set = active_fd_set;
        timeout.tv_sec = timeout_ms / 1000;
        timeout.tv_usec = (timeout_ms % 1000) * 1000;
        select(FD_SETSIZE, &read_fd_set, NULL, NULL, &timeout);

        for (auto& i : listen_socks_) {
            if (FD_ISSET(i, &read_fd_set)) {
                // Connection request on one of the server sockets.
                int sock = accept(i, (struct sockaddr*)&clientname, &address_len);
                if (sock >= 0) {
                    inet_ntop(AF_INET, &(clientname.sin_addr), buffer, 64);
                    fprintf(stderr, "Client connected from %s\n", buffer);
                    FD_SET(sock, &active_fd_set);
                    clients_.emplace(sock, Client(sock, *this));
                }
                FD_CLR(i, &read_fd_set);
            }
        }

        // Service all the clients with input pending.
        for (int i = 0; i < FD_SETSIZE; ++i) {
            if (FD_ISSET(i, &read_fd_set)) {
                // Data arriving on socket.
                if (!clients_.at(i).read()) {
                    ::close(i);
                    FD_CLR(i, &active_fd_set);
                    fprintf(stderr, "Client disconnected\n");
                    clients_.erase(i);
                }
            }
        }
    }
}

void server::call_handler(uint16_t payload_len, uint8_t* opc_packet)
{
    if (opc_packet[1] == static_cast<int>(opc_command::set_pixels)) {
        handlers_[0](opc_packet[0], payload_len, opc_packet + 4);
    } else if (opc_packet[1] == static_cast<int>(opc_command::system_exclusive)) {
        handlers_[1](opc_packet[0], payload_len, opc_packet + 4);
    }
}

bool server::Client::read()
{
    if (state == client_state::opc) {
        return handle_opc();
    } else if (state == client_state::websocket) {
        return handle_websocket_data();
    }

    // We use the first 4 bytes to demultiplex OPC and websocket clients
    if (received < 4) {
        ssize_t len = recv(fd, buffer.data(), 4 - received, 0);
        if (len > 0) {
            received += len;
        } else {
            // IO error or client shutdown
            return false;
        }
    }

    if (received == 4) {
        const std::array<uint8_t, 4> a = { 'G', 'E', 'T', ' ' };
        if (std::equal(std::begin(a), std::end(a), std::begin(buffer))) {
            fprintf(stderr, "Websocket connection\n");
            state = client_state::websocket_handshake;
            return handle_websocket_handshake();
        } else {
            fprintf(stderr, "OPC connection\n");
            state = client_state::opc;
            return handle_opc();
        }
    }

    return true;
}

bool server::Client::handle_opc()
{
    ssize_t len = 0;

    if (!payload_length) {
        if (received < 4) {
            len = recv(fd, buffer.data(), 4 - received, 0);
            if (len > 0) {
                received += len;
            } else {
                return false;
            }
        }

        // Header complete
        if (received == 4) {
            payload_length = ((buffer[2] << 8) | buffer[3]);
            received = 0;
        }
    }

    if (payload_length > 0) {
        if (received < payload_length) {
            len = recv(fd, buffer.data() + 4 + received, payload_length - received, 0);
            if (len > 0) {
                received += len;
            }
        }

        // Payload complete
        if (received == payload_length) {
            server_.call_handler(payload_length, buffer.data());
            received = 0;
            payload_length = 0;
        }
    }

    if (len <= 0) {
        // IO error or client shutdown
        return false;
    }

    return true;
}

bool server::Client::handle_websocket_handshake()
{
    char* buf = reinterpret_cast<char*>(buffer.data());
    ssize_t len = recv(fd, buf + received, buffer.size() - received, 0);

    if (len > 0) {
        received += len;
        auto sbuf = std::string(buf, received);

        // We have received all the headers of the HTTP GET
        if (sbuf.find("\r\n\r\n") != std::string::npos) {
            std::istringstream sstream(sbuf);
            std::string line;
            std::map<std::string, std::string> headers;

            // Method and HTTP version
            std::getline(sstream, line);

            // Parse HTTP headers
            // TODO: check Origin
            while (std::getline(sstream, line)) {
                if (line.length() > 3) {
                    line.pop_back(); // Remove trailling \r
                    auto i = line.find(":");
                    headers.emplace(line.substr(0, i), line.substr(i + 2)); // TODO: header needs better trimming
                }
            }

            for (auto& i : headers) {
                std::cout << i.first << ":" << i.second << std::endl;
            }

            // Compute challenge
            using namespace std::string_literals;
            std::string key = headers["Sec-WebSocket-Key"s];
            char result[SHA1_BASE64_SIZE];
            sha1((key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"s).c_str()).finalize().print_base64(result);

            auto reply = "HTTP/1.1 101 Switching Protocols\r\n"
                         "server: epilepsia\r\n"
                         "Upgrade: websocket\r\n"
                         "Connection: Upgrade\r\n"
                         "Sec-WebSocket-Accept: "s;

            reply += std::string(result);
            reply += "\r\n\r\n";

            std::cout << reply << std::endl;

            // Send HTTP response to client and switch to websocket
            ::send(fd, reply.c_str(), reply.length(), 0);
            state = client_state::websocket;
            received = 0;
        }

        return true;

    } else {
        // IO error or client shutdown
        return false;
    }
}

bool server::Client::handle_websocket_data()
{
    ssize_t len = 0;

    // Receive packet header to get payload length and masking key
    if (!payload_length) {
        if (received < 6) {
            len = recv(fd, buffer.data() + received, 6 - received, 0);
            if (len > 0) {
                received += len;
            }
        }

        if (received >= 6) {
            // TODO: MASK bit should be one
            // TODO: handle message fragmentation
            uint8_t opcode = buffer[0] & 0x0F;
            uint8_t fin = buffer[0] >> 7;
            uint8_t length = buffer[1] & 0x7F;
            
            // Either text mode or fragmented packet
            if (opcode != 2 || fin == 0) {
                std::cout << "Wrong opcode: " << opcode << std::endl;
                return false;
            }

            if (length == 126) {
                len = recv(fd, buffer.data() + received, 8 - received, 0);
                if (len > 0) {
                    received += len;
                }
                if (received == 8) {
                    payload_length = ((buffer[2] << 8) | buffer[3]);
                    std::copy_n(buffer.begin() + 4, 4, masking_key_.begin());
                    received = 0;
                }
            } else if (length == 127) {
                //Not supported. Probably not needed.
                std::cout << "Wow much bytes!" << std::endl;
                return false;
            } else {
                payload_length = length;
                std::copy_n(buffer.begin() + 2, 4, masking_key_.begin());
                received = 0;
            }
        }
    }

    if (payload_length > 0) {

        // Receive payload
        if (received < payload_length) {
            len = recv(fd, buffer.data() + received, payload_length - received, 0);
            if (len > 0) {
                // Unmask payload
                for (size_t i = received; i < received + len; i++) {
                    buffer[i] = buffer[i] ^ masking_key_[i % 4];
                }
                received += len;
            }
        }

        // Payload received
        if (received == payload_length) {
            server_.call_handler(payload_length - 4, buffer.data());
            received = 0;
            payload_length = 0;
        }
    }

    // IO error or client shutdown
    if (len <= 0) {
        return false;
    }

    return true;
}

} // namespace epilepsia