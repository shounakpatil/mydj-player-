#pragma once

#include<bits/stdc++.h>

bool readBytes(FILE *fp, unsigned char *buffer, size_t count);
uint16_t readLittleEndian16(const unsigned char *buffer);
uint32_t readLittleEndian32(const unsigned char *buffer);
bool skipBytes(FILE *fp, uint32_t count);

struct wavHeader {
    char riff[4];
    uint32_t sampleRate = 0;
    uint16_t numChannels = 0;
    uint16_t bitsPerSample = 0;
    std::vector<int16_t> pcm_data;
};

wavHeader readWavHeader(const std::string &filename);