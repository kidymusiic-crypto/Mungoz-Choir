#include "PluginProcessor.h"

using namespace cc;

static constexpr float kTwoPi = juce::MathConstants<float>::twoPi;

static float polyBlep (float t, float dt) noexcept
{
    if (t < dt)         { t /= dt; return t + t - t * t - 1.0f; }
    if (t > 1.0f - dt)  { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
    return 0.0f;
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout ChoirCrowdProcessor::createLayout()
{
    using F = juce::AudioParameterFloat;
    using R = juce::NormalisableRange<float>;
    juce::AudioProcessorValueTreeState::ParameterLayout l;

    l.add (std::make_unique<F> (juce::ParameterID { "vowel", 1 },   "Vowel (Oo-Oh-Ah-Eh-Ee)", R (0.0f, 4.0f, 0.01f), 1.6f));
    l.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { "singers", 1 }, "Singers", 1, kMaxSingers, 12));
    l.add (std::make_unique<F> (juce::ParameterID { "detune", 1 },  "Detune (cents)", R (0.0f, 50.0f, 0.1f), 18.0f));
    l.add (std::make_unique<F> (juce::ParameterID { "spread", 1 },  "Stereo Spread", R (0.0f, 1.0f, 0.01f), 0.8f));
    l.add (std::make_unique<F> (juce::ParameterID { "human", 1 },   "Humanize (ms)", R (0.0f, 150.0f, 1.0f), 70.0f));
    l.add (std::make_unique<F> (juce::ParameterID { "vibrato", 1 }, "Vibrato", R (0.0f, 1.0f, 0.01f), 0.3f));
    l.add (std::make_unique<F> (juce::ParameterID { "breath", 1 },  "Breath", R (0.0f, 1.0f, 0.01f), 0.15f));
    l.add (std::make_unique<F> (juce::ParameterID { "crowd", 1 },   "Crowd", R (0.0f, 1.0f, 0.01f), 0.4f));
    l.add (std::make_unique<F> (juce::ParameterID { "attack", 1 },  "Attack (s)", R (0.005f, 2.0f, 0.001f, 0.4f), 0.12f));
    l.add (std::make_unique<F> (juce::ParameterID { "release", 1 }, "Release (s)", R (0.05f, 6.0f, 0.01f, 0.4f), 1.2f));
    l.add (std::make_unique<F> (juce::ParameterID { "tone", 1 },    "Tone (Hz)", R (1000.0f, 18000.0f, 1.0f, 0.4f), 9000.0f));
    l.add (std::make_unique<F> (juce::ParameterID { "reverb", 1 },  "Reverb", R (0.0f, 1.0f, 0.01f), 0.4f));
    l.add (std::make_unique<F> (juce::ParameterID { "size", 1 },    "Room Size", R (0.0f, 1.0f, 0.01f), 0.75f));
    l.add (std::make_unique<F> (juce::ParameterID { "width", 1 },   "Width", R (0.0f, 1.5f, 0.01f), 1.1f));
    l.add (std::make_unique<F> (juce::ParameterID { "gain", 1 },    "Gain (dB)", R (-24.0f, 6.0f, 0.1f), -3.0f));
    return l;
}

ChoirCrowdProcessor::ChoirCrowdProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

bool ChoirCrowdProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void ChoirCrowdProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    invSr = (float) (1.0 / sampleRate);
    reverb.setSampleRate (sampleRate);
    reverb.reset();
    for (auto& v : voices) v = Voice();
    for (auto& f : crowdFF) f.reset();
    crowdEnv = 0; toneL = toneR = 0; crowdCounter = 0;
    cacheParams();
    vowelSmooth = p.vowel;
}

void ChoirCrowdProcessor::cacheParams()
{
    p.vowel    = get ("vowel");
    p.singers  = juce::jlimit (1, kMaxSingers, (int) std::lround (get ("singers")));
    p.detune   = get ("detune");
    p.spread   = get ("spread");
    p.humanize = get ("human");
    p.vibrato  = get ("vibrato");
    p.breath   = get ("breath");
    p.crowd    = get ("crowd");
    p.attack   = get ("attack");
    p.release  = get ("release");
    p.tone     = get ("tone");
    p.reverb   = get ("reverb");
    p.size     = get ("size");
    p.width    = get ("width");
    p.gain     = juce::Decibels::decibelsToGain (get ("gain"));

    attInc  = 1.0f / juce::jmax (1.0f, p.attack * (float) sr);
    relCoef = std::exp (-6.9078f / juce::jmax (1.0f, p.release * (float) sr));
}

//==============================================================================
void ChoirCrowdProcessor::setFormants (FormantSet& fs, float vowel, float sampleRate)
{
    // Oo, Oh, Ah, Eh, Ee  (F1, F2, F3)
    static constexpr float V[5][3] = { { 300, 870, 2240 }, { 450, 800, 2830 }, { 800, 1150, 2900 },
                                       { 530, 1840, 2480 }, { 270, 2290, 3010 } };
    static constexpr float Q[3] = { 6.0f, 10.0f, 14.0f };

    vowel = juce::jlimit (0.0f, 4.0f, vowel);
    const int i = juce::jlimit (0, 3, (int) std::floor (vowel));
    const float f = vowel - (float) i;
    for (int j = 0; j < 3; ++j)
        fs.c[(size_t) j].set (V[i][j] + (V[i + 1][j] - V[i][j]) * f, Q[j], sampleRate);
}

void ChoirCrowdProcessor::noteOn (int note, float vel)
{
    Voice* v = nullptr;
    for (auto& x : voices) if (x.active && x.note == note) { v = &x; break; }
    if (v == nullptr) for (auto& x : voices) if (! x.active) { v = &x; break; }
    if (v == nullptr)
    {
        v = &voices[0];
        for (auto& x : voices) if (x.env < v->env) v = &x;
    }

    v->active = true; v->held = true; v->sustainPending = false;
    v->note = note; v->vel = 0.35f + 0.65f * vel; v->env = 0;
    v->baseFreq = 440.0f * std::exp2 ((float) (note - 69) / 12.0f);
    v->ff[0].reset(); v->ff[1].reset();

    for (auto& s : v->singers)
    {
        s.phase    = rng.nextFloat();
        s.vibPhase = rng.nextFloat() * kTwoPi;
        s.vibInc   = kTwoPi * (4.2f + rng.nextFloat() * 2.8f) * invSr;
        s.det      = rng.nextFloat() * 2.0f - 1.0f;
        s.gain     = 0.6f + 0.4f * rng.nextFloat();
        const float pan = (rng.nextFloat() * 2.0f - 1.0f) * p.spread;
        const float ang = (pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
        s.panL = std::cos (ang);
        s.panR = std::sin (ang);
        s.delay = (int) (rng.nextFloat() * p.humanize * 0.001f * (float) sr);
    }
}

void ChoirCrowdProcessor::noteOff (int note)
{
    for (auto& v : voices)
        if (v.active && v.held && v.note == note)
        {
            if (sustainDown) v.sustainPending = true;
            else v.held = false;
        }
}

void ChoirCrowdProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())            noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())      noteOff (m.getNoteNumber());
    else if (m.isAllNotesOff() || m.isAllSoundOff())
        for (auto& v : voices) v.held = false;
    else if (m.isController() && m.getControllerNumber() == 64)
    {
        sustainDown = m.getControllerValue() >= 64;
        if (! sustainDown)
            for (auto& v : voices)
                if (v.sustainPending) { v.held = false; v.sustainPending = false; }
    }
}

//==============================================================================
void ChoirCrowdProcessor::render (float* L, float* R, int n)
{
    while (n > 0)
    {
        const int c = juce::jmin (kChunk, n);
        renderChunk (L, R, c);
        L += c; R += c; n -= c;
    }
}

void ChoirCrowdProcessor::renderChunk (float* L, float* R, int n)
{
    vowelSmooth += 0.1f * (p.vowel - vowelSmooth);
    setFormants (formants, vowelSmooth, (float) sr);
    setFormants (crowdFormants, 0.5f * vowelSmooth + 0.8f, (float) sr);

    const int ns = p.singers;
    const float norm = 1.0f / std::sqrt ((float) ns);

    for (auto& v : voices)
        if (v.active)
            for (int k = 0; k < ns; ++k)
                v.singers[(size_t) k].inc = v.baseFreq * std::exp2 (v.singers[(size_t) k].det * p.detune / 1200.0f) * invSr;

    for (int i = 0; i < n; ++i)
    {
        float outL = 0, outR = 0, envMax = 0;

        for (auto& v : voices)
        {
            if (! v.active) continue;
            float sl = 0, sR = 0;

            for (int k = 0; k < ns; ++k)
            {
                auto& s = v.singers[(size_t) k];
                if (s.delay > 0) { --s.delay; continue; }

                const float vib = 1.0f + p.vibrato * 0.012f * std::sin (s.vibPhase);
                const float dt  = juce::jmin (0.45f, s.inc * vib);
                s.vibPhase += s.vibInc;
                if (s.vibPhase > kTwoPi) s.vibPhase -= kTwoPi;
                s.phase += dt;
                if (s.phase >= 1.0f) s.phase -= 1.0f;

                const float x = (2.0f * s.phase - 1.0f - polyBlep (s.phase, dt)) * s.gain;
                sl += x * s.panL;
                sR += x * s.panR;
            }

            sl = sl * norm + (rng.nextFloat() * 2.0f - 1.0f) * p.breath * 0.25f;
            sR = sR * norm + (rng.nextFloat() * 2.0f - 1.0f) * p.breath * 0.25f;

            if (v.held) v.env = juce::jmin (1.0f, v.env + attInc);
            else
            {
                v.env *= relCoef;
                if (v.env < 1.0e-4f) { v.active = false; continue; }
            }

            const float g = v.env * v.vel;
            outL += v.ff[0].process (sl, formants) * g;
            outR += v.ff[1].process (sR, formants) * g;
            envMax = juce::jmax (envMax, g);
        }

        // Camada "crowd": murmúrio filtrado por formantes, com modulação irregular
        crowdEnv += 0.001f * (envMax - crowdEnv);
        if (--crowdCounter <= 0)
        {
            crowdTarget  = 0.35f + 0.65f * rng.nextFloat();
            crowdCounter = 1500 + rng.nextInt (3000);
        }
        crowdMod += 0.0004f * (crowdTarget - crowdMod);
        const float cg = p.crowd * crowdEnv * crowdMod * 5.0f;
        outL += crowdFF[0].process (rng.nextFloat() * 2.0f - 1.0f, crowdFormants) * cg;
        outR += crowdFF[1].process (rng.nextFloat() * 2.0f - 1.0f, crowdFormants) * cg;

        L[i] = outL;
        R[i] = outR;
    }
}

//==============================================================================
void ChoirCrowdProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    if (buffer.getNumChannels() < 2) return;

    cacheParams();
    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);
    const int N = buffer.getNumSamples();

    int pos = 0;
    for (const auto meta : midi)
    {
        const int ev = juce::jlimit (pos, N, meta.samplePosition);
        render (L + pos, R + pos, ev - pos);
        pos = ev;
        handleMidi (meta.getMessage());
    }
    render (L + pos, R + pos, N - pos);

    // Tone (low-pass) + ganho
    const float a = 1.0f - std::exp (-kTwoPi * p.tone * invSr);
    for (int i = 0; i < N; ++i)
    {
        toneL += a * (L[i] - toneL);
        toneR += a * (R[i] - toneR);
        L[i] = toneL * p.gain * 2.0f;
        R[i] = toneR * p.gain * 2.0f;
    }

    // Reverb
    juce::Reverb::Parameters rp;
    rp.roomSize = p.size;
    rp.damping  = 0.45f;
    rp.wetLevel = p.reverb * 0.45f;
    rp.dryLevel = 1.0f - p.reverb * 0.35f;
    rp.width    = 1.0f;
    reverb.setParameters (rp);
    reverb.processStereo (L, R, N);

    // Width (M/S) + saturação suave de segurança
    for (int i = 0; i < N; ++i)
    {
        const float m = 0.5f * (L[i] + R[i]);
        const float s = 0.5f * (L[i] - R[i]) * p.width;
        L[i] = std::tanh (m + s);
        R[i] = std::tanh (m - s);
    }
}

//==============================================================================
juce::AudioProcessorEditor* ChoirCrowdProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

void ChoirCrowdProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void ChoirCrowdProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ChoirCrowdProcessor(); }
