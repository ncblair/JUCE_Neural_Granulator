#include "Grain.h"

void Grain::prepareToPlay(double sampleRate, SafeBuffer* safe_buf_ptr, 
                        juce::SmoothedValue<float>* env_w, juce::SmoothedValue<float>* env_c) {
    morph_buf_ptr = safe_buf_ptr; //set morph_buf_ptr to the pointer to the SafeBuffer passed in
    sample_rate_ms = sampleRate / 1000.0f;
    env = std::make_shared<TriangularRangeTukey>(env_w, env_c);
    env->setSamplingRate(sampleRate);
}

void Grain::noteStarted(float dur, float start) {
    grain_dur = dur;
    grain_start = start;
    note_on[0] = true;
    note_on[1] = true;
    elapsed_ms[0] = 0.0f;
    elapsed_ms[1] = 0.0f;
    cur_sample[0] = grain_start * morph_buf_ptr->get_num_samples();
    cur_sample[1] = grain_start * morph_buf_ptr->get_num_samples();
    // std::cout << "Grain ON, start sample: " << cur_sample << " total samples " << morph_buf_ptr->get_num_samples() << std::endl;

    // Set up envelope.
    env->set(grain_dur);

}

float Grain::getNextSample(float playback_rate, int c) {
    // Takes in c, the current channel

    if (isActive(c)) {
        auto buf = morph_buf_ptr->load();
        auto read_pointer = buf->getReadPointer(c); // do we move this outside

        // if we reach the end of the buffer, set cur_sample to 0 (circular buffer)
        while (cur_sample[c] > buf->getNumSamples() - 2) {
            cur_sample[c] -= buf->getNumSamples();
        }

        // Get grain envelope value
        const auto env_val = env->get(elapsed_ms[c] / grain_dur); 

        // get grain value (interpolation between points bc playback position is float)
        const auto v1 = int(cur_sample[c]);
        const auto v2 = v1 + 1;
        const auto a = cur_sample[c] - v1;
        const auto result = read_pointer[v1] * (1 - a) + a * read_pointer[v2];

        // calculate what percent of the source sample we've gone through
        elapsed_ms[c] += playback_rate / sample_rate_ms;

        // if we have elapsed grain_dur percent of the grain, turn the note off
        if (elapsed_ms[c] > grain_dur) {
            note_on[c] = false;
            // std::cout << "GRAIN_OFF\n";
        }
       
        cur_sample[c] += playback_rate;

        return result * env_val;
    }
    else {
        return 0.0f;
    }
}

bool Grain::isActive(int c) {
    // return if grain is active on channel c
    return note_on[c];
}
