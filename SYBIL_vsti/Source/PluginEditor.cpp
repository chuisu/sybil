/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SYBIL_vstiAudioProcessorEditor::SYBIL_vstiAudioProcessorEditor (SYBIL_vstiAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);

    audioDeviceSelector.addItemList (static_cast<SYBIL_vstiAudioProcessor&>(processor).getInputDeviceNames(), 1);
    audioDeviceSelector.setSelectedId(1);
    addAndMakeVisible (&audioDeviceSelector);

    SYBIL_vstiAudioProcessor* sybilProcessor = dynamic_cast<SYBIL_vstiAudioProcessor*>(&processor);
    if (sybilProcessor != nullptr)
    {
        audioDeviceSelectorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(sybilProcessor->state, "AudioInputDevices", audioDeviceSelector);
    }

    addAndMakeVisible(mainToggleButton);
    mainToggleButton.setButtonText("START");
    mainToggleButton.onClick = [this] {
        if (mainToggleButton.getButtonText() == "START") {
            mainToggleButton.setButtonText("STOP");
            audioProcessor.startSYBIL(); // Calls a method in the processor to start the synth
        } else {
            mainToggleButton.setButtonText("START");
            audioProcessor.stopSYBIL(); // Calls a method in the processor to stop the synth
        }
    };

    // Set button size and position

}

SYBIL_vstiAudioProcessorEditor::~SYBIL_vstiAudioProcessorEditor()
{
}

//==============================================================================
void SYBIL_vstiAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Hi! I'm SYBIL, and I'm always listening!", getLocalBounds(), juce::Justification::topLeft, 1);
}

void SYBIL_vstiAudioProcessorEditor::resized()
{
    audioDeviceSelector.setBounds (10, 20, 150, 20);
    mainToggleButton.setBounds(10, 260, 100, 30);
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
