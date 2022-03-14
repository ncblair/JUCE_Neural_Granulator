#include "utils.h"

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

/**** triangle range tukey Class Implementation ****/

TriangularRangeTukey::TriangularRangeTukey(juce::SmoothedValue<float>* w, juce::SmoothedValue<float>* c) : width(w), center(c){
}

void TriangularRangeTukey::set(float seconds) {
    total_samples = seconds * mSamplingRate;
    percent_elapsed = 0.0f;
}

float TriangularRangeTukey::step(float playback_rate) {
    float x;
    float c = center->getCurrentValue(); // c in 0 to 1
    float w = width->getCurrentValue(); // w in -1 to 1
    
    if (percent_elapsed < c) {
        x = percent_elapsed / (2.0f * c);
    }
    else {
        x = 0.5f + (percent_elapsed - c) / ((1.0f - c) * 2.0f);
    }

    percent_elapsed += playback_rate / total_samples;
    while (percent_elapsed >= 1.0f) {
        percent_elapsed -= 1.0f;
    }

    if (w > 0.0f) {
		w = (1.0f - w) / 2.0f;
		if (x < w){
			return 0.5f * (1.0f + juce::dsp::FastMathApproximations::cos(M_PI * (-1.0f + x/w)));
        }
		else if (x > 1.0f - w){
			return 0.5f * (1.0f + juce::dsp::FastMathApproximations::cos(M_PI * (-1.0f/w + 1.0f + x/w)));
        }
		else{
            return 1.0f;
        }
    }
	else{
		return std::pow(juce::dsp::FastMathApproximations::sin(M_PI * x), (-10.0f * w + 2.0f));
    }
}

/**
 * Fast Midi To Frequency Function
 */
float mtof(const float note_number) {
    return pow(2.0f, note_number*0.08333333333f + 3.03135971352f);
}

/**
 * Fast Frequency To Midi Function
 */
float ftom(const float hertz) {
    return 12.0f*log2(hertz) - 36.376316562295983618f;
}

/**** SafeBuffer Class Implementation ****/

SafeBuffer::SafeBuffer(int channels, int samples) {
    //set up buffers
    buf_1.setSize(num_channels, num_samples);
    buf_2.setSize(num_channels, num_samples);
    buf_1.clear();
    buf_2.clear();

    cur_buf_ptr_atomic.store(&buf_1);
    temp_buf_ptr_atomic.store(&buf_2);

    num_channels.store(channels);
    num_samples.store(samples);
}

juce::AudioBuffer<float>* SafeBuffer::load() {
    // load a pointer to the current buffer
    return cur_buf_ptr_atomic.load();
}

int SafeBuffer::get_num_samples() {
    // get number of samples
    return num_samples.load();
}

int SafeBuffer::get_num_channels() {
    // get number of channels
    return num_channels.load();
}

void SafeBuffer::queue_new_buffer(juce::AudioBuffer<float>* new_buffer) {
    // called from background thread or editor thread
    while (ready_to_update.load());
    temp_buf_ptr_atomic.store(new_buffer);
    ready_to_update.store(true);
}

void SafeBuffer::update() {
    // If a new buffer is ready, switch the pointers atomically
    if (ready_to_update.load()) {
        auto temp_ptr = temp_buf_ptr_atomic.load();
        auto cur_ptr = cur_buf_ptr_atomic.load();
        cur_buf_ptr_atomic.store(temp_ptr);
        temp_buf_ptr_atomic.store(cur_ptr);
        
        // in this case, temp_ptr stores the buffer we are gonna use (the buffer that was queued)
        num_samples.store(temp_ptr->getNumSamples());
        num_channels.store(temp_ptr->getNumChannels());

        ready_to_update.store(false);
    }
}

// void SafeBuffer::set_size(int channels, int samples) {
//     buf_1.setSize(channels, samples);
//     buf_2.setSize(channels, samples);
// }