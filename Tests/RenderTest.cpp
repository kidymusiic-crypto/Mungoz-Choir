// Teste offline: toca um acorde, verifica que sai som, sem NaN/Inf, e que o estado é guardado/reposto.
#include "../Source/PluginProcessor.h"
#include <cstdio>

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    int failures = 0;
    auto check = [&] (bool ok, const char* what) { std::printf ("%s %s\n", ok ? "[OK]  " : "[FAIL]", what); if (! ok) ++failures; };

    for (double rate : { 44100.0, 48000.0, 96000.0 })
        for (int block : { 32, 512, 1024 })
        {
            ChoirCrowdProcessor p;
            p.setPlayConfigDetails (0, 2, rate, block);
            p.prepareToPlay (rate, block);

            juce::AudioBuffer<float> buf (2, block);
            float peak = 0; bool finite = true;
            const int total = (int) (rate * 3.0);
            for (int pos = 0; pos < total; pos += block)
            {
                juce::MidiBuffer midi;
                if (pos == 0) for (int n : { 48, 55, 60, 64 }) midi.addEvent (juce::MidiMessage::noteOn (1, n, 0.9f), 7);
                if (pos <= (int) rate * 2 && pos + block > (int) rate * 2)
                    for (int n : { 48, 55, 60, 64 }) midi.addEvent (juce::MidiMessage::noteOff (1, n), 0);
                p.processBlock (buf, midi);
                for (int c = 0; c < 2; ++c)
                    for (int i = 0; i < block; ++i)
                    {
                        const float x = buf.getSample (c, i);
                        if (! std::isfinite (x)) finite = false;
                        peak = juce::jmax (peak, std::abs (x));
                    }
            }
            char msg[128];
            std::snprintf (msg, sizeof msg, "sr=%.0f block=%d peak=%.3f finite=%d", rate, block, peak, (int) finite);
            check (finite && peak > 0.01f && peak <= 1.0f, msg);
        }

    // Estado (save/load de projeto na DAW)
    ChoirCrowdProcessor a, b;
    a.apvts.getParameter ("vowel")->setValueNotifyingHost (0.9f);
    juce::MemoryBlock mb; a.getStateInformation (mb);
    b.setStateInformation (mb.getData(), (int) mb.getSize());
    check (std::abs (b.apvts.getParameter ("vowel")->getValue() - 0.9f) < 1e-4f, "state save/restore");

    std::printf (failures == 0 ? "ALL PASSED\n" : "%d FAILED\n", failures);
    return failures == 0 ? 0 : 1;
}
