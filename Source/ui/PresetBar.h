#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_events/juce_events.h>

class DaveGrainProcessor;

/** Preset browser (prev / name menu / next), save, randomise, and the "keep sample" lock. */
class PresetBar : public juce::Component, private juce::ChangeListener
{
public:
    explicit PresetBar (DaveGrainProcessor&);
    ~PresetBar() override;

    void resized() override;
    void paint (juce::Graphics&) override;

    std::function<void (const juce::String&)> onMessage;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void showPresetMenu();
    void saveAs();
    void importPreset();
    void update();

    DaveGrainProcessor& processor;
    juce::TextButton prev { "<" }, next { ">" }, name, save { "Save" }, dice { "Randomize" }, keepSample { "Keep Sample" };
    std::unique_ptr<juce::FileChooser> chooser;
};
