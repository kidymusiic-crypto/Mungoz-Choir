#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

namespace cc
{
constexpr int kMaxSingers = 16;
constexpr int kMaxVoices  = 16;
constexpr int kChunk      = 32;

// Filtro de banda (TPT state-variable), ganho unitário no pico
struct SVFCoefs
{
    float g = 0, k = 1, a1 = 0, a2 = 0, a3 = 0;
    void set (float fc, float q, float sr)
    {
        fc = juce::jmin (fc, sr * 0.45f);
        g  = std::tan (juce::MathConstants<float>::pi * fc / sr);
        k  = 1.0f / q;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
};

struct SVFState
{
    float ic1 = 0, ic2 = 0;
    float process (float x, const SVFCoefs& c) noexcept
    {
        const float v3 = x - ic2;
        const float v1 = c.a1 * ic1 + c.a2 * v3;
        const float v2 = ic2 + c.a2 * ic1 + c.a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        return c.k * v1;
    }
};

struct FormantSet
{
    std::array<SVFCoefs, 3> c;
};

struct FormantFilter
{
    std::array<SVFState, 3> s;
    float process (float x, const FormantSet& f) noexcept
    {
        static constexpr float amp[3] = { 1.0f, 0.55f, 0.3f };
        float y = 0;
        for (int i = 0; i < 3; ++i)
            y += s[(size_t) i].process (x, f.c[(size_t) i]) * amp[i];
        return y;
    }
    void reset() { s = {}; }
};

struct Singer
{
    float phase = 0, vibPhase = 0, vibInc = 0, det = 0, gain = 1;
    float panL = 0.7071f, panR = 0.7071f, inc = 0;
    int delay = 0;
};

struct Voice
{
    bool active = false, held = false, sustainPending = false;
    int note = -1;
    float vel = 0, env = 0, baseFreq = 440.0f;
    std::array<Singer, kMaxSingers> singers;
    std::array<FormantFilter, 2> ff;
};

struct Params
{
    float vowel = 1.6f, detune = 18, spread = 0.8f, humanize = 70, vibrato = 0.3f;
    float breath = 0.15f, crowd = 0.4f, attack = 0.12f, release = 1.2f, tone = 9000;
    float reverb = 0.4f, size = 0.75f, width = 1.1f, gain = 0.5f;
    int singers = 12;
};
} // namespace cc

class ChoirCrowdProcessor : public juce::AudioProcessor
{
public:
    ChoirCrowdProcessor();
    ~ChoirCrowdProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Choir Crowd"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Asake Crowd"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    float get (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    void cacheParams();
    void handleMidi (const juce::MidiMessage&);
    void noteOn (int note, float vel);
    void noteOff (int note);
    void render (float* L, float* R, int n);
    void renderChunk (float* L, float* R, int n);
    static void setFormants (cc::FormantSet&, float vowel, float sr);

    std::array<cc::Voice, cc::kMaxVoices> voices;
    cc::Params p;
    cc::FormantSet formants, crowdFormants;
    std::array<cc::FormantFilter, 2> crowdFF;
    juce::Reverb reverb;
    juce::Random rng;

    double sr = 44100.0;
    float invSr = 1.0f / 44100.0f;
    float attInc = 0.01f, relCoef = 0.999f;
    float vowelSmooth = 1.6f;
    float crowdEnv = 0, crowdMod = 0.5f, crowdTarget = 0.5f;
    int crowdCounter = 0;
    float toneL = 0, toneR = 0;
    bool sustainDown = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChoirCrowdProcessor)
};
