#include "GranulatorVoice.h"

void GranulatorVoice::noteStarted() {
    // turn note on and finger down
    note_on = true;
    finger_down = true;

    //set gain based on velocity received
    gain = currentlyPlayingNote.noteOnVelocity.asUnsignedFloat() * 0.25f;

    //turn on amplitude envelope
    env1.noteOn();

    // set the pitch of this midi voice
    notePitchbendChanged();

    // reset note trigger
    trigger_helper = 0;
    spray_factor = 0.0f;

    // set grain envelope params to the current value
    grain_env_width.setCurrentAndTargetValue (grain_env_width.getTargetValue());
    grain_env_center.setCurrentAndTargetValue (grain_env_center.getTargetValue());
}

void GranulatorVoice::noteStopped(bool allowTailOff) {
    finger_down = false;
    env1.noteOff();
    juce::ignoreUnused (allowTailOff);
}

void GranulatorVoice::notePitchbendChanged() {
    playback_rate = float(currentlyPlayingNote.getFrequencyInHertz()) / mtof(60.0f);
    // // load current grain
    // auto grain_buffer = processorRef.grain_buffer_ptr_atomic.load();
    // // get how much more room we need in buffer for new note
    // auto scale = mtof(60.0f) / float(currentlyPlayingNote.getFrequencyInHertz());
    
    // pitched_samples = int(grain_buffer->getNumSamples() * scale);
    
    // if (scale != 1.0f) {
    //     //repitch grain
    //     interp.process(scale, 
    //                 grain_buffer->getReadPointer(0), 
    //                 pitched_grain_buffer.getWritePointer(0), 
    //                 pitched_samples);
    // }
    // else {
    //     // if we don't need to scale we can just copy
    //     pitched_grain_buffer.copyFrom(0, 0, *grain_buffer, 0, 0, pitched_samples);
    // }
}

void GranulatorVoice::notePressureChanged() {

}

void GranulatorVoice::noteTimbreChanged() {
    
}

void GranulatorVoice::noteKeyStateChanged() {
    
}

void GranulatorVoice::prepareToPlay(double sampleRate, int samplesPerBlock, int outputChannels, AudioPluginAudioProcessor* p) {
    // Set up grain buffer
    grains.resize(N_GRAINS);

    // Set up Envelope
    env1.setSampleRate(sampleRate);

    // Set up Buffers
    internal_playback_buffer.setSize(outputChannels, samplesPerBlock * 2); //defensively set to twice samplesPerBlock
    internal_playback_buffer.clear();
    // pitched_grain_buffer.setSize(1, juce::roundToInt(sampleRate * 16)); //fit 16 seconds of audio
    // pitched_grain_buffer.clear();

    //Set up Processor Ref
    processor_ptr = p;


    for (int i = 0; i < N_GRAINS; ++i) {
        grains[i].prepareToPlay(sampleRate, &(processor_ptr->morph_buf), &grain_env_width, &grain_env_center);
    }

    grain_env_width.reset(sampleRate, 0.005);
    grain_env_center.reset(sampleRate, 0.005);
}

void GranulatorVoice::setCurrentSampleRate (double newRate) {
    if (currentSampleRate != newRate)
    {
        currentSampleRate = newRate;
    }
}

void GranulatorVoice::renderNextBlock (juce::AudioBuffer< float > &outputBuffer, int startSample, int numSamples) {
    if (isActive()) {
        //check if we need to increase the size of our playback buffer (shouldn't happen hopefully)
        while (numSamples > internal_playback_buffer.getNumSamples()) {
            std::cout << "Internal Playback Buffer Resized" << std::endl;;
            internal_playback_buffer.setSize(internal_playback_buffer.getNumChannels(), numSamples * 2);
            
        }

        internal_playback_buffer.clear();
        
        for (int c = 0; c < internal_playback_buffer.getNumChannels(); ++c) {
            write_pointers[c] = internal_playback_buffer.getWritePointer(c);
        }
        
        

        // We will process everything one sample at a time (we might need to trigger a new grain at a certain sample)
        for (int i = 0; i < numSamples; ++i) {
             // check if we need to trigger a new grain
            if (trigger()) {            
                // set grain_index to next free grain using circular buffer
                //if we check every grain and nothing is available end loop
                for (int j = 0; grains[grain_index=(++grain_index==N_GRAINS)?0:grain_index].isActive() && ++j < N_GRAINS + 1;);
                // turn on free grain with params at grain_index
                grains[grain_index].noteStarted(grain_size, grain_start);
            }

            // Render All Grains [only active ones will actually do anything]
            for (int c = 0; c < internal_playback_buffer.getNumChannels(); ++c) {
                for (int j = 0; j < N_GRAINS; j++) {
                    // update grain env params
                    grain_env_center.getNextValue();
                    grain_env_width.getNextValue();
                    // call render method on individual grains. this has to be FAST
                    write_pointers[c][i] += grains[j].getNextSample(playback_rate, c);
                }
            }
        }

        //Apply ADSR Envelope
        env1.applyEnvelopeToBuffer(internal_playback_buffer, 0, numSamples);

        // Add to Output Buffer with Gain Applied
        for (int c = 0; c < outputBuffer.getNumChannels(); c++) {
            outputBuffer.addFrom(c, startSample, internal_playback_buffer, c, 0, numSamples, gain);
        }


        // if we reached the end of our envelope, turn the note off
        if (!env1.isActive()) {
            note_on = false;
        }
    }
}

bool GranulatorVoice::isActive() const{
    return note_on;
}

bool GranulatorVoice::trigger() {
    // trigger approximately ~density~ times per second
    // spray_factor is initialized to 0 so we will always trigger the first time this is called
    // spray_factor is then set to a random float between 0 and 2 that determines how soon the
    //  next grain arrives
    trigger_helper += density;
    if (trigger_helper > processor_ptr->getSampleRate() * spray_factor) {
        // std::cout << "trigger " << std::endl;
        trigger_helper = 0.0;
        spray_factor = 1.0f + spray * (random.nextFloat()*2.0f - 1);
        return true;
    }
    return false;
}

void GranulatorVoice::update_parameters(juce::AudioProcessorValueTreeState& apvts) {

    //Set env1 ADSR Parameters
    env1.setParameters(juce::ADSR::Parameters(
        apvts.getRawParameterValue("ENV1_ATTACK")->load(),
        apvts.getRawParameterValue("ENV1_DECAY")->load(),
        apvts.getRawParameterValue("ENV1_SUSTAIN")->load(),
        apvts.getRawParameterValue("ENV1_RELEASE")->load())
    );
    // set grain parameters
    grain_size = apvts.getRawParameterValue("GRAIN_SIZE")->load();
    grain_start = apvts.getRawParameterValue("GRAIN_SCAN")->load();

    //set granulator parameters
    spray = apvts.getRawParameterValue("SPRAY")->load();
    density = apvts.getRawParameterValue("DENSITY")->load();
    // grain_env_type = apvts.getRawParameterValue("GRAIN_ENV_TYPE")->load();

    // update grain env parameters;
    grain_env_center.setTargetValue(apvts.getRawParameterValue("GRAIN_ENV_CENTER")->load());
    grain_env_width.setTargetValue(apvts.getRawParameterValue("GRAIN_ENV_WIDTH")->load());

}