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
#include <arpa/inet.h>
#include <errno.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace epilepsia {

Server::Server(std::initializer_list<uint16_t> ports)
    : ports_(ports)
{
}

bool Server::start() noexcept
{
    if (!running_) {
        if (listen()) {
            running_ = true;
            thread_ = std::thread(&Server::run, this);
            return true;
        }
        return false;
    }
    return true;
}

void Server::stop() noexcept
{
    if (running_) {
        running_ = false;
        thread_.join();
    }
}

bool Server::listen()
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

void Server::run()
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
                    fprintf(stderr, "Server: Client connected from %s\n", buffer);
                    FD_SET(sock, &active_fd_set);
                }
                FD_CLR(i, &read_fd_set);
            }
        }

        // Service all the clients with input pending.
        for (int i = 0; i < FD_SETSIZE; ++i) {
            if (FD_ISSET(i, &read_fd_set)) {
                // Data arriving on an client socket.
                if (!read_from_client(i)) {
                    close(i);
                    FD_CLR(i, &active_fd_set);
                }
            }
        }
    }
}

bool Server::read_from_client(int sock)
{
    auto client = clients_[sock];
    auto* buffer = client.buffer;
    size_t len;

    if (client.received < 4) {
        len = recv(sock, buffer, 4 - client.received, 0);
        if (len > 0) {
            client.received += len;
        } else {
            return false;
        }
    }

    // Header complete
    if (client.received == 4) {
        client.total_length = ((buffer[2] << 8) | buffer[3]) + 4;
    }

    if (client.total_length > 0 && client.received < client.total_length) {
        len = recv(sock, buffer + client.received,
            client.total_length - client.received, 0);
        if (len > 0) {
            client.received += len;
        } else {
            return false;
        }
    }

    // Payload complete
    if (client.received == client.total_length) {
        if (buffer[1] == static_cast<int>(OpcCommand::set_pixels)) {
            handlers_[0](buffer[0], client.total_length - 4, buffer + 4);
        } else if (buffer[1] == static_cast<int>(OpcCommand::system_exclusive)) {
            handlers_[1](buffer[0], client.total_length - 4, buffer + 4);
        }
        client.received = 0;
        client.total_length = 0;
    }

    return true;
}
} // namespace epilepsia