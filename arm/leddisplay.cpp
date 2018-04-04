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

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <iostream>
#include <sys/mman.h>
#include <assert.h>
#include "leddisplay.h"

using namespace std;

LedDisplay::LedDisplay() {
    openSharedMem();
}

LedDisplay::~LedDisplay() {
    closeSharedMem();
}

uint8_t* LedDisplay::getFrameBuffer() {
    return frame_buffer_;
}

void LedDisplay::clear() {
    memset(frame_buffer_, 0, frame_buffer_size);
    commitFrameBuffer();
}

void LedDisplay::commitFrameBuffer() {
    remapBits();
    swapPruBuffers();
    remapBits();
    swapPruBuffers();
}

void LedDisplay::swapPruBuffers() {
    current_buffer = !current_buffer;
    // Wait for both PRU to be ready for the next frame
    while (*flag_pru_[0] == 0 || *flag_pru_[1] == 0) {usleep(10);}
    *flag_pru_[0] = 0;
    *flag_pru_[1] = 0;
}

void LedDisplay::setBrightness(const float brightness, bool dithering) {
    lookup_table_ = ColorLookupTable(brightness, dithering);
}

void LedDisplay::openSharedMem(){
    mem_fd_ = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd_ < 0) {
        printf("Failed to open /dev/mem (%s)\n", strerror(errno));
    }
    assert(mem_fd_ >= 0);

    // Address of the PRUs shared memory in a am335 soc
    shared_memory_ = static_cast<uint8_t*>(mmap(0, 0x00003000, PROT_WRITE | PROT_READ, MAP_SHARED, mem_fd_, 0x4A300000+0x00010000));
    if (shared_memory_ == NULL) {
        printf("Failed to map the device (%s)\n", strerror(errno));
        close(mem_fd_);
    }
    assert(shared_memory_ != NULL);

    //memset(shared_memory_, 0, 12*1024); // reset shared memory

    flag_pru_[0] = &shared_memory_[0];
    flag_pru_[1] = &shared_memory_[1];
    frame_[0]    = &shared_memory_[2];
    frame_[1]    = &shared_memory_[2+frame_buffer_size];

}

void LedDisplay::closeSharedMem(){
    close(mem_fd_);
}

void LedDisplay::remapBits() {
    uint8_t* frame = frame_[current_buffer];
    uint8_t tmp1[frame_buffer_size];
    uint8_t tmp2[frame_buffer_size];
    uint8_t p;

    memset(tmp2, 0, frame_buffer_size);
    memcpy(tmp1, frame_buffer_, frame_buffer_size);

    // RGB to GRB, gamma correction and brightness adjustment
    for (auto i=0;i<frame_buffer_size;i+=3) {
        p = tmp1[i];
        tmp1[i]   = lookup_table_[current_buffer][tmp1[i+1]];
        tmp1[i+1] = lookup_table_[current_buffer][p];
        tmp1[i+2] = lookup_table_[current_buffer][tmp1[i+2]];
    }

    // Every two lines of the display is wired upside-down
    for (auto i=bytes_per_strip/2;i<frame_buffer_size;i+=bytes_per_strip) {
        for (auto j=0;j<bytes_per_strip/4;j+=3) {
            p = tmp1[i+j];
            tmp1[i+j] = tmp1[i+bytes_per_strip/2-1-j-2];
            tmp1[i+bytes_per_strip/2-1-j-2] = p;

            p = tmp1[i+j+1];
            tmp1[i+j+1] = tmp1[i+bytes_per_strip/2-1-j-1];
            tmp1[i+bytes_per_strip/2-1-j-1] = p;

            p = tmp1[i+j+2];
            tmp1[i+j+2] = tmp1[i+bytes_per_strip/2-1-j];
            tmp1[i+bytes_per_strip/2-1-j] = p;
        }
    }

    // We reorder the bits of the buffer for the
    // serial to parallel shift registers
    {

        uint32_t m = 0, n = 0;
        uint32_t *tmp1r = reinterpret_cast<uint32_t*>(tmp1);

        for (auto l=0,ll=0;l<frame_buffer_size/4;l+=frame_buffer_size/8,ll+=frame_buffer_size/2) {
            for (auto i=0,ii=0;i<bytes_per_strip/4;i++,ii+=32) {
                for (auto j=0;j<8;j++) {
                    m = 0;
                    for (auto k=0,kk=0;k<8;k++,kk+=bytes_per_strip/4) {
                        n = tmp1r[l+i+kk];
                        m |= ( ( ( n >>(7-j)) & 0x01010101 ) << (7-k) );
                    }
                    tmp2[ll+ii+j]    = (m >> 0) & 0xFF;
                    tmp2[ll+ii+8+j]  = (m >> 8) & 0xFF;
                    tmp2[ll+ii+16+j] = (m >> 16) & 0xFF;
                    tmp2[ll+ii+24+j] = (m >> 24) & 0xFF;
                }
            }
        }
    }

    memcpy(frame, tmp2, frame_buffer_size);

}

void LedDisplay::printFrameRate() {
    static bool b = false;
    static timespec ts;
    static int frame_count = 0;
    static long start = 0, end = 0;

    if (!b) {
        b = true;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        start = ts.tv_sec;
    }

    clock_gettime(CLOCK_MONOTONIC, &ts);
    end = ts.tv_sec;
    frame_count++;

    if (end - start > 0) {
        start = end;
        printf("FPS: %d\n",frame_count);
        frame_count = 0;
    }
}
