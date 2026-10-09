#include "PresetManager.h"
#include "PluginProcessor.h"
#include "FactoryPresets.h"
#include "dsp/BuiltinSources.h"

PresetManager::PresetManager (DaveGrainProcessor& p) : processor (p)
{
    refresh();
}

juce::File PresetManager::getPresetDirectory()
{
    return juce::File::getSpecialLocation (juce::File::userMusicDirectory)
               .getChildFile ("Dave Grain")
               .getChildFile ("Presets");
}

void PresetManager::refresh()
{
    entries.clear();

    const auto& factory = getFactoryPresets();
    for (int i = 0; i < (int) factory.size(); ++i)
        entries.add ({ factory[(size_t) i].name, factory[(size_t) i].category, true, i, {} });

    auto dir = getPresetDirectory();
    if (dir.isDirectory())
    {
        auto files = dir.findChildFiles (juce::File::findFiles, true, "*" + extension);
        files.sort();
        for (auto& f : files)
        {
            auto parent = f.getParentDirectory();
            entries.add ({ f.getFileNameWithoutExtension(),
                           parent == dir ? juce::String ("User") : "User / " + parent.getFileName(),
                           false, -1, f });
        }
    }

    sendChangeMessage();
}

int PresetManager::getCurrentIndex() const
{
    for (int i = 0; i < entries.size(); ++i)
    {
        const auto& e = entries.getReference (i);
        if (e.isFactory ? (currentFile == juce::File() && e.name == currentName)
                        : e.file == currentFile)
            return i;
    }
    return -1;
}

void PresetManager::setCurrentName (const juce::String& name)
{
    currentName = name;
    sendChangeMessage();
}

void PresetManager::setParameter (const juce::String& id, float realValue)
{
    if (auto* param = processor.apvts.getParameter (id))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 (realValue));
        param->endChangeGesture();
    }
}

void PresetManager::resetAllParameters()
{
    for (auto* param : processor.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
        {
            ranged->beginChangeGesture();
            ranged->setValueNotifyingHost (ranged->getDefaultValue());
            ranged->endChangeGesture();
        }
}

void PresetManager::applyFactory (int factoryIndex)
{
    const auto& preset = getFactoryPresets()[(size_t) factoryIndex];

    resetAllParameters();
    for (auto& [id, value] : preset.values)
        setParameter (id, value);

    // Factory presets never replace a sample you loaded yourself; they only swap demo sources.
    const bool canSwapSample = ! processor.hasUserSample() && ! processor.keepSampleOnPresetLoad.load();
    if (canSwapSample || processor.getSample() == nullptr)
        processor.loadBuiltinSample (preset.builtinSource);

    currentFile = juce::File();
    currentName = preset.name;
    sendChangeMessage();
}

void PresetManager::loadEntry (int index)
{
    if (! juce::isPositiveAndBelow (index, entries.size())) return;

    const auto e = entries[index];
    if (e.isFactory) applyFactory (e.factoryIndex);
    else             loadPresetFile (e.file);
}

void PresetManager::step (int delta)
{
    if (entries.isEmpty()) return;
    auto index = getCurrentIndex();
    index = index < 0 ? 0 : (index + delta + entries.size()) % entries.size();
    loadEntry (index);
}

bool PresetManager::loadPresetFile (const juce::File& file)
{
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr) return false;

    auto tree = juce::ValueTree::fromXml (*xml);
    if (! tree.isValid()) return false;

    // Start from defaults so presets saved by older versions don't inherit stray values.
    resetAllParameters();
    processor.restoreStateTree (tree, ! processor.keepSampleOnPresetLoad.load());

    currentFile = file;
    currentName = file.getFileNameWithoutExtension();
    sendChangeMessage();
    return true;
}

bool PresetManager::savePreset (const juce::File& requested)
{
    auto file = requested.withFileExtension (extension);
    if (! file.getParentDirectory().createDirectory()) return false;

    auto tree = processor.createStateTree();
    tree.setProperty ("presetName", file.getFileNameWithoutExtension(), nullptr);

    auto xml = tree.createXml();
    if (xml == nullptr || ! xml->writeTo (file)) return false;

    currentFile = file;
    currentName = file.getFileNameWithoutExtension();
    refresh();
    return true;
}

void PresetManager::initialise()
{
    applyFactory (0);
}

void PresetManager::randomise()
{
    using namespace ParamIDs;

    for (auto& id : randomisableParameterIDs())
    {
        auto* param = processor.apvts.getParameter (id);
        if (param == nullptr) continue;

        float v = random.nextFloat();

        // Keep things musical: limit the wet effects and make LFOs gentler most of the time.
        if (id == delayMix)                          v *= 0.45f;
        else if (id == reverbMix)                    v *= 0.6f;
        else if (id == delayFb)                      v *= 0.75f;
        else if (id == spray)                        v = v * v;
        else if (id == scan)                         v = 0.5f + (v - 0.5f) * 0.4f; // +/- 80 %
        else if (id == lfo1Depth || id == lfo2Depth) v = 0.5f + (v - 0.5f) * 0.6f;
        else if (id == lfo1Rate || id == lfo2Rate)   v *= 0.7f;
        else if (id == cutoff)                       v = 0.45f + 0.55f * v;
        else if (id == resonance)                    v *= 0.6f;
        else if (id == pitchRand)                    v = random.nextFloat() < 0.5f ? 0.0f : v;

        param->beginChangeGesture();
        param->setValueNotifyingHost (v);
        param->endChangeGesture();
    }

    currentFile = juce::File();
    currentName = "Random " + juce::String::toHexString (random.nextInt (0xfff) + 0x100).toUpperCase();
    sendChangeMessage();
}
