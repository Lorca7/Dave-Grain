#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace Palette
{
    inline const juce::Colour background   { 0xff111217 };
    inline const juce::Colour header       { 0xff0b0c10 };
    inline const juce::Colour panel        { 0xff1b1d25 };
    inline const juce::Colour panelEdge    { 0xff2a2d39 };
    inline const juce::Colour well         { 0xff0e0f14 };
    inline const juce::Colour text         { 0xffe9eaf0 };
    inline const juce::Colour textDim      { 0xff8b90a5 };

    inline const juce::Colour grain        { 0xff38d9c4 };
    inline const juce::Colour pitch        { 0xfff6b24e };
    inline const juce::Colour window       { 0xffa88bfa };
    inline const juce::Colour amp          { 0xfff472b6 };
    inline const juce::Colour filter       { 0xff60a5fa };
    inline const juce::Colour lfo          { 0xff9ad31d };
    inline const juce::Colour fx           { 0xfffb923c };
    inline const juce::Colour output       { 0xffe9eaf0 };
}

class DaveLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DaveLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float startAngle, float endAngle, juce::Slider&) override;

    juce::Label* createSliderTextBox (juce::Slider&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool highlighted, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;

    juce::Font getPopupMenuFont() override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    void drawTooltip (juce::Graphics&, const juce::String& text, int width, int height) override;
    juce::Rectangle<int> getTooltipBounds (const juce::String& tipText, juce::Point<int> screenPos,
                                           juce::Rectangle<int> parentArea) override;
};
