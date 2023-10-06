/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <juce_dsp/juce_dsp.h>
#include <thread>
#include <mutex>
#include <tensorflow/core/public/session.h>
#include <tensorflow/core/platform/env.h>
#include "tensorflow/cc/saved_model/loader.h"
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

    // void audioDeviceIOCallback(const float** inputChannelData, int numInputChannels, float** outputChannelData, int numOutputChannels, int numSamples);
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    float detectBPM(std::vector<float>& audioBuffer);
    void detectBPMThreaded(std::vector<float> audioData);
    std::vector<float> computeHPCPs(std::vector<float>& audioData);
    float predictNote(const std::vector<float>& hpcpValues);

    juce::StringArray getInputDeviceNames();
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void loadTFModel(const std::string& modelPath);


    juce::AudioBuffer<float> ringBuffer;

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

    juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout();
    juce::AudioProcessorValueTreeState SYBILValueTreeState {*this, nullptr, "Parameters", createParameterLayout()};


private:
    //==============================================================================
    //juce::AudioDeviceManager audioDeviceManager;

    juce::dsp::Oscillator<float> sineOsc;
    std::thread bpmThread; // Thread for BPM detection
    std::mutex bpmMutex;   // Mutex for protecting shared data
    std::condition_variable bpmCondVar;
    bool shouldExit = false; // A flag to signal the thread to terminate
    bool hasNewData = false;  // A flag to signal the thread that new data is available
    std::vector<float> threadAudioData;  // Buffer for data that should be processed by the thread
    float bpm = 120.0;
    float* bpmPointer;
    bool isPredicting = false;

    tensorflow::Session* session;
    tensorflow::SavedModelBundle bundle;
    tensorflow::Status status;
    juce::File binaryLocation = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    juce::File modelFile = binaryLocation.getSiblingFile("sybil_model/saved_model.pb");
    juce::File modelDir = modelFile.getParentDirectory();

    int bpmCounter = 0;
    int hpcpCounter = 0;
    int writePos = 0;
    double sampleRate = 0.0;
    const int bpmThreshold = 258;
    juce::UndoManager undoManager;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SYBIL_vstiAudioProcessor)
};
