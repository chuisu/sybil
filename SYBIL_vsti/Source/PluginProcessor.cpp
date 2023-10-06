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
#include <tensorflow/core/public/session.h>
#include <tensorflow/core/platform/env.h>

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
                       )

#endif
{
    audioDeviceManager.initialise(2, 2, nullptr, true);
    essentia::init();
    bpmPointer = &bpm;
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
    isPredicting = true;
}

void SYBIL_vstiAudioProcessor::stopSYBIL() {
    juce::Logger::writeToLog("stopping her!");
    isPredicting = false;  // set the flag off to stop prediction
    audioDeviceManager.removeAudioCallback(this);
    // Additional cleanup code
}

void SYBIL_vstiAudioProcessor::audioDeviceAboutToStart(juce::AudioIODevice* device) {
    sampleRate = device->getCurrentSampleRate();
}

void SYBIL_vstiAudioProcessor::audioDeviceStopped() {
    sampleRate = 0;
}

void SYBIL_vstiAudioProcessor::audioDeviceIOCallback(const float** inputChannelData, int numInputChannels, float** outputChannelData, int numOutputChannels, int numSamples) {

}

float SYBIL_vstiAudioProcessor::detectBPM(std::vector<float>& audioBuffer) {
    using namespace essentia;
    using namespace essentia::standard;
        // Create a pool to store the results
    Pool pool;

    // Algorithm factory instance
    AlgorithmFactory& factory = AlgorithmFactory::instance();

    // Create the RhythmExtractor2013 algorithm
    Algorithm* rhythmExtractor = factory.create("RhythmExtractor2013");

    // Declare separate variables for the output
    float bpm;
    std::vector<float> ticks;
    float confidence;
    std::vector<float> estimates;
    std::vector<float> bpmIntervals;

    // Input and output setup
    rhythmExtractor->input("signal").set(audioBuffer);
    rhythmExtractor->output("bpm").set(bpm);
    rhythmExtractor->output("ticks").set(ticks);
    rhythmExtractor->output("confidence").set(confidence);
    rhythmExtractor->output("estimates").set(estimates);
    rhythmExtractor->output("bpmIntervals").set(bpmIntervals);


    // Compute BPM
    rhythmExtractor->compute();

    // Store the output in the pool
    pool.add("bpm", bpm);

    // Clean-up, if needed (Depends on how often you plan to use the algorithm)
    //delete rhythmExtractor;

    return bpm;
}

std::vector<float> SYBIL_vstiAudioProcessor::computeHPCPs(std::vector<float>& audioData) {
    using namespace essentia;
    using namespace essentia::standard;
    // pad the audio in case it's odd
    int size = audioData.size();
        if (size % 2 != 0) {
        audioData.push_back(0); // zero-padding
    }

    AlgorithmFactory& factory = AlgorithmFactory::instance();

    // Declare algorithms
    Algorithm* window = factory.create("Windowing",
                                   "type", "hann"); // you can choose other types like "hamming", "blackman", etc.
    Algorithm* fft = factory.create("FFT");
    Algorithm* spectralPeaks = factory.create("SpectralPeaks");
    Algorithm* hpcp = factory.create("HPCP");

    // Input and output vectors
    std::vector<std::complex<float>> fftOutput;
    std::vector<float> frequenciesVector;
    std::vector<float> magnitudesVector;
    std::vector<float> windowedFrame;

    window->input("frame").set(audioData);
    window->output("frame").set(windowedFrame);
    window->compute();

    // Configure and connect algorithms
    fft->input("frame").set(windowedFrame); // Assuming audioFrame is a frame of audio data
    fft->output("fft").set(fftOutput);

    spectralPeaks->input("spectrum").set(windowedFrame);
    spectralPeaks->output("frequencies").set(frequenciesVector);
    spectralPeaks->output("magnitudes").set(magnitudesVector);

    // Compute FFT and Spectral Peaks
    fft->compute();
    spectralPeaks->compute();
    // ... configure the HPCP algorithm if necessary ...

    // Input & output setup
    std::vector<float> hpcpValues;
    hpcp->input("frequencies").set(frequenciesVector);
    hpcp->input("magnitudes").set(magnitudesVector);
    hpcp->output("hpcp").set(hpcpValues);

    // Compute
    hpcp->compute();

    // Cleanup if necessary
    delete hpcp;

    return hpcpValues;
}

void SYBIL_vstiAudioProcessor::predictNote() {
    tensorflow::Session* session;
    tensorflow::Status status = tensorflow::NewSession(tensorflow::SessionOptions(), &session);
    if (!status.ok()) {
        std::cerr << status.ToString() << std::endl;
    } else {
        std::cout << "TensorFlow session created successfully." << std::endl;
    }
}

void SYBIL_vstiAudioProcessor::loadTFModel(const std::string& modelPath) {
    tensorflow::Status status = tensorflow::NewSession(tensorflow::SessionOptions(), &session);
    if (!status.ok()) {
        // Handle error
    }

    tensorflow::GraphDef graph_def;
    status = tensorflow::ReadBinaryProto(tensorflow::Env::Default(), modelPath, &graph_def);
    if (!status.ok()) {
        // Handle error
    }

    status = session->Create(graph_def);
    if (!status.ok()) {
        // Handle error
    }
}

//==============================================================================
void SYBIL_vstiAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    // Assuming you want to allocate 5 seconds of audio buffer. Multiply sampleRate by 5 and by the number of channels.
    ringBuffer.setSize(2, sampleRate * 5.0);

    // Initialize write position and counter
    writePos = 0;
    bpmCounter = 0;
    hpcpCounter = 0;
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

void SYBIL_vstiAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    int newWritePos = (writePos + buffer.getNumSamples()) % ringBuffer.getNumSamples();

    for (int channel = 0; channel < totalNumOutputChannels; ++channel) {
        if (writePos + buffer.getNumSamples() <= ringBuffer.getNumSamples()) {
            ringBuffer.copyFrom(channel, writePos, buffer, channel, 0, buffer.getNumSamples());
        } else {
            const int samplesToEnd = ringBuffer.getNumSamples() - writePos;
            const int samplesToBeginning = buffer.getNumSamples() - samplesToEnd;
            ringBuffer.copyFrom(channel, writePos, buffer, channel, 0, samplesToEnd);
            ringBuffer.copyFrom(channel, 0, buffer, channel, samplesToEnd, samplesToBeginning);
        }
    }
    writePos = newWritePos;

    bpmCounter += buffer.getNumSamples();
    if (bpmCounter >= getSampleRate()) {
        bpmCounter = 0;
        std::vector<float> audioData(ringBuffer.getWritePointer(0), ringBuffer.getWritePointer(0) + ringBuffer.getNumSamples());
        bpmThread = std::thread(&SYBIL_vstiAudioProcessor::detectBPMThreaded, this, audioData);
        bpmThread.detach();
    }

    float currentBPM;
    {
        std::lock_guard<std::mutex> lock(bpmMutex);
        currentBPM = *bpmPointer;
    }

    int samplesPerBeat = static_cast<int>(getSampleRate() * 60.0 / currentBPM);
    int samplesPerSixteenth = samplesPerBeat / 4;

    hpcpCounter += buffer.getNumSamples();
    if (hpcpCounter >= samplesPerSixteenth) {
        hpcpCounter = 0;
        std::vector<float> audioDataForHPCP(ringBuffer.getWritePointer(0), ringBuffer.getWritePointer(0) + samplesPerSixteenth);
        std::vector<float> hpcpValues = computeHPCPs(audioDataForHPCP);

        for (float value : hpcpValues) {
            std::cout << value << " ";
        }
        std::cout << std::endl;
    }
}

void SYBIL_vstiAudioProcessor::detectBPMThreaded(std::vector<float> audioData)
{
    // Lock the mutex while performing BPM detection
    std::lock_guard<std::mutex> lock(bpmMutex);

    // Your existing detectBPM code
    bpm = detectBPM(audioData);
    std::cout << "bpm = " << bpm << std::endl;
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

juce::AudioProcessorValueTreeState::ParameterLayout
    SYBIL_vstiAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // If we need more params, we can add them here

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SYBIL_vstiAudioProcessor();
}
