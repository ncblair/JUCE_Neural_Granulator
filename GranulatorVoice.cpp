#include "GranulatorVoice.h"
#include <math.h>

float mtof(float note_number) {
    return pow(2.0f, note_number*0.08333333333f + 3.03135971352f);
}

float ftom(float hertz) {
    return 12.0f*log2(hertz) - 36.376316562295983618f;
}

void GranulatorVoice::noteStarted() {
    note_on = true;
    finger_down = true;
}

void GranulatorVoice::noteStopped(bool allowTailOff) {
    finger_down = false;
    juce::ignoreUnused (allowTailOff);
}

void GranulatorVoice::notePitchbendChanged() {

}

void GranulatorVoice::notePressureChanged() {

}

void GranulatorVoice::noteTimbreChanged() {
    
}

void GranulatorVoice::noteKeyStateChanged() {
    
}

void GranulatorVoice::prepareToPlay(double sampleRate, int samplesPerBlock, int outputChannels) {
    env1.setSampleRate(sampleRate);
    juce::ignoreUnused (samplesPerBlock, outputChannels);
}

void GranulatorVoice::setCurrentSampleRate (double newRate) {
    if (currentSampleRate != newRate)
    {
        currentSampleRate = newRate;
    }
}

void GranulatorVoice::renderNextBlock (juce::AudioBuffer< float > &outputBuffer, int startSample, int numSamples) {
    if (isActive()) {

    }
}

bool GranulatorVoice::isActive() const{
    return note_on;
}

void GranulatorVoice::updateParameters(juce::AudioProcessorValueTreeState& apvts) {

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