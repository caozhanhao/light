#pragma once

#include "bar.h"
#include "music.h"
#include "error.h"

#include <pulse/simple.h>
#include <pulse/error.h>

#include <mad.h>
#include <string.h>

#include <array>
#include <vector>
#include <string>
#include <memory>
namespace light::audio
{
  struct Data
  {
    //input
    std::shared_ptr<music::Music> music;
    std::array<unsigned char, LIGHT_AUDIO_READ_BUFFER_SIZE> decoder_buffer;
    //pulseaudio
    std::string server;
    pa_simple* s;
    pa_sample_spec ss;
    //input output bar
    bool started;
    //bar
    bool with_bar;
    std::unique_ptr<bar::TimeBar> bar;

    Data() :s(NULL), with_bar(true) {  }
    ~Data()
    {
      if (s)
        pa_simple_free(s);
    }
    void bar_drain() { if (with_bar)bar->drain(); }
  };

  short scale(mad_fixed_t sample)
  {
    sample += (1L << (MAD_F_FRACBITS - 16));
    if (sample >= MAD_F_ONE)
      sample = MAD_F_ONE - 1;
    else if (sample < -MAD_F_ONE)
      sample = -MAD_F_ONE;
    return sample >> (MAD_F_FRACBITS + 1 - 16);
  }
  enum mad_flow output(void* data, struct mad_header const* header, struct mad_pcm* pcm)
  {
    Data* d = (Data*)data;
    unsigned int nchannels, nsamples, i;
    const mad_fixed_t* channel[2];
    std::vector<short> output;
    output.reserve(8192);
    short sample;
    nchannels = pcm->channels;
    nsamples = pcm->length;
    for (i = 0; i < nchannels; i++)
      channel[i] = pcm->samples[i];
    while (nsamples--)
    {
      for (i = 0; i < nchannels; i++)
      {
        sample = scale(*channel[i]++);
        output.emplace_back(sample);
      }
    }
    pa_simple_write(d->s, &output[0], output.size() * sizeof(short), NULL);
    return MAD_FLOW_CONTINUE;
  }
  enum mad_flow input(void* data, struct mad_stream* stream)
  {
    Data* d = (Data*)data;
    if (d->music->eof())
      return MAD_FLOW_STOP;
    std::size_t bytes = 0;
    std::size_t length = 0;
    if (stream->next_frame != NULL)
    {
      bytes = stream->bufend - stream->next_frame;
      memcpy(d->decoder_buffer.data(), stream->next_frame, bytes);
    }
    length = d->music->read
    (d->decoder_buffer.data() + bytes,
      LIGHT_AUDIO_READ_BUFFER_SIZE - bytes);
    if (!d->started && length < 1024)
    {
      throw error::Error(LIGHT_ERROR_LOCATION, __func__,
        "The data is too small. Please check the URL or path");
    }
    mad_stream_buffer(stream, d->decoder_buffer.data(), length + bytes);
    return MAD_FLOW_CONTINUE;
  }
  enum mad_flow header_func(void* data, struct mad_header const* header)
  {
    Data* d = (Data*)data;
    if (!d->started)
    {
      d->started = true;
      int err;
      d->ss = { .format = PA_SAMPLE_S16LE,
        .rate = header->samplerate,
        .channels = 2 };
      if (d->s)
        pa_simple_free(d->s);
      d->s = pa_simple_new(d->server.c_str(),
        "pulseaudio", PA_STREAM_PLAYBACK,
        NULL, "playback", &d->ss, NULL, NULL, &err);
      if (!d->s)
      {
        throw error::Error(LIGHT_ERROR_LOCATION, __func__,
          "Error initing PulseAudio: "
          + std::string(pa_strerror(err)) + "\n");
      }
      if (d->with_bar)
      {
        d->bar = std::make_unique<bar::TimeBar>();
        d->bar->set_time((d->music->size() / 8192) * ((8192 * 8) / (header->bitrate / 1000)));
        d->bar->start();
      }
    }
    return MAD_FLOW_CONTINUE;
  }
  enum mad_flow error(void* data, 
    struct mad_stream* stream,
    struct mad_frame* frame)
  {
    return MAD_FLOW_CONTINUE;
  }
  class Audio
  {
  private:
    Data data;
    bool inited;
  public:
    Audio() : inited(false){ };
    Audio(const Audio& b) = delete;
    void no_bar() { data.with_bar = false; }
    void with_bar() { data.with_bar = true; }
    void init(const std::string& server)
    {
      data.server = server;
      inited = true;
    }
    void play(const std::shared_ptr<music::Music>& m)
    {
      if (!inited)
        default_init();
      data.music = m;
      data.started = false;
      struct mad_decoder decoder;
      mad_decoder_init(&decoder, &data, input, header_func, 0, output, error, 0);
      mad_decoder_options(&decoder, 0);
      mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);
      mad_decoder_finish(&decoder);

      pa_simple_drain(data.s, NULL);
      data.bar_drain();
    }
  private:
    void default_init()
    {
      auto s = getenv("PULSE_SERVER");
      if (s)
        init(getenv("PULSE_SERVER"));
      else
        throw error::Error(LIGHT_ERROR_LOCATION, __func__, "Can not find PULSE_SERVER");
    }
  };
}