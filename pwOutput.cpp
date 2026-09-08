#include "pwOutput.h"
#include "readHeader.h"
#include<bits/stdc++.h>
#include <utility>
#include <spa/param/audio/format-utils.h>
#include <pipewire/pipewire.h>

#define M_PI_M2 (M_PI + M_PI)

#define DEFAULT_RATE 44100
#define DEFAULT_VOLUME 0.7

static void on_process(void *userdata)
{
    auto *data = static_cast<AppData *>(userdata);
    struct pw_buffer *b;
    struct spa_buffer *buf;
    size_t n_frames;
    const size_t stride = sizeof(int16_t) * data->channels;
    int16_t *dst;

    if ((b = pw_stream_dequeue_buffer(data->stream)) == nullptr)
    {
        pw_log_warn("out of buffers: %m");
        return;
    }

    buf = b->buffer;
    if ((dst = static_cast<int16_t *>(buf->datas[0].data)) == nullptr)
        return;

    n_frames = buf->datas[0].maxsize / stride;
    if (b->requested)
        n_frames = std::min(static_cast<size_t>(b->requested), n_frames);

    const size_t total_frames = data->pcm_buffer.size() / data->channels;
    const size_t frames_available = total_frames - data->current_frame_index;
    const size_t frames_to_read = std::min(n_frames, frames_available);

    if (frames_to_read)
    {
        std::memcpy(dst,
                    &data->pcm_buffer[data->current_frame_index * data->channels],
                    frames_to_read * stride);
        data->current_frame_index += frames_to_read;
    }

    if (frames_to_read < n_frames) {
        int silence_frames = n_frames - frames_to_read;
        std::memset(reinterpret_cast<uint8_t*>(dst) + (frames_to_read * stride), 
                    0, 
                    silence_frames * stride);
    }

    buf->datas[0].chunk->offset = 0;
    buf->datas[0].chunk->stride = static_cast<int32_t>(stride);
    buf->datas[0].chunk->size = static_cast<uint32_t>(n_frames * stride);

    pw_stream_queue_buffer(data->stream, b);

    if (frames_to_read < n_frames)
        pw_main_loop_quit(data->loop);
}

void playWav(struct wavHeader header)
{
    AppData app_data{};
    app_data.channels = header.numChannels;
    app_data.pcm_buffer = std::move(header.pcm_data);
    const struct spa_pod *params[1];
    uint8_t buffer[1024];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

    pw_init(nullptr, nullptr);

    app_data.loop = pw_main_loop_new(nullptr);

    struct pw_stream_events stream_events = {};
    stream_events.version = PW_VERSION_STREAM_EVENTS;
    stream_events.process = on_process;

    app_data.stream = pw_stream_new_simple(
        pw_main_loop_get_loop(app_data.loop),
        "audio-src",
        pw_properties_new(
            PW_KEY_MEDIA_TYPE, "Audio",
            PW_KEY_MEDIA_CATEGORY, "Playback",
            PW_KEY_MEDIA_ROLE, "Music",
            nullptr),
        &stream_events,
        &app_data);

    struct spa_audio_info_raw info = {};
    info.format = SPA_AUDIO_FORMAT_S16;
    info.channels = header.numChannels;
    info.rate = header.sampleRate;

    params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &info);

    pw_stream_connect(app_data.stream,
                      PW_DIRECTION_OUTPUT,
                      PW_ID_ANY,
                      static_cast<pw_stream_flags>(
                          PW_STREAM_FLAG_AUTOCONNECT |
                          PW_STREAM_FLAG_MAP_BUFFERS |
                          PW_STREAM_FLAG_RT_PROCESS),
                      params, 1);

    pw_main_loop_run(app_data.loop);

    pw_stream_destroy(app_data.stream);
    pw_main_loop_destroy(app_data.loop);
}
