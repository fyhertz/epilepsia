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

#include <strings.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "opcsource.h"

namespace opc {

Source::Source(int port)
    :port_(port)
{

}

void Source::setHandler(Handler&& handler) {
    // TODO: mutex
    handler_ = handler;
}

bool Source::start() {
    if (!running_ && listen()) {
        running_ = true;
        thread_ = std::thread(&Source::run, this);
        return true;
    }
    return false;
}

void Source::run() {
    while (running_) {
        receive(5000);
    }
}

void Source::stop() {
    if (running_) {
        running_ = false;
        thread_.join();
    }
}

bool Source::listen() {
    struct sockaddr_in address;
    int one = 1;

    listen_sock_ = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    setsockopt(listen_sock_, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    address.sin_family = AF_INET;
    address.sin_port = htons(port_);
    bzero(&address.sin_addr, sizeof(address.sin_addr));
    if (bind(listen_sock_, (struct sockaddr*) &address, sizeof(address)) != 0) {
        fprintf(stderr, "OPC: Could not bind to port %d: ", port_);
        perror(NULL);
        return false;
    }
    if (::listen(listen_sock_, 0) != 0) {
        fprintf(stderr, "OPC: Could not listen on port %d: ", port_);
        perror(NULL);
        return false;
    }

    fprintf(stderr, "OPC: Listening on port %d\n", port_);
    return true;
}

uint8_t Source::receive(uint32_t timeout_ms) {
    int nfds;
    fd_set readfds;
    struct timeval timeout;
    struct sockaddr_in address;
    socklen_t address_len = sizeof(address);
    uint16_t payload_expected;
    ssize_t received = 1;  /* nonzero, because we treat zero to mean "closed" */
    char buffer[64];

    /* Select for inbound data or connections. */
    FD_ZERO(&readfds);
    if (listen_sock_ >= 0) {
        FD_SET(listen_sock_, &readfds);
        nfds = listen_sock_ + 1;
    } else if (sock_ >= 0) {
        FD_SET(sock_, &readfds);
        nfds = sock_ + 1;
    }
    timeout.tv_sec = timeout_ms/1000;
    timeout.tv_usec = (timeout_ms % 1000)*1000;
    select(nfds, &readfds, NULL, NULL, &timeout);
    if (listen_sock_ >= 0 && FD_ISSET(listen_sock_, &readfds)) {
        /* Handle an inbound connection. */
        sock_ = accept(
                    listen_sock_, (struct sockaddr*) &(address), &address_len);
        inet_ntop(AF_INET, &(address.sin_addr), buffer, 64);
        fprintf(stderr, "OPC: Client connected from %s\n", buffer);
        ::close(listen_sock_);
        listen_sock_ = -1;
        header_length_ = 0;
        payload_length_ = 0;
    } else if (sock_ >= 0 && FD_ISSET(sock_, &readfds)) {
        /* Handle inbound data on an existing connection. */
        if (header_length_ < 4) {  /* need header */
            received = recv(sock_, header_ + header_length_,
                            4 - header_length_, 0);
            if (received > 0) {
                header_length_ += received;
            }
        }
        if (header_length_ == 4) {  /* header complete */
            payload_expected = (header_[2] << 8) | header_[3];
            if (payload_length_ < payload_expected) {  /* need payload */
                received = recv(sock_, payload_ + payload_length_,
                                payload_expected - payload_length_, 0);
                if (received > 0) {
                    payload_length_ += received;
                }
            }
            if (header_length_ == 4 &&
                    payload_length_ == payload_expected) {  /* payload complete */
                if (header_[1] == SET_PIXELS) {
                    handler_(header_[0], payload_expected/3,
                            (Pixel*) payload_);
                }
                header_length_ = 0;
                payload_length_ = 0;
            }
        }
        if (received <= 0) {
            /* Connection was closed; wait for more connections. */
            fprintf(stderr, "OPC: Client closed connection\n");
            ::close(sock_);
            sock_ = -1;
            listen();
        }
    } else {
        /* timeout_ms milliseconds passed with no incoming data or connections. */
        return 0;
    }
    return 1;
}

void Source::close() {
    if (sock_ >= 0) {
        fprintf(stderr, "OPC: Closed connection\n");
        ::close(sock_);
        sock_ = -1;
    }
}

void Source::reset() {
    if (sock_ >= 0) {
        close();
        listen();
    }
}

}
