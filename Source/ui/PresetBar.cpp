#include "PresetBar.h"
#include "DaveLookAndFeel.h"
#include "../PluginProcessor.h"

using namespace juce;

PresetBar::PresetBar (DaveGrainProcessor& p) : processor (p)
{
    for (auto* b : { &prev, &next, &name, &save, &dice, &keepSample })
        addAndMakeVisible (b);

    prev.setTooltip ("Previous preset");
    next.setTooltip ("Next preset");
    name.setTooltip ("Browse presets");
    save.setTooltip ("Save the current sound (including the sample's location) as a preset");
    dice.setTooltip ("Randomize the grain, pitch, filter, LFO and FX settings for happy accidents");
    keepSample.setTooltip ("When on, loading presets keeps the sample you're working with");

    prev.onClick = [this] { processor.presets.step (-1); };
    next.onClick = [this] { processor.presets.step (+1); };
    name.onClick = [this] { showPresetMenu(); };
    save.onClick = [this] { saveAs(); };
    dice.onClick = [this] { processor.presets.randomise(); };

    keepSample.setClickingTogglesState (true);
    keepSample.setColour (TextButton::buttonOnColourId, Palette::pitch);
    keepSample.setToggleState (processor.keepSampleOnPresetLoad.load(), dontSendNotification);
    keepSample.onClick = [this] { processor.keepSampleOnPresetLoad.store (keepSample.getToggleState()); };

    dice.setColour (TextButton::buttonOnColourId, Palette::window);

    processor.presets.addChangeListener (this);
    update();
}

PresetBar::~PresetBar()
{
    processor.presets.removeChangeListener (this);
}

void PresetBar::changeListenerCallback (ChangeBroadcaster*) { update(); }

void PresetBar::update()
{
    name.setButtonText (processor.presets.getCurrentName());
    keepSample.setToggleState (processor.keepSampleOnPresetLoad.load(), dontSendNotification);
}

void PresetBar::paint (Graphics&) {}

void PresetBar::resized()
{
    auto r = getLocalBounds();
    keepSample.setBounds (r.removeFromRight (100).reduced (2, 0));
    r.removeFromRight (6);
    dice.setBounds (r.removeFromRight (96).reduced (2, 0));
    save.setBounds (r.removeFromRight (64).reduced (2, 0));
    r.removeFromRight (6);
    prev.setBounds (r.removeFromLeft (30).reduced (2, 0));
    next.setBounds (r.removeFromRight (30).reduced (2, 0));
    name.setBounds (r.reduced (2, 0));
}

void PresetBar::showPresetMenu()
{
    auto& pm = processor.presets;
    pm.refresh();
    const auto& entries = pm.getEntries();
    const int current = pm.getCurrentIndex();

    PopupMenu menu;
    std::map<String, PopupMenu> factory, user;
    StringArray factoryOrder, userOrder;

    for (int i = 0; i < entries.size(); ++i)
    {
        const auto& e = entries.getReference (i);
        auto& group = e.isFactory ? factory : user;
        auto& order = e.isFactory ? factoryOrder : userOrder;
        if (! order.contains (e.category)) order.add (e.category);
        group[e.category].addItem (i + 1, e.name, true, i == current);
    }

    menu.addSectionHeader ("Factory");
    for (auto& c : factoryOrder) menu.addSubMenu (c, factory[c]);

    menu.addSectionHeader ("User");
    if (userOrder.isEmpty())
        menu.addItem (-100, "(no saved presets yet)", false);
    for (auto& c : userOrder)
    {
        if (c == "User") for (PopupMenu::MenuItemIterator it (user[c]); it.next();) menu.addItem (it.getItem());
        else menu.addSubMenu (c.fromFirstOccurrenceOf ("User / ", false, false), user[c]);
    }

    menu.addSeparator();
    menu.addItem (-1, "Save Preset As...");
    menu.addItem (-2, "Import Preset File...");
    menu.addItem (-3, "Show Preset Folder");
    menu.addItem (-4, "Init (reset all)");

    menu.showMenuAsync (PopupMenu::Options().withTargetComponent (&name).withMinimumWidth (name.getWidth()),
                        [this] (int result)
                        {
                            if (result > 0) processor.presets.loadEntry (result - 1);
                            else if (result == -1) saveAs();
                            else if (result == -2) importPreset();
                            else if (result == -3)
                            {
                                auto dir = PresetManager::getPresetDirectory();
                                dir.createDirectory();
                                dir.revealToUser();
                            }
                            else if (result == -4) processor.presets.initialise();
                        });
}

void PresetBar::saveAs()
{
    auto dir = PresetManager::getPresetDirectory();
    dir.createDirectory();

    auto suggested = processor.presets.getCurrentName();
    if (suggested.isEmpty() || suggested == "Init") suggested = "My Grain Sound";

    chooser = std::make_unique<FileChooser> ("Save Dave Grain preset",
                                             dir.getChildFile (File::createLegalFileName (suggested) + PresetManager::extension),
                                             "*" + PresetManager::extension);

    chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles
                              | FileBrowserComponent::warnAboutOverwriting,
                          [this] (const FileChooser& fc)
                          {
                              auto file = fc.getResult();
                              if (file == File()) return;
                              if (processor.presets.savePreset (file))
                              {
                                  if (onMessage) onMessage ("Saved preset \"" + file.getFileNameWithoutExtension() + "\"");
                              }
                              else if (onMessage) onMessage ("Could not save preset to " + file.getFullPathName());
                          });
}

void PresetBar::importPreset()
{
    chooser = std::make_unique<FileChooser> ("Open Dave Grain preset", PresetManager::getPresetDirectory(),
                                             "*" + PresetManager::extension);
    chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                          [this] (const FileChooser& fc)
                          {
                              auto file = fc.getResult();
                              if (file != File() && ! processor.presets.loadPresetFile (file) && onMessage)
                                  onMessage ("That file isn't a valid Dave Grain preset.");
                          });
}
