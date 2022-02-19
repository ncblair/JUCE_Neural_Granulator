#include "Grain.h"

void Grain::prepareToPlay(double sampleRate, std::atomic<juce::AudioBuffer<float>*>buf_atomic) {
    this->buf_atomic = buf_atomic;
    sample_rate_ms = sampleRate / 1000;
}

void Grain::noteStarted(float size, float start) {
    grain_size = size;
    grain_start = start;
    note_on = true;
    elapsed_ms = 0.0f;
    cur_sample = grain_start * buf->getNumSamples();
}

float getNextSample(float playback_rate) {

    if isActive() {
        auto buf = buf_atomic.load();
        read_pointer = buf.getReadPointer(0); // do we move this outside

        // if we reach the end of the buffer, set cur_sample to 0 (circular buffer)
        while (cur_sample > buf->getNumSamples() - 2) {
            cur_sample -= buf->getNumSamples();
        }
        // calculate what percent of the source sample we've gone through
        elapesed_ms += playback_rate / sample_rate_ms;

        // if we have elapsed grain_size percent of the grain, turn the note off
        if (elapsed_ms > grain_size) {
            note_on = false;
        }

        // TODO: multiply by grain envelope
        const auto v1 = int(cur_sample);
        const auto v2 = int(cur_sample) + 1;
        const auto a = cur_sample - v1;
        const auto result = read_pointer[v1] * (1 - a) + a * read_pointer[v2];
        cur_sample += playback_rate;
        return result;
    }
    else {
        return 0.0f;
    }
}

bool isActive() {
    return note_on;
}

