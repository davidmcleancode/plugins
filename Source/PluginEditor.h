#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// A rotary knob styled after a knurled-edge hardware knob with a brushed-metal
// cap: dark ridged outer ring, a lighter metal disc on top that rotates with
// the value, and a small notch line so you can see its position at a glance.
//==============================================================================
class KnurledKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    KnurledKnobLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    juce::Label* createSliderTextBox (juce::Slider&) override;

    juce::Colour amber   { 0xffe8a33d };
    juce::Colour ink     { 0xffe9e6da };
    juce::Colour inkDim  { 0xff8a8779 };
    juce::Colour panel2  { 0xff1c1c1b };
};

//==============================================================================
// One knob + its text label, wired to an APVTS parameter.
//==============================================================================
struct KnobUnit
{
    std::unique_ptr<juce::Slider> slider;
    std::unique_ptr<juce::Label> nameLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

//==============================================================================
class SubOscAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    explicit SubOscAudioProcessorEditor (SubOscAudioProcessor&);
    ~SubOscAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    KnobUnit makeKnob (const juce::String& paramID, const juce::String& label, bool tiny = false);

    SubOscAudioProcessor& processor;
    KnurledKnobLookAndFeel knobLnF;

    // Oscillator sections (wave combo box + octave/detune/level knobs) x3
    struct OscSection
    {
        std::unique_ptr<juce::ComboBox> waveBox;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveAttachment;
        std::unique_ptr<juce::Label> waveLabel;
        KnobUnit octave, detune, level;
    };
    std::array<OscSection, 3> oscSections;

    KnobUnit glideKnob, resonanceKnob, egAmountKnob;
    KnobUnit attackKnob, decayKnob, sustainKnob, releaseKnob;
    KnobUnit volumeKnob, bpmKnob;

    juce::TextButton playButton { "Play" };
    juce::TextButton clearButton { "Clear" };
    juce::Label statusLabel;

    // Per-step controls
    struct StepUI
    {
        std::unique_ptr<juce::ToggleButton> activeToggle;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> activeAttachment;
        KnobUnit note, length, freq;
        juce::Label numberLabel;
    };
    std::array<StepUI, SubOscAudioProcessor::numSteps> steps;

    int lastPaintedStep = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SubOscAudioProcessorEditor)
};
