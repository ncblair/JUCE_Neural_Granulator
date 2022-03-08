#include "Grain.h"

void Grain::prepareToPlay(double sampleRate, SafeBuffer* safe_buf_ptr) {
    morph_buf_ptr = safe_buf_ptr; //set morph_buf_ptr to the pointer to the SafeBuffer passed in
    sample_rate_ms = sampleRate / 1000;
    env.setSamplingRate(sampleRate);
}

void Grain::noteStarted(float size, float start) {
    grain_size = size;
    grain_start = start;
    note_on = true;
    elapsed_ms = 0.0f;
    cur_sample = grain_start * morph_buf_ptr->get_num_samples();
    // std::cout << "Grain ON, start sample: " << cur_sample << " total samples " << morph_buf_ptr->get_num_samples() << std::endl;

    // Set up envelope.
    env.set(grain_size / 1000.0);

}

float Grain::getNextSample(float playback_rate) {

    if (isActive()) {
        auto buf = morph_buf_ptr->load();
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
            // std::cout << "GRAIN_OFF\n";
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

