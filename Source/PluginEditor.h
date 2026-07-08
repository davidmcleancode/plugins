#pragma once

#include <JuceHeader.h>
#include <functional>
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
// A rotary slider that also supports alt/option-click for a custom "link
// reset" action (used by the 3 oscillator Level knobs, which all reset to
// the same shared value together).
//==============================================================================
class LinkableSlider : public juce::Slider
{
public:
    std::function<void()> onAltClick;

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isAltDown() && onAltClick != nullptr)
        {
            onAltClick();
            return;
        }
        juce::Slider::mouseDown (e);
    }
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
// The per-step "Step On" toggle: a solid lit box when active (stays lit,
// doesn't flicker), plus a bright ring overlay when the sequencer's playhead
// is currently on this step — a different colour from the amber "on" fill so
// the two states are easy to tell apart at a glance.
//==============================================================================
class StepOnButton : public juce::ToggleButton
{
public:
    bool playhead = false;

    void paintButton (juce::Graphics& g, bool /*highlighted*/, bool /*down*/) override
    {
        auto b = getLocalBounds().toFloat().reduced (1.0f);
        bool on = getToggleState();

        g.setColour (on ? juce::Colour (0xffe8a33d) : juce::Colour (0xff101010));
        g.fillRoundedRectangle (b, 3.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawRoundedRectangle (b, 3.0f, 1.0f);

        if (playhead)
        {
            g.setColour (juce::Colour (0xff5ad1e6)); // bright cyan — distinct from the amber "on" fill
            g.drawRoundedRectangle (b.reduced (1.0f), 2.5f, 2.0f);
        }
    }
};

//==============================================================================
// A compact 3-way vertical switch for picking a filter type (Low Pass / Band
// Pass / High Pass). Used both as the global default switch and, one per
// step, in the sequencer grid.
//==============================================================================
class FilterTypeSwitch : public juce::Component
{
public:
    int value = 0; // 0 = LP, 1 = BP, 2 = HP
    std::function<void (int)> onChange;

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        float segH = b.getHeight() / 3.0f;
        static const char* labels[3] = { "L", "B", "H" };
        static const juce::Colour cols[3] = {
            juce::Colour (0xffe8a33d), // LP - amber
            juce::Colour (0xffe35a9e), // BP - pink
            juce::Colour (0xff5aa9e6)  // HP - blue
        };

        for (int i = 0; i < 3; ++i)
        {
            auto seg = juce::Rectangle<float> (0.0f, (float) i * segH, b.getWidth(), segH).reduced (1.0f);
            bool selected = (value == i);
            g.setColour (selected ? cols[i] : juce::Colour (0xff101010));
            g.fillRoundedRectangle (seg, 2.0f);
            g.setColour (selected ? juce::Colours::black : juce::Colour (0xff8a8779));
            g.setFont (juce::Font (9.0f, juce::Font::bold));
            g.drawText (labels[i], seg.toNearestInt(), juce::Justification::centred);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        float segH = (float) getHeight() / 3.0f;
        int idx = juce::jlimit (0, 2, (int) (e.position.y / segH));
        if (idx != value)
        {
            value = idx;
            if (onChange != nullptr)
                onChange (idx);
            repaint();
        }
    }

    void setValueQuiet (int v)
    {
        if (v != value)
        {
            value = v;
            repaint();
        }
    }
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

    KnobUnit makeKnob (const juce::String& paramID, const juce::String& label, bool tiny = false,
                       std::function<void()> altClickAction = nullptr);
    void setChoiceParam (const juce::String& paramID, int index);

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
    KnobUnit volumeKnob;       // "Master Volume" — top-right of the window
    KnobUnit filterFreqKnob;   // global "Filter" dial, left of Resonance

    FilterTypeSwitch globalFilterTypeSwitch;
    juce::Label globalFilterTypeLabel;

    juce::TextButton playButton { "Play" };
    juce::TextButton clearButton { "Clear" };
    juce::Label statusLabel;

    // Per-step controls
    struct StepUI
    {
        std::unique_ptr<StepOnButton> activeToggle;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> activeAttachment;
        KnobUnit note, length, freq;
        std::unique_ptr<FilterTypeSwitch> typeSwitch;
        juce::Label numberLabel;
    };
    std::array<StepUI, SubOscAudioProcessor::numSteps> steps;

    // Row labels for the per-step grid (identify what each row of controls does)
    juce::Label seqStepsRowLabel, seqActiveRowLabel, seqNoteRowLabel, seqLengthRowLabel, seqFreqRowLabel, seqTypeRowLabel;

    int lastPaintedStep = -1;
    bool lastSyncedState = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SubOscAudioProcessorEditor)
};
