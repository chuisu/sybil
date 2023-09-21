/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class SYBIL_vstiAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    SYBIL_vstiAudioProcessorEditor (SYBIL_vstiAudioProcessor&);
    ~SYBIL_vstiAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SYBIL_vstiAudioProcessor& audioProcessor;
    juce::TextButton mainToggleButton;

    juce::ComboBox audioDeviceSelector;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> audioDeviceSelectorAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SYBIL_vstiAudioProcessorEditor)
};
