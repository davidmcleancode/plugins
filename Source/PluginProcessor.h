#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>

//==============================================================================
// A single one-shot polyphonic voice. Every parameter it needs (oscillator
// waveform/octave/detune/level, resonance, EG amount, ADSR times, and this
// particular step's filter cutoff) is captured once at trigger time, exactly
// like the original playNote() in the web version — a voice never reacts to
// knob movements after it has started, only *new* voices do.
//==============================================================================
struct Voice
{
    bool active = false;

    // Oscillator phases (0..1) for the 3 oscillators
    double phase[3] { 0.0, 0.0, 0.0 };

    // Captured per-oscillator parameters
    int    waveType[3]   { 0, 0, 0 };   // 0 sine, 1 triangle, 2 saw, 3 square
    double octaveMult[3] { 1.0, 1.0, 1.0 };
    double detuneCents[3]{ 0.0, 0.0, 0.0 };
    double level[3]      { 0.0, 0.0, 0.0 };

    double freqBase = 440.0;                 // fundamental frequency (Hz)
    juce::SmoothedValue<double> freqGlide;   // portamento from the previous note

    double baseCutoff = 8000.0;
    double resonanceQ = 1.0;
    double egAmount   = 0.0;                 // -1..1

    juce::ADSR adsr;
    juce::ADSR::Parameters adsrParams;

    juce::dsp::IIR::Filter<float> filter;

    int64_t samplesRemainingUntilRelease = 0; // when to call adsr.noteOff()
    bool releaseTriggered = false;

    void reset()
    {
        active = false;
        adsr.reset();
        filter.reset();
    }
};

//==============================================================================
class SubOscAudioProcessor final : public juce::AudioProcessor
{
public:
    SubOscAudioProcessor();
    ~SubOscAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SubOsc"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static constexpr int numSteps = 16;
    static constexpr int numVoices = 8;
    static constexpr double freqMin = 20.0;
    static constexpr double freqMax = 16000.0;

    juce::AudioProcessorValueTreeState apvts;

    // Transport control for the built-in sequencer. Set from the editor.
    std::atomic<bool> sequencerPlaying { false };
    std::atomic<int>  uiCurrentStep    { -1 }; // for the editor's playhead LED

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void triggerStep (int stepIndex);
    Voice* findFreeVoice();

    std::array<Voice, numVoices> voices;

    double sampleRate = 44100.0;
    double stepAccumulatorSamples = 0.0;
    int currentStep = 0;
    double lastTriggeredFreq = 0.0;

    // cached atomic parameter pointers, fetched once in prepareToPlay
    struct OscParams
    {
        std::atomic<float>* wave = nullptr;
        std::atomic<float>* octave = nullptr;
        std::atomic<float>* detune = nullptr;
        std::atomic<float>* level = nullptr;
    };
    std::array<OscParams, 3> oscParams;

    std::atomic<float>* pGlide = nullptr;
    std::atomic<float>* pResonance = nullptr;
    std::atomic<float>* pEgAmount = nullptr;
    std::atomic<float>* pAttack = nullptr;
    std::atomic<float>* pDecay = nullptr;
    std::atomic<float>* pSustain = nullptr;
    std::atomic<float>* pRelease = nullptr;
    std::atomic<float>* pVolume = nullptr;
    std::atomic<float>* pBpm = nullptr;

    struct StepParams
    {
        std::atomic<float>* active = nullptr;
        std::atomic<float>* note = nullptr;
        std::atomic<float>* length = nullptr;
        std::atomic<float>* freq = nullptr;
    };
    std::array<StepParams, numSteps> stepParams;

    juce::SmoothedValue<float> masterVolumeSmoothed;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SubOscAudioProcessor)
};
