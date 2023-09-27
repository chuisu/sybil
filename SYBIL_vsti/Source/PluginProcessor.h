/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class SYBIL_vstiAudioProcessor  : public juce::AudioProcessor, public juce::AudioIODeviceCallback
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:

    //==============================================================================
    SYBIL_vstiAudioProcessor();
    virtual ~SYBIL_vstiAudioProcessor() override;
    //==============================================================================

    void startSYBIL();
    void stopSYBIL();

    void audioDeviceIOCallback(const float** inputChannelData, int numInputChannels, float** outputChannelData, int numOutputChannels, int numSamples);
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    void analyzeAudio(const float** inputChannelData, int numInputChannels, int numSamples);
    void makePrediction();

    juce::StringArray getInputDeviceNames();
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    
    juce::AudioProcessorValueTreeState state;


private:
    //==============================================================================
    juce::UndoManager undoManager;
    juce::AudioDeviceManager audioDeviceManager;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SYBIL_vstiAudioProcessor)
};
