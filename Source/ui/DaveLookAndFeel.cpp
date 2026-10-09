#include "DaveLookAndFeel.h"

using namespace juce;

DaveLookAndFeel::DaveLookAndFeel()
{
    setColour (ResizableWindow::backgroundColourId, Palette::background);
    setColour (Label::textColourId, Palette::text);
    setColour (Slider::textBoxTextColourId, Palette::text);
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    setColour (Slider::textBoxHighlightColourId, Palette::grain.withAlpha (0.4f));
    setColour (Slider::rotarySliderOutlineColourId, Palette::panelEdge);
    setColour (TextEditor::backgroundColourId, Palette::well);
    setColour (TextEditor::textColourId, Palette::text);
    setColour (TextEditor::outlineColourId, Palette::panelEdge);
    setColour (TextEditor::focusedOutlineColourId, Palette::grain);
    setColour (ComboBox::backgroundColourId, Palette::well);
    setColour (ComboBox::textColourId, Palette::text);
    setColour (ComboBox::outlineColourId, Palette::panelEdge);
    setColour (ComboBox::arrowColourId, Palette::textDim);
    setColour (PopupMenu::backgroundColourId, Palette::panel);
    setColour (PopupMenu::textColourId, Palette::text);
    setColour (PopupMenu::headerTextColourId, Palette::textDim);
    setColour (PopupMenu::highlightedBackgroundColourId, Palette::grain.withAlpha (0.25f));
    setColour (PopupMenu::highlightedTextColourId, Palette::text);
    setColour (TextButton::buttonColourId, Palette::panel);
    setColour (TextButton::buttonOnColourId, Palette::grain);
    setColour (TextButton::textColourOffId, Palette::text);
    setColour (TextButton::textColourOnId, Palette::header);
    setColour (AlertWindow::backgroundColourId, Palette::panel);
    setColour (AlertWindow::textColourId, Palette::text);
    setColour (AlertWindow::outlineColourId, Palette::panelEdge);
    setColour (TooltipWindow::backgroundColourId, Palette::header);
    setColour (TooltipWindow::textColourId, Palette::text);
}

void DaveLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int width, int height, float pos,
                                        float startAngle, float endAngle, Slider& slider)
{
    const auto accent = slider.findColour (Slider::rotarySliderFillColourId);
    const auto bounds = Rectangle<int> (x, y, width, height).toFloat().reduced (4.0f);
    const float radius = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float track = jmax (2.5f, radius * 0.13f);
    const float arcRadius = radius - track * 0.5f;
    const float angle = startAngle + pos * (endAngle - startAngle);
    const bool hot = slider.isMouseOverOrDragging();

    // Track
    Path bg;
    bg.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, endAngle, true);
    g.setColour (Palette::panelEdge);
    g.strokePath (bg, PathStrokeType (track, PathStrokeType::curved, PathStrokeType::rounded));

    // Value arc (bipolar sliders fill from the centre)
    const bool bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
    const float from = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;
    Path arc;
    arc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, jmin (from, angle), jmax (from, angle), true);
    g.setColour (accent.withAlpha (hot ? 1.0f : 0.9f));
    g.strokePath (arc, PathStrokeType (track, PathStrokeType::curved, PathStrokeType::rounded));

    // Knob body
    const float bodyR = arcRadius - track * 1.3f;
    g.setGradientFill (ColourGradient (Colour (0xff2c2f3b), centre.x, centre.y - bodyR,
                                       Colour (0xff17181f), centre.x, centre.y + bodyR, false));
    g.fillEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);
    g.setColour (hot ? accent.withAlpha (0.5f) : Palette::panelEdge);
    g.drawEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);

    // Pointer
    Path pointer;
    const float pw = jmax (2.0f, bodyR * 0.14f);
    pointer.addRoundedRectangle (-pw * 0.5f, -bodyR + 3.0f, pw, bodyR * 0.45f, pw * 0.5f);
    g.setColour (Palette::text);
    g.fillPath (pointer, AffineTransform::rotation (angle).translated (centre));
}

Label* DaveLookAndFeel::createSliderTextBox (Slider& slider)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (slider);
    l->setFont (Font (FontOptions (11.5f)));
    l->setColour (Label::textColourId, Palette::textDim);
    l->setColour (Label::outlineWhenEditingColourId, Palette::grain);
    l->setJustificationType (Justification::centred);
    return l;
}

void DaveLookAndFeel::drawButtonBackground (Graphics& g, Button& button, const Colour&, bool highlighted, bool down)
{
    auto r = button.getLocalBounds().toFloat().reduced (0.5f);
    const auto accent = button.findColour (TextButton::buttonOnColourId);
    const bool on = button.getToggleState();

    Colour fill = on ? accent : Palette::well;
    if (highlighted) fill = fill.brighter (0.12f);
    if (down) fill = fill.darker (0.15f);

    g.setColour (fill);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (on ? accent.brighter (0.2f) : (highlighted ? accent.withAlpha (0.6f) : Palette::panelEdge));
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
}

void DaveLookAndFeel::drawButtonText (Graphics& g, TextButton& button, bool, bool)
{
    g.setFont (getTextButtonFont (button, button.getHeight()));
    g.setColour (button.getToggleState() ? Palette::header : Palette::text.withAlpha (button.isEnabled() ? 1.0f : 0.4f));
    g.drawFittedText (button.getButtonText(), button.getLocalBounds().reduced (4, 0), Justification::centred, 1);
}

Font DaveLookAndFeel::getTextButtonFont (TextButton&, int buttonHeight)
{
    return Font (FontOptions (jmin (13.0f, (float) buttonHeight * 0.55f), Font::bold));
}

void DaveLookAndFeel::drawComboBox (Graphics& g, int width, int height, bool, int, int, int, int, ComboBox& box)
{
    auto r = Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);
    g.setColour (Palette::well);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (box.isMouseOver (true) ? box.findColour (ComboBox::focusedOutlineColourId) : Palette::panelEdge);
    g.drawRoundedRectangle (r, 5.0f, 1.0f);

    Path arrow;
    const float ax = (float) width - 12.0f, ay = (float) height * 0.5f;
    arrow.startNewSubPath (ax - 4.0f, ay - 2.0f);
    arrow.lineTo (ax, ay + 2.0f);
    arrow.lineTo (ax + 4.0f, ay - 2.0f);
    g.setColour (Palette::textDim);
    g.strokePath (arrow, PathStrokeType (1.5f, PathStrokeType::curved, PathStrokeType::rounded));
}

Font DaveLookAndFeel::getComboBoxFont (ComboBox&)          { return Font (FontOptions (12.5f)); }
Font DaveLookAndFeel::getPopupMenuFont()                    { return Font (FontOptions (14.0f)); }

void DaveLookAndFeel::positionComboBoxText (ComboBox& box, Label& label)
{
    label.setBounds (4, 1, box.getWidth() - 22, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
    label.setJustificationType (Justification::centredLeft);
}

void DaveLookAndFeel::drawTooltip (Graphics& g, const String& text, int width, int height)
{
    auto r = Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);
    g.setColour (Palette::header.withAlpha (0.97f));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (Palette::panelEdge);
    g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);

    AttributedString s;
    s.setJustification (Justification::centredLeft);
    s.append (text, Font (FontOptions (13.0f)), Palette::text);
    TextLayout layout;
    layout.createLayoutWithBalancedLineLengths (s, 300.0f);
    layout.draw (g, r.reduced (10.0f, 6.0f));
}

Rectangle<int> DaveLookAndFeel::getTooltipBounds (const String& tipText, Point<int> screenPos, Rectangle<int> parentArea)
{
    AttributedString s;
    s.append (tipText, Font (FontOptions (13.0f)));
    TextLayout layout;
    layout.createLayoutWithBalancedLineLengths (s, 300.0f);
    const int w = (int) layout.getWidth() + 22, h = (int) layout.getHeight() + 14;

    return Rectangle<int> (screenPos.x > parentArea.getCentreX() ? screenPos.x - (w + 12) : screenPos.x + 24,
                           screenPos.y > parentArea.getCentreY() ? screenPos.y - (h + 6) : screenPos.y + 6, w, h)
        .constrainedWithin (parentArea);
}
