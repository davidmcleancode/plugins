#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SubOscAudioProcessor::SubOscAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
}

SubOscAudioProcessor::~SubOscAudioProcessor() {}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout SubOscAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    juce::StringArray waveNames { "Sine", "Triangle", "Saw", "Square" };
    juce::StringArray filterTypeNames { "Low Pass", "Band Pass", "High Pass" };

    // --- 3 oscillators -----------------------------------------------------
    // All three start as Saw, all three start at the same level (this is also
    // what alt/option-click on any Level knob resets all three back to).
    struct OscDefault { int wave; int octave; float detune; float level; };
    const OscDefault oscDefaults[3] = {
        { 2, 0,  0.0f, sharedOscLevelDefault },
        { 2, 0,  7.0f, sharedOscLevelDefault },  // slightly detuned for thickness
        { 2, -1, 0.0f, sharedOscLevelDefault }   // one octave down (sub)
    };

    for (int i = 1; i <= 3; ++i)
    {
        auto n = juce::String (i);
        auto& d = oscDefaults[i - 1];

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID ("osc" + n + "Wave", 1), "Osc " + n + " Wave", waveNames, d.wave));

        params.push_back (std::make_unique<juce::AudioParameterInt> (
            juce::ParameterID ("osc" + n + "Octave", 1), "Osc " + n + " Octave", -2, 2, d.octave));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("osc" + n + "Detune", 1), "Osc " + n + " Detune",
            juce::NormalisableRange<float> (-50.0f, 50.0f), d.detune));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("osc" + n + "Level", 1), "Osc " + n + " Level",
            juce::NormalisableRange<float> (0.0f, 1.0f), d.level));
    }

    // --- global controls -----------------------------------------------------
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("glide", 1), "Glide", juce::NormalisableRange<float> (0.0f, 0.4f), 0.0f));

    // Main filter frequency — the shared base that every step's filter mod
    // knob shifts up or down from.
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("filterFreq", 1), "Filter Freq",
        juce::NormalisableRange<float> ((float) freqMin, (float) freqMax, 0.0f, 0.3f), 4000.0f));

    // Global default filter type. Steps each have their own type switch too
    // (so different steps can use different filter types); this global one
    // is what "Clear" resets every step's type back to.
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID ("filterType", 1), "Filter Type", filterTypeNames, 0));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("resonance", 1), "Resonance", juce::NormalisableRange<float> (0.1f, 20.0f), 1.2f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("egAmount", 1), "EG Amount", juce::NormalisableRange<float> (-100.0f, 100.0f), 45.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("attack", 1), "Attack",
        juce::NormalisableRange<float> (0.001f, 2.0f, 0.0f, 0.3f), 0.015f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("decay", 1), "Decay",
        juce::NormalisableRange<float> (0.001f, 2.0f, 0.0f, 0.3f), 0.22f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("sustain", 1), "Sustain", juce::NormalisableRange<float> (0.0f, 1.0f), 0.55f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("release", 1), "Release",
        juce::NormalisableRange<float> (0.001f, 3.0f, 0.0f, 0.3f), 0.35f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("volume", 1), "Master Volume", juce::NormalisableRange<float> (0.0f, 1.0f), 0.65f));

    // Tempo is no longer shown as a knob (the plugin syncs to the host
    // transport), but the parameter stays defined so the Standalone build
    // (which has no host to sync to) still has a sensible fixed tempo to
    // fall back on.
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("bpm", 1), "Tempo", juce::NormalisableRange<float> (60.0f, 200.0f), 120.0f));

    // --- 16 sequencer steps ---------------------------------------------------
    // A simple default pattern so the plugin isn't silent on first load.
    const bool defaultActive[numSteps] = {
        true,false,true,false, true,false,true,false,
        true,false,true,false, true,false,true,false
    };
    const int defaultNote[numSteps] = {
        64,64,67,67, 64,64,67,67, 64,64,67,67, 64,64,67,67
    };

    for (int s = 1; s <= numSteps; ++s)
    {
        auto n = juce::String (s);
        int idx = s - 1;

        params.push_back (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID ("step" + n + "Active", 1), "Step " + n + " Active", defaultActive[idx]));

        params.push_back (std::make_unique<juce::AudioParameterInt> (
            juce::ParameterID ("step" + n + "Note", 1), "Step " + n + " Note", 48, 84, defaultNote[idx]));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("step" + n + "Length", 1), "Step " + n + " Length",
            juce::NormalisableRange<float> (0.0f, 1.4f), 0.65f));

        // Bidirectional: negative = pulls the filter down from the global
        // Filter Freq knob when this step's note starts, positive = pushes
        // it up. 0 (centre) = no shift. Depth is scaled by EG Amount.
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("step" + n + "Freq", 1), "Step " + n + " Filter Mod",
            juce::NormalisableRange<float> (-1.0f, 1.0f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID ("step" + n + "Type", 1), "Step " + n + " Filter Type", filterTypeNames, 0));
    }

    return { params.begin(), params.end() };
}

//==============================================================================
void SubOscAudioProcessor::prepareToPlay (double newSampleRate, int /*samplesPerBlock*/)
{
    sampleRate = newSampleRate;
    stepAccumulatorSamples = 0.0;
    currentStep = 0;
    lastHostStepIndex = -1;
    lastTriggeredFreq = 0.0;

    masterVolumeSmoothed.reset (sampleRate, 0.01);

    for (auto& v : voices)
        v.reset();

    for (int i = 0; i < 3; ++i)
    {
        auto n = juce::String (i + 1);
        oscParams[i].wave   = apvts.getRawParameterValue ("osc" + n + "Wave");
        oscParams[i].octave = apvts.getRawParameterValue ("osc" + n + "Octave");
        oscParams[i].detune = apvts.getRawParameterValue ("osc" + n + "Detune");
        oscParams[i].level  = apvts.getRawParameterValue ("osc" + n + "Level");
    }

    pGlide      = apvts.getRawParameterValue ("glide");
    pFilterFreq = apvts.getRawParameterValue ("filterFreq");
    pFilterType = apvts.getRawParameterValue ("filterType");
    pResonance  = apvts.getRawParameterValue ("resonance");
    pEgAmount   = apvts.getRawParameterValue ("egAmount");
    pAttack     = apvts.getRawParameterValue ("attack");
    pDecay      = apvts.getRawParameterValue ("decay");
    pSustain    = apvts.getRawParameterValue ("sustain");
    pRelease    = apvts.getRawParameterValue ("release");
    pVolume     = apvts.getRawParameterValue ("volume");
    pBpm        = apvts.getRawParameterValue ("bpm");

    for (int s = 0; s < numSteps; ++s)
    {
        auto n = juce::String (s + 1);
        stepParams[s].active  = apvts.getRawParameterValue ("step" + n + "Active");
        stepParams[s].note    = apvts.getRawParameterValue ("step" + n + "Note");
        stepParams[s].length  = apvts.getRawParameterValue ("step" + n + "Length");
        stepParams[s].freqMod = apvts.getRawParameterValue ("step" + n + "Freq");
        stepParams[s].type    = apvts.getRawParameterValue ("step" + n + "Type");
    }
}

void SubOscAudioProcessor::releaseResources() {}

bool SubOscAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

//==============================================================================
Voice* SubOscAudioProcessor::findFreeVoice()
{
    for (auto& v : voices)
        if (! v.active)
            return &v;

    // No free voice: steal the first one (simple, may click occasionally).
    return &voices[0];
}

void SubOscAudioProcessor::triggerStep (int stepIndex, double bpmForTiming)
{
    auto& sp = stepParams[stepIndex];
    bool isActive = sp.active->load() > 0.5f;
    if (! isActive)
        return;

    double lengthFrac = (double) sp.length->load();

    // Length knob at minimum -> the step is silent.
    if (lengthFrac <= 0.001)
        return;

    double secPerStep = 60.0 / bpmForTiming / 4.0; // 16th notes
    double durSeconds = secPerStep * lengthFrac;
    auto durSamples = (int64_t) (durSeconds * sampleRate);

    int noteNumber = (int) std::round (sp.note->load());
    double freqBase = 440.0 * std::pow (2.0, (noteNumber - 69) / 12.0);

    Voice* v = findFreeVoice();
    v->reset();
    v->active = true;

    double glideTime = (double) pGlide->load();
    double startFreq = (glideTime > 0.0 && lastTriggeredFreq > 0.0) ? lastTriggeredFreq : freqBase;
    v->freqGlide.reset (sampleRate, juce::jmax (0.0001, glideTime));
    v->freqGlide.setCurrentAndTargetValue (startFreq);
    v->freqGlide.setTargetValue (freqBase);
    lastTriggeredFreq = freqBase;
    v->freqBase = freqBase;

    for (int o = 0; o < 3; ++o)
    {
        v->waveType[o]    = (int) std::round (oscParams[o].wave->load());
        int octave        = (int) std::round (oscParams[o].octave->load());
        v->octaveMult[o]  = std::pow (2.0, octave);
        v->detuneCents[o] = (double) oscParams[o].detune->load();
        v->level[o]       = (double) oscParams[o].level->load();
        v->phase[o] = 0.0;
    }

    // The frequency this note starts on: the global Filter Freq knob, shifted
    // up or down by this step's (bidirectional) filter mod knob, scaled by
    // how far EG Amount is turned up.
    double egAmt = (double) pEgAmount->load() / 100.0; // -1..1
    double globalFilterFreq = (double) pFilterFreq->load();
    double stepModNorm = (double) sp.freqMod->load(); // -1..1
    const double maxOctaveSpan = 4.0; // full swing at eg=100% and mod=+-1
    double startCutoff = globalFilterFreq * std::pow (2.0, stepModNorm * egAmt * maxOctaveSpan);
    startCutoff = juce::jlimit (30.0, freqMax, startCutoff);

    v->baseCutoff  = startCutoff;
    v->resonanceQ  = (double) pResonance->load();
    v->egAmount    = egAmt;
    v->filterType  = (int) std::round (sp.type->load());

    v->adsrParams.attack  = juce::jmax (0.001f, pAttack->load());
    v->adsrParams.decay   = juce::jmax (0.001f, pDecay->load());
    v->adsrParams.sustain = pSustain->load();
    v->adsrParams.release = juce::jmax (0.001f, pRelease->load());
    v->adsr.setSampleRate (sampleRate);
    v->adsr.setParameters (v->adsrParams);
    v->adsr.noteOn();

    v->samplesRemainingUntilRelease = durSamples;
    v->releaseTriggered = false;
}

//==============================================================================
void SubOscAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    const double modRange = 9000.0;

    // ---- figure out our clock source for this block -----------------------
    // If the host provides a playhead with a valid ppq position (virtually
    // every real DAW, including Reason 12.5+), we lock the sequencer to the
    // host's tempo, play state, and bar position sample-accurately. If not
    // (e.g. running as the Standalone app with no host), we fall back to our
    // own free-running clock driven by the fixed internal tempo and the Play
    // button.
    bool hostSynced = false;
    bool hostIsPlaying = false;
    double hostPpqAtBlockStart = 0.0;
    double effectiveBpm = (double) pBpm->load();

    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (auto ppqOpt = pos->getPpqPosition())
            {
                hostSynced = true;
                hostPpqAtBlockStart = *ppqOpt;
                hostIsPlaying = pos->getIsPlaying();
                if (auto bpmOpt = pos->getBpm())
                    effectiveBpm = *bpmOpt;
            }
        }
    }
    hostSyncActive.store (hostSynced);

    const double ppqPerSample   = (effectiveBpm / 60.0) / sampleRate;
    const double samplesPerStep = (60.0 / effectiveBpm / 4.0) * sampleRate; // 16th notes, free-running fallback only

    masterVolumeSmoothed.setTargetValue (pVolume->load());

    for (int i = 0; i < numSamples; ++i)
    {
        if (hostSynced)
        {
            if (hostIsPlaying)
            {
                double currentPpq = hostPpqAtBlockStart + (double) i * ppqPerSample;
                auto stepIndex64 = (int64_t) std::floor (currentPpq / 0.25); // 0.25 quarter-note = one 16th
                if (stepIndex64 != lastHostStepIndex)
                {
                    lastHostStepIndex = stepIndex64;
                    int col = (int) (((stepIndex64 % numSteps) + numSteps) % numSteps);
                    triggerStep (col, effectiveBpm);
                    uiCurrentStep.store (col);
                }
            }
            else
            {
                // Host is stopped/paused: reset so playback resumes cleanly
                // and re-triggers correctly next time it starts, wherever
                // the playhead has moved to in the meantime.
                lastHostStepIndex = -1;
            }
        }
        else if (sequencerPlaying.load())
        {
            stepAccumulatorSamples += 1.0;
            if (stepAccumulatorSamples >= samplesPerStep)
            {
                stepAccumulatorSamples -= samplesPerStep;
                triggerStep (currentStep, effectiveBpm);
                uiCurrentStep.store (currentStep);
                currentStep = (currentStep + 1) % numSteps;
            }
        }

        float mixSample = 0.0f;

        for (auto& v : voices)
        {
            if (! v.active)
                continue;

            float env = v.adsr.getNextSample();
            if (! v.adsr.isActive())
            {
                v.active = false;
                continue;
            }

            if (! v.releaseTriggered)
            {
                if (v.samplesRemainingUntilRelease <= 0)
                {
                    v.adsr.noteOff();
                    v.releaseTriggered = true;
                }
                else
                {
                    --v.samplesRemainingUntilRelease;
                }
            }

            double freqNow = v.freqGlide.getNextValue();
            double oscSum = 0.0;

            for (int o = 0; o < 3; ++o)
            {
                double detuneMult = std::pow (2.0, v.detuneCents[o] / 1200.0);
                double freq = freqNow * v.octaveMult[o] * detuneMult;

                double ph = v.phase[o];
                double s = 0.0;
                switch (v.waveType[o])
                {
                    case 0: s = std::sin (2.0 * juce::MathConstants<double>::pi * ph); break;
                    case 1: s = 4.0 * std::abs (ph - 0.5) - 1.0; break;
                    case 2: s = 2.0 * ph - 1.0; break;
                    default: s = (ph < 0.5) ? 1.0 : -1.0; break;
                }
                oscSum += s * v.level[o];

                ph += freq / sampleRate;
                if (ph >= 1.0) ph -= 1.0;
                v.phase[o] = ph;
            }

            // The note-start frequency (v.baseCutoff) already includes this
            // step's static filter-mod offset; on top of that, the ADSR
            // envelope continues to breathe the cutoff around that point for
            // the life of the note, using the same EG Amount depth.
            double cutoffNow = v.baseCutoff + v.egAmount * modRange * (double) env;
            cutoffNow = juce::jlimit (30.0, freqMax, cutoffNow);
            float qClamped = (float) juce::jlimit (0.1, 20.0, v.resonanceQ);

            switch (v.filterType)
            {
                case 1:
                    v.filter.coefficients = juce::dsp::IIR::Coefficients<float>::makeBandPass (
                        sampleRate, (float) cutoffNow, qClamped);
                    break;
                case 2:
                    v.filter.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (
                        sampleRate, (float) cutoffNow, qClamped);
                    break;
                default:
                    v.filter.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (
                        sampleRate, (float) cutoffNow, qClamped);
                    break;
            }

            float filtered = v.filter.processSample ((float) oscSum);
            mixSample += filtered * env;
        }

        mixSample *= masterVolumeSmoothed.getNextValue();

        for (int ch = 0; ch < numChannels; ++ch)
            buffer.setSample (ch, i, mixSample);
    }
}

//==============================================================================
void SubOscAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void SubOscAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessorEditor* SubOscAudioProcessor::createEditor()
{
    return new SubOscAudioProcessorEditor (*this);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SubOscAudioProcessor();
}
