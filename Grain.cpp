#include "Grain.h"

void Grain::prepareToPlay(double sampleRate, std::atomic<juce::AudioBuffer<float>*>* buf_atomic_ptr_arg) {
    this->buf_atomic_ptr = buf_atomic_ptr_arg;
    sample_rate_ms = sampleRate / 1000;
    env.setSamplingRate(sampleRate);
}

void Grain::noteStarted(float size, float start) {
    grain_size = size;
    grain_start = start;
    note_on = true;
    elapsed_ms = 0.0f;
    cur_sample = grain_start * buf_atomic_ptr->load()->getNumSamples();
    std::cout << "Grain ON, start sample: " << cur_sample << " total samples " << buf_atomic_ptr->load()->getNumSamples() << std::endl;

    // Set up envelope.
    env.set(grain_size / 1000.0);

}

float Grain::getNextSample(float playback_rate) {

    if (isActive()) {
        auto buf = buf_atomic_ptr->load();
        auto read_pointer = buf->getReadPointer(0); // do we move this outside

        // if we reach the end of the buffer, set cur_sample to 0 (circular buffer)
        while (cur_sample > buf->getNumSamples() - 2) {
            std::cout << "WRAP " << playback_rate << std::endl;
            cur_sample -= buf->getNumSamples();
        }
        // calculate what percent of the source sample we've gone through
        elapsed_ms += playback_rate / sample_rate_ms;

        // if we have elapsed grain_size percent of the grain, turn the note off
        if (elapsed_ms > grain_size) {
            note_on = false;
            elapsed_ms = 0.0f;
            std::cout << "GRAIN_OFF\n";
        }

        // TODO: multiply by grain envelope
        const auto v1 = int(cur_sample);
        const auto v2 = int(cur_sample) + 1;
        const auto a = cur_sample - v1;
        const auto result = read_pointer[v1] * (1 - a) + a * read_pointer[v2];
        // const auto env_value = env_read_pointer[v1] * (1 - a) + a * env_read_pointer[v2];
        const auto env_val = env(playback_rate); 
        cur_sample += playback_rate;
        // std::cout << "cur sample " << cur_sample << "result " << result << std::endl;
        return result * env_val;
    }
    else {
        return 0.0f;
    }
}

bool Grain::isActive() {
    return note_on;
}


/**** tukey Class Implementation ****/

float tukey::operator()(float playback_rate) {
    if (currentS < (alpha_totalS) / 2) {
        value = 0.5 * (1 + juce::dsp::FastMathApproximations::cos((2 * currentS / (alpha_totalS) - 1) * M_PI));
        currentS += playback_rate;
    } else if (currentS <= totalS * (1 - alpha / 2)) {
        value = 1;
        currentS += playback_rate;
    } else if (currentS <= totalS) {
        value =
            0.5 *
            (1 +
            juce::dsp::FastMathApproximations::cos((2 * currentS / (alpha_totalS) - (2 / alpha) + 1) * M_PI));
        currentS += playback_rate;
    } else
        currentS = 0;
    return value;
}

void tukey::set() {
    if (totalS <= 0)
        totalS = 1;
    currentS = 0;
    value = 0;
    alpha_totalS = totalS * alpha;
}

void tukey::set(float seconds, float alpha) {
    this->alpha = alpha;
    totalS = seconds * mSamplingRate;
    set();
}

void tukey::set(float seconds) {
    totalS = seconds * mSamplingRate;
    set();
}

