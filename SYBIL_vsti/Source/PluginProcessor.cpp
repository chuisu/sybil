/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <iostream>
#include <fstream>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <essentia/algorithmfactory.h>
#include <essentia/essentiamath.h>
#include <essentia/pool.h>

//==============================================================================
SYBIL_vstiAudioProcessor::SYBIL_vstiAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),

#endif
{
    audioDeviceManager.initialise(2, 2, nullptr, true);
}

SYBIL_vstiAudioProcessor::~SYBIL_vstiAudioProcessor() {
}

//==============================================================================
const juce::String SYBIL_vstiAudioProcessor::getName() const {
    return JucePlugin_Name;
}

bool SYBIL_vstiAudioProcessor::acceptsMidi() const {
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SYBIL_vstiAudioProcessor::producesMidi() const {
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SYBIL_vstiAudioProcessor::isMidiEffect() const {
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SYBIL_vstiAudioProcessor::getTailLengthSeconds() const {
    return 0.0;
}

int SYBIL_vstiAudioProcessor::getNumPrograms() {
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SYBIL_vstiAudioProcessor::getCurrentProgram() {
    return 0;
}

void SYBIL_vstiAudioProcessor::setCurrentProgram (int index) {
}

const juce::String SYBIL_vstiAudioProcessor::getProgramName (int index) {
    return {};
}

void SYBIL_vstiAudioProcessor::changeProgramName (int index, const juce::String& newName) {
}

//==============================================================================

juce::StringArray SYBIL_vstiAudioProcessor::getInputDeviceNames()
{
    juce::StringArray deviceNames;

    if (auto* currentAudioDeviceType = audioDeviceManager.getCurrentDeviceTypeObject())
    {
        deviceNames = currentAudioDeviceType->getDeviceNames(true); // true for input devices
    }
    else
    {
        deviceNames.add("none found");
    }

    return deviceNames;
}

void SYBIL_vstiAudioProcessor::startSYBIL() {
    juce::Logger::writeToLog("we have liftoff!");
    // Initialization code here
}

void SYBIL_vstiAudioProcessor::audioDeviceAboutToStart(juce::AudioIODevice* device) {
    // We need to better understand this audioDeviceAboutToStart
}

void SYBIL_vstiAudioProcessor::audioDeviceStopped() {
    // What does this function do?
}

void SYBIL_vstiAudioProcessor::audioDeviceIOCallback(const float** inputChannelData, int numInputChannels, float** outputChannelData, int numOutputChannels, int numSamples) {
    analyzeAudio(inputChannelData, numInputChannels, numSamples);
    // Handle output if necessary
}

void SYBIL_vstiAudioProcessor::analyzeAudio(const float** inputChannelData, int numInputChannels, int numSamples) {
    // Analyze audio, estimate BPM, adjust window size, etc.
    // You could call makePrediction() from here or elsewhere depending on how you want to structure it
}

void SYBIL_vstiAudioProcessor::makePrediction() {
    // Make predictions in Hz using 'SYBIL.h5'
}
void SYBIL_vstiAudioProcessor::stopSYBIL() {
    juce::Logger::writeToLog("stopping her!");
    audioDeviceManager.removeAudioCallback(this);
    // Additional cleanup code
}

//==============================================================================
void SYBIL_vstiAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void SYBIL_vstiAudioProcessor::releaseResources() {
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SYBIL_vstiAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const {
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SYBIL_vstiAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
        /* auto* channelData = buffer.getWritePointer (channel); */

        // ..do something to the data...
    }
}

//==============================================================================
bool SYBIL_vstiAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SYBIL_vstiAudioProcessor::createEditor() {
    return new SYBIL_vstiAudioProcessorEditor (*this);
}

//==============================================================================
void SYBIL_vstiAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SYBIL_vstiAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}



//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SYBIL_vstiAudioProcessor();
}
