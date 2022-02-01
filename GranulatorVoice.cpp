#include "GranulatorVoice.h"
#include <math.h>

// TODO: MOVE TO UTILS FILE
float mtof(float note_number) {
    return pow(2.0f, note_number*0.08333333333f + 3.03135971352f);
}

// TODO: MOVE TO UTILS FILE
float ftom(float hertz) {
    return 12.0f*log2(hertz) - 36.376316562295983618f;
}

void GranulatorVoice::noteStarted() {
    // turn note on and finger down
    note_on = true;
    finger_down = true;

    //set gain based on velocity received
    gain = currentlyPlayingNote.noteOnVelocity.asUnsignedFloat() * 0.25f;

    //turn on amplitude envelope
    env1.noteOn();

    // set the pitch of this midi voice
    notePitchbenChanged();
}

void GranulatorVoice::noteStopped(bool allowTailOff) {
    finger_down = false;
    juce::ignoreUnused (allowTailOff);
}

void GranulatorVoice::notePitchbendChanged() {
    // get how much more room we need in buffer for new note
    auto scale = mtof(60.0f) / float(currentlyPlayingNote.getFrequencyInHertz());
    pitched_samples = int(grain_buffer.getNumSamples() * scale);
    
    if (scale != 1.0f) {
        //repitch grain
        interp.process(scale, 
                    grain_buffer.getReadPointer(0), 
                    note_buffer.getWritePointer(0), 
                    pitched_samples);
    }
    else {
        // if we don't need to scale we can just copy
        pitched_grain_buffer.copyFrom(0, 0, grain_buffer, 0, 0, pitched_samples);
    }
}

void GranulatorVoice::notePressureChanged() {

}

void GranulatorVoice::noteTimbreChanged() {
    
}

void GranulatorVoice::noteKeyStateChanged() {
    
}

void GranulatorVoice::prepareToPlay(double sampleRate, int samplesPerBlock, int outputChannels) {
    // Set up Envelope
    env1.setSampleRate(sampleRate);

    // Set up Buffers
    internal_playback_buffer.setSize(outputChannels, samplesPerBlock * 2); //defensively set to twice samplesPerBlock
    internal_playback_buffer.clear();
    pitched_grain_buffer.setSize(1, juce::roundToInt(sampleRate * 16)); //fit 16 seconds of audio
    pitched_grain_buffer.clear();
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
        if (numSamples > internal_playback_buffer.getNumSamples()) {
            internal_playback_buffer.setSize(internal_playback_buffer.getNumChannels(), (numSamples) * 2);
        }

        // check if we need to trigger a new grain
        if (grain_scheduler.trigger()) {            
            // set grain_index to next free grain using circular buffer
            //if we check every grain and nothing is available end loop
            for (int i = 0; grains[grain_index=(++grain_index==N_GRAINS)?0:grain_index].isActive() && ++i < N_GRAINS + 1;);
            // turn on free grain with params at grain_index
            grains[grain_index].noteStarted(grain_size, grain_scan);
        }

        // Render All Grains [only active ones will actually do anything]
        for (int i = 0; i < N_GRAINS; i++) {
            // call render method on individual grains. this has to be FAST
            grains[i].renderNextBlock(internal_playback_buffer, numSamples); 
        }
        
        //Apply ADSR Envelope
        env1.applyEnvelopeToBuffer(internal_playback_buffer, startSample, numSamples);

        // Add to Output Buffer with Gain Applied
        for (int c = 0; c < outputBuffer.getNumChannels; c++) {
            outputBuffer.addFrom(c, startSample, internal_playback_buffer, c, 0, numSamples, gain);
        }
    }
}

bool GranulatorVoice::isActive() const{
    return note_on;
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
    grain_scan = apvts.getRawParameterValue("GRAIN_SCAN")->load();

    //set granulator parameters
    spray = apvts.getRawParameterValue("SPRAY")->load();
    density = apvts.getRawParameterValue("DENSITY")->load();
    grain_env_type = apvts.getRawParameterValue("GRAIN_ENV_TYPE")->load();
}

//MOVE THIS OUTSIDE TO AN PLUGIN PROCESSOR
// void GranulatorVoice::replace_audio(const at::Tensor& grain, std::mutex& tensor_mutex) {
//     const std::lock_guard<std::mutex> lock(tensor_mutex); // protect the tensor's data while copying
//     for (int channel = 0; channel < grain_buffer.getNumChannels(); ++channel)
//     {
//         grain_buffer.copyFrom(0, 0, tensor.data_ptr<float>(), grain_buffer.getNumSamples());
//     }
// }