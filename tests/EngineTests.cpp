// Headless checks: render audio through the real processor for every factory preset,
// verify state/preset round-trips and WAV loading. Exit code 0 = all passed.

#include "PluginProcessor.h"
#include "FactoryPresets.h"
#include "dsp/BuiltinSources.h"
#include "PluginEditor.h"

namespace
{
    int failures = 0;

    void check (bool ok, const juce::String& what)
    {
        std::cout << (ok ? "  [ok]   " : "  [FAIL] ") << what << std::endl;
        if (! ok) ++failures;
    }

    struct RenderStats { float rms = 0.0f, peak = 0.0f; bool finite = true; };

    RenderStats render (DaveGrainProcessor& proc, double seconds, int note = 60, int blockSize = 512,
                        bool releaseHalfway = false)
    {
        const double sr = proc.getSampleRate();
        juce::AudioBuffer<float> buffer (2, blockSize);
        RenderStats stats;
        double sumSq = 0.0;
        juce::int64 count = 0;
        const int totalBlocks = (int) (seconds * sr / blockSize);

        for (int b = 0; b < totalBlocks; ++b)
        {
            juce::MidiBuffer midi;
            if (b == 0) midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 17);
            if (releaseHalfway && b == totalBlocks / 2) midi.addEvent (juce::MidiMessage::noteOff (1, note), 3);

            proc.processBlock (buffer, midi);

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < blockSize; ++i)
                {
                    const float v = buffer.getSample (ch, i);
                    if (! std::isfinite (v)) stats.finite = false;
                    stats.peak = juce::jmax (stats.peak, std::abs (v));
                    sumSq += (double) v * v;
                    ++count;
                }
        }

        // Release everything so the next render starts clean.
        juce::MidiBuffer off;
        off.addEvent (juce::MidiMessage::allSoundOff (1), 0);
        proc.processBlock (buffer, off);

        stats.rms = (float) std::sqrt (sumSq / (double) juce::jmax ((juce::int64) 1, count));
        return stats;
    }

    juce::File writeTestWav (const juce::File& dir, int channels, double sr)
    {
        auto file = dir.getChildFile ("test_" + juce::String (channels) + "ch.wav");
        file.deleteFile();
        juce::AudioBuffer<float> buf (channels, (int) sr * 2);
        for (int ch = 0; ch < channels; ++ch)
            for (int i = 0; i < buf.getNumSamples(); ++i)
                buf.setSample (ch, i, 0.5f * std::sin ((float) i * 0.03f * (float) (ch + 1)));

        juce::WavAudioFormat wav;
        std::unique_ptr<juce::OutputStream> stream (file.createOutputStream());
        auto options = juce::AudioFormatWriterOptions().withSampleRate (sr).withNumChannels (channels).withBitsPerSample (24);
        auto writer = wav.createWriterFor (stream, options);
        writer->writeFromAudioSampleBuffer (buf, 0, buf.getNumSamples());
        return file;
    }
}

/** Renders the editor (with a held chord, so grains are visible) to a PNG. Needs a display. */
static int snapshot (const juce::File& out, const juce::String& preset)
{
    DaveGrainProcessor proc;
    proc.setPlayConfigDetails (0, 2, 48000.0, 512);
    proc.prepareToPlay (48000.0, 512);

    for (int i = 0; i < proc.presets.getEntries().size(); ++i)
        if (proc.presets.getEntries()[i].name == preset) proc.presets.loadEntry (i);

    std::unique_ptr<juce::AudioProcessorEditor> editor (proc.createEditorIfNeeded());

    juce::AudioBuffer<float> buffer (2, 512);
    for (int b = 0; b < 120; ++b)
    {
        juce::MidiBuffer midi;
        if (b == 0) for (int n : { 48, 55, 60, 64 }) midi.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 100), 0);
        proc.processBlock (buffer, midi);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (5);
    }
    juce::MessageManager::getInstance()->runDispatchLoopUntil (200);

    auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
    out.deleteFile();
    juce::FileOutputStream stream (out);
    juce::PNGImageFormat png;
    const bool ok = png.writeImageToStream (image, stream);
    editor = nullptr;
    proc.editorBeingDeleted (nullptr);
    return ok ? 0 : 1;
}

/** Worst case: 8 voices, max density, longest grains. Prints the realtime CPU load of one core. */
static int bench()
{
    DaveGrainProcessor proc;
    proc.setPlayConfigDetails (0, 2, 48000.0, 256);
    proc.prepareToPlay (48000.0, 256);
    proc.apvts.getParameter (ParamIDs::density)->setValueNotifyingHost (1.0f);
    proc.apvts.getParameter (ParamIDs::size)->setValueNotifyingHost (1.0f);
    proc.apvts.getParameter (ParamIDs::reverbMix)->setValueNotifyingHost (0.5f);
    proc.apvts.getParameter (ParamIDs::delayMix)->setValueNotifyingHost (0.5f);

    juce::AudioBuffer<float> buffer (2, 256);
    juce::MidiBuffer midi;
    for (int n = 0; n < 8; ++n) midi.addEvent (juce::MidiMessage::noteOn (1, 48 + n * 3, (juce::uint8) 100), 0);
    proc.processBlock (buffer, midi);
    midi.clear();

    const int blocks = (int) (10.0 * 48000.0 / 256.0);
    const auto start = juce::Time::getMillisecondCounterHiRes();
    for (int b = 0; b < blocks; ++b) proc.processBlock (buffer, midi);
    const double ms = juce::Time::getMillisecondCounterHiRes() - start;

    std::cout << "Rendered 10 s in " << ms << " ms -> " << ms / 100.0 << " % of one core ("
              << proc.engine.vizNumGrains.load() << " live grains)" << std::endl;
    return 0;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    if (argc >= 2 && juce::String (argv[1]) == "--bench")
        return bench();

    if (argc >= 3 && juce::String (argv[1]) == "--snapshot")
        return snapshot (juce::File (argv[2]), argc >= 4 ? juce::String (argv[3]) : juce::String ("Init"));

    DaveGrainProcessor proc;
    proc.setPlayConfigDetails (0, 2, 48000.0, 512);
    proc.prepareToPlay (48000.0, 512);

    std::cout << "Basics" << std::endl;
    check (proc.getSample() != nullptr, "built-in source loaded at startup");
    check (proc.getParameters().size() >= 45, "parameter count = " + juce::String (proc.getParameters().size()));

    {
        juce::AudioBuffer<float> buffer (2, 512);
        juce::MidiBuffer midi;
        proc.processBlock (buffer, midi);
        check (buffer.getMagnitude (0, 512) <= 0.0f, "silent with no notes");
    }

    auto s = render (proc, 2.0, 60, 512, true);
    check (s.finite && s.rms > 0.005f && s.peak <= 1.0f,
           "default patch makes sound (rms " + juce::String (s.rms, 4) + ", peak " + juce::String (s.peak, 3) + ")");

    std::cout << "Factory presets" << std::endl;
    const auto& factory = getFactoryPresets();
    for (int i = 0; i < (int) factory.size(); ++i)
    {
        proc.presets.loadEntry (i);
        auto st = render (proc, 1.5, 60, 256);
        check (proc.presets.getCurrentName() == factory[(size_t) i].name && st.finite && st.rms > 0.001f && st.peak <= 1.0f,
               juce::String (factory[(size_t) i].name).paddedRight (' ', 18) + " rms " + juce::String (st.rms, 4)
                   + "  peak " + juce::String (st.peak, 3));
    }

    std::cout << "Odd block sizes / extremes" << std::endl;
    for (int bs : { 1, 7, 64, 333, 2048 })
    {
        auto st = render (proc, 0.5, 48, bs);
        check (st.finite && st.peak <= 1.0f, "block size " + juce::String (bs));
    }
    {
        // Max density, min size, max pitch, full random, everything on.
        auto set = [&] (const char* id, float norm) { proc.apvts.getParameter (id)->setValueNotifyingHost (norm); };
        set (ParamIDs::density, 1.0f); set (ParamIDs::size, 1.0f); set (ParamIDs::pitch, 1.0f);
        set (ParamIDs::pitchRand, 1.0f); set (ParamIDs::spray, 1.0f); set (ParamIDs::resonance, 1.0f);
        set (ParamIDs::delayMix, 1.0f); set (ParamIDs::delayFb, 1.0f); set (ParamIDs::reverbMix, 1.0f);
        set (ParamIDs::scan, 0.0f);
        for (int n : { 24, 60, 96, 127 })
        {
            auto st = render (proc, 0.5, n);
            check (st.finite && st.peak <= 1.0f, "extreme settings, note " + juce::String (n));
        }
    }
    {
        // 8+ simultaneous notes (voice stealing)
        juce::AudioBuffer<float> buffer (2, 512);
        juce::MidiBuffer midi;
        for (int n = 0; n < 12; ++n) midi.addEvent (juce::MidiMessage::noteOn (1, 48 + n, (juce::uint8) 90), n);
        bool finite = true;
        for (int b = 0; b < 100; ++b)
        {
            proc.processBlock (buffer, midi);
            midi.clear();
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 512; ++i) finite &= std::isfinite (buffer.getSample (ch, i));
        }
        check (finite && proc.engine.getNumActiveVoices() == GrainEngine::maxVoices, "12 notes -> 8 voices, stealing ok");
        juce::MidiBuffer off; off.addEvent (juce::MidiMessage::allSoundOff (1), 0);
        proc.processBlock (buffer, off);
    }

    std::cout << "Randomize" << std::endl;
    for (int i = 0; i < 20; ++i)
    {
        proc.presets.randomise();
        auto st = render (proc, 0.5, 60, 512);
        if (! (st.finite && st.peak <= 1.0f)) check (false, "random patch " + juce::String (i));
    }
    check (true, "20 random patches rendered finite and <= 0 dBFS");

    std::cout << "Sample loading" << std::endl;
    auto tmp = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("DaveGrainTests");
    tmp.createDirectory();
    juce::String error;
    auto mono = writeTestWav (tmp, 1, 44100.0);
    check (proc.loadSampleFile (mono, error) && proc.getSample()->buffer.getNumChannels() == 2, "mono WAV loads as stereo");
    auto stereo = writeTestWav (tmp, 2, 96000.0);
    check (proc.loadSampleFile (stereo, error) && juce::approximatelyEqual (proc.getSample()->sampleRate, 96000.0), "96 kHz stereo WAV loads");
    check (proc.hasUserSample(), "user sample flagged");
    check (! proc.loadSampleFile (tmp.getChildFile ("nope.wav"), error) && error.isNotEmpty(), "missing file reports error");
    proc.presets.initialise();
    check (proc.getSample()->source == stereo.getFullPathName(), "factory preset keeps the user's sample");
    {
        auto st = render (proc, 1.0);
        check (st.finite && st.rms > 0.001f, "renders user WAV");
    }

    std::cout << "State + presets" << std::endl;
    proc.apvts.getParameter (ParamIDs::size)->setValueNotifyingHost (0.77f);
    proc.apvts.getParameter (ParamIDs::window)->setValueNotifyingHost (1.0f);
    juce::MemoryBlock block;
    proc.getStateInformation (block);

    {
        DaveGrainProcessor other;
        other.setStateInformation (block.getData(), (int) block.getSize());
        check (std::abs (other.apvts.getParameter (ParamIDs::size)->getValue() - 0.77f) < 1.0e-4f, "host state restores parameters");
        check (other.getSample() != nullptr && other.getSample()->source == stereo.getFullPathName(), "host state restores sample");
    }

    auto presetFile = tmp.getChildFile ("Unit Test" + PresetManager::extension);
    check (proc.presets.savePreset (presetFile), "save preset file");
    proc.presets.initialise();
    proc.loadBuiltinSample ("Bell Cloud");
    check (proc.presets.loadPresetFile (presetFile), "load preset file");
    check (std::abs (proc.apvts.getParameter (ParamIDs::size)->getValue() - 0.77f) < 1.0e-4f
               && proc.getSample()->source == stereo.getFullPathName()
               && proc.presets.getCurrentName() == "Unit Test",
           "preset restores parameters, sample and name");

    proc.keepSampleOnPresetLoad = true;
    proc.loadBuiltinSample ("Wind Texture");
    proc.presets.loadPresetFile (presetFile);
    check (proc.getSample()->name == "Wind Texture", "Keep Sample keeps the current sample");

    {
        DaveGrainProcessor missing;
        auto tree = proc.createStateTree();
        tree.setProperty ("sampleSource", "/does/not/exist.wav", nullptr);
        missing.restoreStateTree (tree, true);
        check (missing.getMissingSample() == "/does/not/exist.wav", "missing sample is reported, not fatal");
    }

    tmp.deleteRecursively();
    proc.releaseResources();

    std::cout << std::endl << (failures == 0 ? "ALL TESTS PASSED" : juce::String (failures) + " TEST(S) FAILED") << std::endl;
    return failures == 0 ? 0 : 1;
}
