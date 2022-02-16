#include "Grain.h"

void Grain::noteStarted(float size, float scan) {
    grain_size = size;
    grain_scan = scan;
    note_on = true;
    percent_elapsed = 0.0f;
    cur_samples = int(grain_scan * numSamplesInSourceBuffer);
}

void Grain::prepareToPlay(double sampleRate, juce::AudioBuffer<float>* grain) {
    source = grain;
    // grain_env.setSampleRate(sampleRate);

    // // TODO: make this a parameter UI element and add this to NoteStarted
    // grain_env.setParameters(juce::ADSR::Parameters(
    //     0.005,
    //     0.0,
    //     1.0,
    //     0.005)
    // );
}

float getNextSample(int numSamplesInSourceBuffer) {
    // only thing that will change during this callback is numSamplesInSourceBuffer. 
    // Once grain size is set, it's set

    // TODO: Make sure if numSamplesInSourceBuffer changes, 
    // we stay in the right part of the grain in terms of percentage

    if isActive() {
        read_pointer = source.getReadPointer(0); // do we move this outside

        // if we reach the end of the buffer, set cur_sample to 0 (circular buffer)
        if (cur_sample >= numSamplesInSourceBuffer) {
            cur_sample = 0;
        }
        // calculate what percent of the source sample we've gone through
        percent_elapsed += 1.0f / float(numSamplesInSourceBuffer);
        // if we have elapsed grain_size percent of the grain, turn the note off
        if (percent_elapsed > grain_size) {
            note_on = false;
        }
        // TODO: multiply by grain envelope
        return read_pointer[cur_sample++];
    }
    else {
        return 0.0f;
    }
}

bool isActive() {
    return note_on;
}

