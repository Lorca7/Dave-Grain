#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class DaveGrainProcessor;

/**
    Factory presets (compiled in) + user presets (".dgpreset" XML files in ~/Music/Dave Grain/Presets).
    A preset holds every parameter plus the sample's location.
*/
class PresetManager : public juce::ChangeBroadcaster
{
public:
    struct Entry
    {
        juce::String name;
        juce::String category;
        bool isFactory = false;
        int factoryIndex = -1;
        juce::File file;
    };

    explicit PresetManager (DaveGrainProcessor& processor);

    static juce::File getPresetDirectory();
    static inline const juce::String extension { ".dgpreset" };

    void refresh();
    const juce::Array<Entry>& getEntries() const { return entries; }

    void loadEntry (int index);
    void step (int delta);
    bool loadPresetFile (const juce::File& file);
    bool savePreset (const juce::File& file);
    void initialise();
    void randomise();

    int getCurrentIndex() const;
    juce::String getCurrentName() const { return currentName; }
    void setCurrentName (const juce::String& name);

private:
    void applyFactory (int factoryIndex);
    void setParameter (const juce::String& id, float realValue);
    void resetAllParameters();

    DaveGrainProcessor& processor;
    juce::Array<Entry> entries;
    juce::String currentName { "Init" };
    juce::File currentFile;
    juce::Random random;
};
