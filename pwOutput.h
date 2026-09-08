#pragma once


#include<bits/stdc++.h>

struct wavHeader;
struct pw_main_loop;
struct pw_stream;

struct AppData {
    struct pw_main_loop *loop;
    struct pw_stream *stream;
    uint16_t channels;
    std::vector<int16_t> pcm_buffer;
    size_t current_frame_index;
};

void playWav(struct wavHeader header);