#include "PluginEditor.h"

//==============================================================================
KnurledKnobLookAndFeel::KnurledKnobLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (0xff0c0c0b));
    setColour (juce::Label::textColourId, ink);
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff101010));
    setColour (juce::ComboBox::textColourId, ink);
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff2a2a27));
    setColour (juce::TextButton::buttonColourId, juce::Colour (0xff101010));
    setColour (juce::TextButton::textColourOffId, ink);
    setColour (juce::TextButton::textColourOnId, juce::Colour (0xff0c0c0b));
    setColour (juce::ToggleButton::tickColourId, amber);
    setColour (juce::ToggleButton::tickDisabledColourId, inkDim);
}

juce::Label* KnurledKnobLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    return LookAndFeel_V4::createSliderTextBox (slider);
}

void KnurledKnobLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                                float sliderPos, float rotaryStartAngle,
                                                float rotaryEndAngle, juce::Slider&)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (2.0f);
    auto centre = bounds.getCentre();
    auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // --- fixed tick marks around the outside (do not rotate) ---------------
    const int numTicks = 17;
    for (int i = 0; i < numTicks; ++i)
    {
        float t = (float) i / (float) (numTicks - 1);
        float tickAngle = rotaryStartAngle + t * (rotaryEndAngle - rotaryStartAngle);
        bool major = (i == 0 || i == numTicks - 1 || i == numTicks / 2);
        float rInner = radius * (major ? 0.96f : 1.0f);
        float rOuter = radius * 1.16f;

        juce::Point<float> p1 (centre.x + rInner * std::sin (tickAngle), centre.y - rInner * std::cos (tickAngle));
        juce::Point<float> p2 (centre.x + rOuter * std::sin (tickAngle), centre.y - rOuter * std::cos (tickAngle));
        g.setColour (major ? ink : inkDim);
        g.drawLine ({ p1, p2 }, major ? 1.6f : 1.1f);
    }

    // --- knurled outer ring (dark, ridged look) -----------------------------
    auto knobBounds = juce::Rectangle<float>().withSize (radius * 1.62f, radius * 1.62f).withCentre (centre);
    g.setColour (juce::Colour (0xff0a0a0a));
    g.fillEllipse (knobBounds);

    {
        juce::Graphics::ScopedSaveState save (g);
        juce::Path ridgePath;
        ridgePath.addEllipse (knobBounds);
        g.reduceClipRegion (ridgePath);
        const int numRidges = 22;
        for (int i = 0; i < numRidges; ++i)
        {
            float a = (float) i / (float) numRidges * juce::MathConstants<float>::twoPi;
            g.setColour (i % 2 == 0 ? juce::Colour (0xff2c2c2a) : juce::Colour (0xff0a0a0a));
            juce::Path wedge;
            wedge.addPieSegment (knobBounds, a, a + juce::MathConstants<float>::twoPi / numRidges, 0.0f);
            g.fillPath (wedge);
        }
    }

    // --- brushed metal cap, rotates with value ------------------------------
    auto capBounds = juce::Rectangle<float>().withSize (radius * 1.2f, radius * 1.2f).withCentre (centre);
    {
        juce::Graphics::ScopedSaveState save (g);
        g.addTransform (juce::AffineTransform::rotation (angle, centre.x, centre.y));

        juce::ColourGradient grad (juce::Colour (0xfff4f3ee), capBounds.getX(), capBounds.getY(),
                                    juce::Colour (0xff4a4a46), capBounds.getRight(), capBounds.getBottom(), false);
        grad.addColour (0.35, juce::Colour (0xffc9c9c4));
        grad.addColour (0.65, juce::Colour (0xff6b6b66));
        g.setGradientFill (grad);
        g.fillEllipse (capBounds);

        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawEllipse (capBounds, 1.0f);

        // notch
        g.setColour (juce::Colour (0xff0c0c0b));
        float notchW = juce::jmax (2.0f, radius * 0.05f);
        g.fillRoundedRectangle (centre.x - notchW * 0.5f, capBounds.getY() + capBounds.getHeight() * 0.06f,
                                 notchW, capBounds.getHeight() * 0.3f, notchW * 0.5f);
    }
}

//==============================================================================
SubOscAudioProcessorEditor::SubOscAudioProcessorEditor (SubOscAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&knobLnF);

    // All 3 oscillator Level knobs reset together on alt/option-click, back
    // to the same shared default value.
    auto resetAllOscLevels = [this]
    {
        for (int j = 1; j <= 3; ++j)
        {
            auto id = "osc" + juce::String (j) + "Level";
            if (auto* param = processor.apvts.getParameter (id))
                param->setValueNotifyingHost (param->convertTo0to1 (SubOscAudioProcessor::sharedOscLevelDefault));
        }
    };

    const juce::StringArray oscLabels { "OSC 1", "OSC 2", "OSC 3" };
    for (int i = 0; i < 3; ++i)
    {
        auto n = juce::String (i + 1);
        auto& sec = oscSections[(size_t) i];

        sec.waveBox = std::make_unique<juce::ComboBox>();
        sec.waveBox->addItemList ({ "Sine", "Triangle", "Saw", "Square" }, 1);
        addAndMakeVisible (*sec.waveBox);
        sec.waveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            processor.apvts, "osc" + n + "Wave", *sec.waveBox);

        sec.waveLabel = std::make_unique<juce::Label>();
        sec.waveLabel->setText (oscLabels[i], juce::dontSendNotification);
        sec.waveLabel->setJustificationType (juce::Justification::centred);
        sec.waveLabel->setColour (juce::Label::textColourId, knobLnF.ink);
        sec.waveLabel->setFont (juce::Font (13.0f, juce::Font::bold));
        addAndMakeVisible (*sec.waveLabel);

        sec.octave = makeKnob ("osc" + n + "Octave", "OCTAVE");
        sec.detune = makeKnob ("osc" + n + "Detune", "DETUNE");
        sec.level  = makeKnob ("osc" + n + "Level",  "LEVEL", false, resetAllOscLevels);
    }

    glideKnob      = makeKnob ("glide", "GLIDE");
    filterFreqKnob = makeKnob ("filterFreq", "FILTER");
    resonanceKnob  = makeKnob ("resonance", "RESONANCE");
    egAmountKnob   = makeKnob ("egAmount", "EG AMOUNT");
    attackKnob     = makeKnob ("attack", "ATTACK");
    decayKnob      = makeKnob ("decay", "DECAY");
    sustainKnob    = makeKnob ("sustain", "SUSTAIN");
    releaseKnob    = makeKnob ("release", "RELEASE");

    volumeKnob = makeKnob ("volume", "MASTER VOLUME");
    volumeKnob.nameLabel->setFont (juce::Font (8.0f, juce::Font::bold));

    globalFilterTypeSwitch.value = (int) std::round (processor.apvts.getRawParameterValue ("filterType")->load());
    globalFilterTypeSwitch.onChange = [this] (int idx) { setChoiceParam ("filterType", idx); };
    addAndMakeVisible (globalFilterTypeSwitch);
    globalFilterTypeLabel.setText ("TYPE", juce::dontSendNotification);
    globalFilterTypeLabel.setJustificationType (juce::Justification::centred);
    globalFilterTypeLabel.setColour (juce::Label::textColourId, knobLnF.ink);
    globalFilterTypeLabel.setFont (juce::Font (9.0f, juce::Font::bold));
    addAndMakeVisible (globalFilterTypeLabel);

    addAndMakeVisible (playButton);
    playButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff101010));
    playButton.onClick = [this]
    {
        bool newState = ! processor.sequencerPlaying.load();
        processor.sequencerPlaying.store (newState);
        playButton.setButtonText (newState ? "Stop" : "Play");
        playButton.setToggleState (newState, juce::dontSendNotification);
        playButton.setColour (juce::TextButton::buttonColourId,
                               newState ? knobLnF.amber : juce::Colour (0xff101010));
        statusLabel.setText (newState ? "Sequencer running" : "Stopped", juce::dontSendNotification);
    };

    addAndMakeVisible (clearButton);
    clearButton.onClick = [this]
    {
        int defaultType = (int) std::round (processor.apvts.getRawParameterValue ("filterType")->load());
        for (int s = 0; s < SubOscAudioProcessor::numSteps; ++s)
        {
            steps[(size_t) s].activeToggle->setToggleState (false, juce::sendNotification);
            auto id = "step" + juce::String (s + 1) + "Type";
            if (auto* param = processor.apvts.getParameter (id))
                param->setValueNotifyingHost (param->convertTo0to1 ((float) defaultType));
            steps[(size_t) s].typeSwitch->setValueQuiet (defaultType);
        }
    };

    statusLabel.setText ("Click Play to start", juce::dontSendNotification);
    statusLabel.setColour (juce::Label::textColourId, knobLnF.inkDim);
    statusLabel.setFont (juce::Font (11.0f));
    statusLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (statusLabel);

    for (int s = 0; s < SubOscAudioProcessor::numSteps; ++s)
    {
        auto n = juce::String (s + 1);
        auto& st = steps[(size_t) s];

        st.activeToggle = std::make_unique<StepOnButton>();
        addAndMakeVisible (*st.activeToggle);
        st.activeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
            processor.apvts, "step" + n + "Active", *st.activeToggle);

        st.note   = makeKnob ("step" + n + "Note",   "", true);
        st.length = makeKnob ("step" + n + "Length", "", true);
        st.freq   = makeKnob ("step" + n + "Freq",   "", true);
        st.freq.slider->setDoubleClickReturnValue (true, 0.0); // bidirectional: double-click recentres to 0

        st.typeSwitch = std::make_unique<FilterTypeSwitch>();
        st.typeSwitch->value = (int) std::round (processor.apvts.getRawParameterValue ("step" + n + "Type")->load());
        int stepIndexCapture = s;
        st.typeSwitch->onChange = [this, stepIndexCapture] (int idx)
        {
            setChoiceParam ("step" + juce::String (stepIndexCapture + 1) + "Type", idx);
        };
        addAndMakeVisible (*st.typeSwitch);

        st.numberLabel.setText (n, juce::dontSendNotification);
        st.numberLabel.setJustificationType (juce::Justification::centred);
        st.numberLabel.setColour (juce::Label::textColourId, knobLnF.inkDim);
        st.numberLabel.setFont (juce::Font (9.0f));
        addAndMakeVisible (st.numberLabel);
    }

    auto setupRowLabel = [this] (juce::Label& lbl, const juce::String& text)
    {
        lbl.setText (text, juce::dontSendNotification);
        lbl.setJustificationType (juce::Justification::centredRight);
        lbl.setColour (juce::Label::textColourId, knobLnF.ink);
        lbl.setFont (juce::Font (10.0f, juce::Font::bold));
        addAndMakeVisible (lbl);
    };
    setupRowLabel (seqStepsRowLabel,  "STEP");
    setupRowLabel (seqActiveRowLabel, "ON");
    setupRowLabel (seqNoteRowLabel,   "NOTE");
    setupRowLabel (seqLengthRowLabel, "LENGTH");
    setupRowLabel (seqFreqRowLabel,   "FILTER MOD");
    setupRowLabel (seqTypeRowLabel,   "TYPE");

    setSize (1180, 590);
    startTimerHz (30);
}

SubOscAudioProcessorEditor::~SubOscAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void SubOscAudioProcessorEditor::setChoiceParam (const juce::String& paramID, int index)
{
    if (auto* param = processor.apvts.getParameter (paramID))
        param->setValueNotifyingHost (param->convertTo0to1 ((float) index));
}

KnobUnit SubOscAudioProcessorEditor::makeKnob (const juce::String& paramID, const juce::String& label, bool tiny,
                                                std::function<void()> altClickAction)
{
    KnobUnit k;

    if (altClickAction != nullptr)
    {
        auto linkable = std::make_unique<LinkableSlider>();
        linkable->onAltClick = altClickAction;
        k.slider = std::move (linkable);
    }
    else
    {
        k.slider = std::make_unique<juce::Slider>();
    }

    k.slider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider->setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
    k.slider->setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    k.slider->setLookAndFeel (&knobLnF);
    addAndMakeVisible (*k.slider);

    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, paramID, *k.slider);

    if (label.isNotEmpty())
    {
        k.nameLabel = std::make_unique<juce::Label>();
        k.nameLabel->setText (label, juce::dontSendNotification);
        k.nameLabel->setJustificationType (juce::Justification::centred);
        k.nameLabel->setColour (juce::Label::textColourId, knobLnF.ink);
        k.nameLabel->setFont (juce::Font (tiny ? 8.0f : 10.0f, juce::Font::bold));
        addAndMakeVisible (*k.nameLabel);
    }

    return k;
}

void SubOscAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff161615));

    g.setColour (juce::Colour (0xffe9e6da));
    g.setFont (juce::Font (20.0f, juce::Font::bold));
    g.drawText ("SUB // OSC", 16, 8, 400, 26, juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xff8a8779));
    g.setFont (juce::Font (10.0f));
    g.drawText ("3-OSCILLATOR SYNTH + 16-STEP SEQUENCER", 16, 32, 500, 16, juce::Justification::centredLeft);

    g.setColour (juce::Colour (0xff57554d));
    g.drawLine (16.0f, 54.0f, (float) getWidth() - 16.0f, 54.0f, 1.0f);
}

void SubOscAudioProcessorEditor::resized()
{
    auto placeKnob = [] (KnobUnit& k, int x, int y, int w, int h)
    {
        int labelH = k.nameLabel ? 14 : 0;
        if (k.nameLabel)
            k.nameLabel->setBounds (x, y, w, labelH);
        k.slider->setBounds (x, y + labelH, w, h - labelH);
    };

    auto full = getLocalBounds();
    int x0 = 16, y0 = 64;

    // --- top-right: Master Volume ------------------------------------------
    placeKnob (volumeKnob, full.getWidth() - 108, 6, 92, 90);

    // --- 3 oscillator modules --------------------------------------------
    const int oscW = 210, oscH = 150;
    for (int i = 0; i < 3; ++i)
    {
        int x = x0 + i * (oscW + 14);
        oscSections[(size_t) i].waveLabel->setBounds (x, y0, oscW, 18);
        oscSections[(size_t) i].waveBox->setBounds (x, y0 + 20, oscW, 24);

        int kw = oscW / 3;
        int ky = y0 + 54;
        placeKnob (oscSections[(size_t) i].octave, x,          ky, kw, 90);
        placeKnob (oscSections[(size_t) i].detune, x + kw,     ky, kw, 90);
        placeKnob (oscSections[(size_t) i].level,  x + 2 * kw, ky, kw, 90);
    }

    // --- filter / envelope / transport, one row -----------------------------
    int row2Y = y0 + oscH + 12;
    int gx = x0;

    placeKnob (filterFreqKnob, gx, row2Y, 74, 90); gx += 80;

    globalFilterTypeLabel.setBounds (gx, row2Y, 44, 14);
    globalFilterTypeSwitch.setBounds (gx, row2Y + 16, 40, 60);
    gx += 56;

    placeKnob (resonanceKnob, gx, row2Y, 74, 90); gx += 80;
    placeKnob (egAmountKnob,  gx, row2Y, 74, 90); gx += 100;

    placeKnob (attackKnob,  gx, row2Y, 74, 90); gx += 80;
    placeKnob (decayKnob,   gx, row2Y, 74, 90); gx += 80;
    placeKnob (sustainKnob, gx, row2Y, 74, 90); gx += 80;
    placeKnob (releaseKnob, gx, row2Y, 74, 90); gx += 100;

    placeKnob (glideKnob, gx, row2Y, 74, 90); gx += 100;

    playButton.setBounds (gx, row2Y + 30, 70, 28);
    gx += 78;
    clearButton.setBounds (gx, row2Y + 30, 60, 28);
    gx += 68;
    statusLabel.setBounds (gx, row2Y + 34, 180, 20);

    // --- sequencer grid ------------------------------------------------------
    int seqY = row2Y + 130;
    const int labelGutter = 78;
    int seqX0 = x0 + labelGutter;
    int seqW = full.getWidth() - seqX0 - x0;
    float colW = (float) seqW / (float) SubOscAudioProcessor::numSteps;

    seqStepsRowLabel.setBounds  (x0, seqY,       labelGutter - 8, 14);
    seqActiveRowLabel.setBounds (x0, seqY + 16,  labelGutter - 8, 20);
    seqNoteRowLabel.setBounds   (x0, seqY + 42,  labelGutter - 8, 34);
    seqLengthRowLabel.setBounds (x0, seqY + 80,  labelGutter - 8, 34);
    seqFreqRowLabel.setBounds   (x0, seqY + 118, labelGutter - 8, 34);
    seqTypeRowLabel.setBounds   (x0, seqY + 156, labelGutter - 8, 40);

    for (int s = 0; s < SubOscAudioProcessor::numSteps; ++s)
    {
        int cx = seqX0 + (int) (s * colW);
        int cw = (int) colW - 2;
        auto& st = steps[(size_t) s];

        st.numberLabel.setBounds (cx, seqY, cw, 14);
        st.activeToggle->setBounds (cx + cw / 2 - 10, seqY + 16, 20, 20);

        st.note.slider->setBounds   (cx, seqY + 42,  cw, 34);
        st.length.slider->setBounds (cx, seqY + 80,  cw, 34);
        st.freq.slider->setBounds   (cx, seqY + 118, cw, 34);

        int swW = juce::jmin (cw, 34);
        st.typeSwitch->setBounds (cx + (cw - swW) / 2, seqY + 156, swW, 40);
    }
}

void SubOscAudioProcessorEditor::timerCallback()
{
    int cur = processor.uiCurrentStep.load();
    if (cur != lastPaintedStep)
    {
        if (lastPaintedStep >= 0 && lastPaintedStep < (int) steps.size())
        {
            steps[(size_t) lastPaintedStep].activeToggle->playhead = false;
            steps[(size_t) lastPaintedStep].activeToggle->repaint();
        }

        if (cur >= 0 && cur < (int) steps.size())
        {
            steps[(size_t) cur].activeToggle->playhead = true;
            steps[(size_t) cur].activeToggle->repaint();
        }

        lastPaintedStep = cur;
    }

    // Keep the filter-type switches in sync in case their parameters changed
    // externally (host automation, preset recall, etc).
    int globalType = (int) std::round (processor.apvts.getRawParameterValue ("filterType")->load());
    globalFilterTypeSwitch.setValueQuiet (globalType);
    for (int s = 0; s < SubOscAudioProcessor::numSteps; ++s)
    {
        int t = (int) std::round (processor.apvts.getRawParameterValue ("step" + juce::String (s + 1) + "Type")->load());
        steps[(size_t) s].typeSwitch->setValueQuiet (t);
    }

    bool synced = processor.hostSyncActive.load();
    if (synced != lastSyncedState)
    {
        lastSyncedState = synced;
        playButton.setEnabled (! synced);
        if (synced)
        {
            playButton.setButtonText ("Host");
            statusLabel.setText ("Synced to host transport", juce::dontSendNotification);
        }
        else
        {
            playButton.setButtonText (processor.sequencerPlaying.load() ? "Stop" : "Play");
            statusLabel.setText ("Click Play to start", juce::dontSendNotification);
        }
    }
}
