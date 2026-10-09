#include "Controls.h"
#include "../Parameters.h"

using namespace juce;

namespace
{
    void styleName (Label& l, const String& text)
    {
        l.setText (text.toUpperCase(), dontSendNotification);
        l.setFont (Font (FontOptions (10.5f, Font::bold)).withExtraKerningFactor (0.06f));
        l.setColour (Label::textColourId, Palette::textDim);
        l.setJustificationType (Justification::centred);
        l.setInterceptsMouseClicks (false, false);
    }
}

// =============================================================================================
Knob::Knob (APVTS& state, const String& paramID, const String& name, Colour accent)
{
    styleName (label, name);
    addAndMakeVisible (label);

    slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (Slider::TextBoxBelow, false, 80, 16);
    slider.setColour (Slider::rotarySliderFillColourId, accent);
    slider.setMouseDragSensitivity (220);
    slider.setScrollWheelEnabled (true);
    slider.setTooltip (parameterHelp (paramID) + "\n(Double-click to reset, click the value to type.)");
    addAndMakeVisible (slider);

    attachment = std::make_unique<APVTS::SliderAttachment> (state, paramID, slider);

    if (auto* p = state.getParameter (paramID))
        slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
}

void Knob::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (14));
    slider.setTextBoxStyle (Slider::TextBoxBelow, false, r.getWidth(), 16);
    slider.setBounds (r);
}

// =============================================================================================
Toggle::Toggle (APVTS& state, const String& paramID, const String& name, const String& buttonText, Colour accent)
{
    styleName (label, name);
    addAndMakeVisible (label);

    button.setButtonText (buttonText);
    button.setClickingTogglesState (true);
    button.setColour (TextButton::buttonOnColourId, accent);
    button.setTooltip (parameterHelp (paramID));
    addAndMakeVisible (button);

    attachment = std::make_unique<APVTS::ButtonAttachment> (state, paramID, button);
}

void Toggle::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (14));
    button.setBounds (r.withSizeKeepingCentre (jmin (r.getWidth() - 8, 76), 26));
}

// =============================================================================================
Choice::Choice (APVTS& state, const String& paramID, const String& name, Colour accent)
{
    styleName (label, name);
    addAndMakeVisible (label);

    if (auto* p = dynamic_cast<AudioParameterChoice*> (state.getParameter (paramID)))
        box.addItemList (p->choices, 1);

    box.setColour (ComboBox::focusedOutlineColourId, accent);
    box.setTooltip (parameterHelp (paramID));
    addAndMakeVisible (box);

    attachment = std::make_unique<APVTS::ComboBoxAttachment> (state, paramID, box);
}

void Choice::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (14));
    box.setBounds (r.withSizeKeepingCentre (r.getWidth() - 8, 26));
}

// =============================================================================================
Panel::Panel (const String& t, Colour a, int c, int r) : title (t), accent (a), columns (c), rows (r) {}

void Panel::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (Palette::panel);
    g.fillRoundedRectangle (r, 9.0f);
    g.setColour (Palette::panelEdge);
    g.drawRoundedRectangle (r, 9.0f, 1.0f);

    // accent tab + title
    g.setColour (accent);
    g.fillRoundedRectangle (12.0f, 11.0f, 3.0f, 12.0f, 1.5f);
    g.setFont (Font (FontOptions (12.0f, Font::bold)).withExtraKerningFactor (0.12f));
    g.setColour (Palette::text);
    g.drawText (title.toUpperCase(), 21, 6, getWidth() - 30, 22, Justification::centredLeft);
}

void Panel::place (Component& c, int col, int row, int colSpan, int rowSpan)
{
    cells.push_back ({ &c, col, row, colSpan, rowSpan });
    addAndMakeVisible (c);
}

void Panel::resized()
{
    auto area = getLocalBounds().reduced (6, 0);
    area.removeFromTop (30);
    area.removeFromBottom (6);

    const float cw = (float) area.getWidth() / (float) columns;
    const float ch = (float) area.getHeight() / (float) rows;

    for (auto& cell : cells)
        cell.comp->setBounds (Rectangle<float> ((float) area.getX() + cw * (float) cell.col,
                                                (float) area.getY() + ch * (float) cell.row,
                                                cw * (float) cell.colSpan, ch * (float) cell.rowSpan)
                                  .toNearestInt()
                                  .reduced (2, 1));
}
