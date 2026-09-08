#include "readHeader.h"

#include<bits/stdc++.h>

using namespace std;

bool readBytes(FILE *fp, unsigned char *buffer, size_t count)
{
    return fread(buffer, 1, count, fp) == count;
}

uint16_t readLittleEndian16(const unsigned char *buffer)
{
    return static_cast<uint16_t>(buffer[0]) |
           (static_cast<uint16_t>(buffer[1]) << 8);
}

uint32_t readLittleEndian32(const unsigned char *buffer)
{
    return static_cast<uint32_t>(buffer[0]) |
           (static_cast<uint32_t>(buffer[1]) << 8) |
           (static_cast<uint32_t>(buffer[2]) << 16) |
           (static_cast<uint32_t>(buffer[3]) << 24);
}

bool skipBytes(FILE *fp, uint32_t count)
{
    return fseek(fp, static_cast<long>(count), SEEK_CUR) == 0;
}

wavHeader readWavHeader(const std::string &filename)
{
    FILE *fp = fopen(filename.c_str(), "rb");

    if(fp == nullptr)
    {
        cerr<<"Error opening file\n";
        exit(1);
    }

    unsigned char riffHeader[12];
    if(!readBytes(fp, riffHeader, sizeof(riffHeader)) ||
       memcmp(riffHeader, "RIFF", 4) != 0 ||
       memcmp(riffHeader + 8, "WAVE", 4) != 0)
    {
        cerr<<"Invalid RIFF/WAVE file\n";
        fclose(fp);
        exit(1);
    }

    uint32_t sampleRate = 0;
    uint16_t numChannels = 0;
    uint16_t bitsPerSample = 0;
    std::vector<int16_t> pcm_data;
    bool foundFormat = false;
    bool foundData = false;

    while(!foundFormat || !foundData)
    {
        unsigned char chunkHeader[8];
        if(!readBytes(fp, chunkHeader, sizeof(chunkHeader)))
        {
            cerr<<"Missing fmt or data chunk\n";
            fclose(fp);
            exit(1);
        }

        uint32_t chunkSize = readLittleEndian32(chunkHeader + 4);

        if(memcmp(chunkHeader, "fmt ", 4) == 0)
        {
            if(chunkSize < 16)
            {
                cerr<<"Invalid fmt chunk\n";
                fclose(fp);
                exit(1);
            }

            unsigned char formatHeader[16];
            if(!readBytes(fp, formatHeader, sizeof(formatHeader)))
            {
                cerr<<"Error reading fmt chunk\n";
                fclose(fp);
                exit(1);
            }

            uint16_t audioFormat = readLittleEndian16(formatHeader);
            if(audioFormat != 1)
            {
                cerr<<"Only uncompressed PCM WAV files are supported\n";
                fclose(fp);
                exit(1);
            }

            numChannels = readLittleEndian16(formatHeader + 2);
            sampleRate = readLittleEndian32(formatHeader + 4);
            bitsPerSample = readLittleEndian16(formatHeader + 14);
            if (bitsPerSample != 16)
            {
                std::cerr << "Only 16-bit PCM WAV files are supported\n";
                fclose(fp);
                exit(1);
            }
            foundFormat = true;

            if(!skipBytes(fp, chunkSize - 16))
            {
                cerr<<"Error skipping extra fmt data\n";
                fclose(fp);
                exit(1);
            }
        }
        else if(memcmp(chunkHeader, "data", 4) == 0)
        {
            if (!foundFormat || chunkSize % sizeof(int16_t) != 0)
            {
                std::cerr << "Invalid PCM data chunk\n";
                fclose(fp);
                exit(1);
            }

            pcm_data.resize(chunkSize / sizeof(int16_t));
            if (!readBytes(fp,
                           reinterpret_cast<unsigned char *>(pcm_data.data()),
                           chunkSize))
            {
                std::cerr << "Error reading data chunk\n";
                fclose(fp);
                exit(1);
            }
            foundData = true;
        }
        else if(!skipBytes(fp, chunkSize))
        {
            cerr<<"Error skipping WAV chunk\n";
            fclose(fp);
            exit(1);
        }

        if(chunkSize % 2 != 0 && !skipBytes(fp, 1))
        {
            cerr<<"Error skipping WAV padding\n";
            fclose(fp);
            exit(1);
        }
    }

    fclose(fp);

    if(numChannels == 0 || sampleRate == 0 || pcm_data.empty())
    {
        std::cerr << "Invalid WAV audio parameters\n";
        exit(1);
    }

    wavHeader result{};
    std::memcpy(result.riff, "RIFF", sizeof(result.riff));
    result.sampleRate = sampleRate;
    result.numChannels = numChannels;
    result.bitsPerSample = bitsPerSample;
    result.pcm_data = std::move(pcm_data);
    return result;

}
