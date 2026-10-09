#include "WaveformView.h"
#include "DaveLookAndFeel.h"
#include "../PluginProcessor.h"

using namespace juce;

WaveformView::WaveformView (DaveGrainProcessor& p) : processor (p)
{
    setRepaintsOnMouseActivity (true);
    startTimerHz (30);
}

Rectangle<float> WaveformView::waveArea() const
{
    return getLocalBounds().toFloat().reduced (12.0f, 30.0f).withTrimmedBottom (-12.0f);
}

void WaveformView::resized()
{
    rebuildPeaks();
}

void WaveformView::rebuildPeaks()
{
    peaks.clear();
    if (sample == nullptr || sample->getNumSamples() == 0) return;

    const int columns = jmax (1, (int) waveArea().getWidth());
    const int n = sample->getNumSamples();
    const float* l = sample->buffer.getReadPointer (0);
    const float* r = sample->buffer.getReadPointer (1);

    peaks.resize ((size_t) columns);
    for (int c = 0; c < columns; ++c)
    {
        const int start = (int) ((juce::int64) c * n / columns);
        const int end = jmax (start + 1, (int) ((juce::int64) (c + 1) * n / columns));
        float lo = 0.0f, hi = 0.0f;
        const int stride = jmax (1, (end - start) / 256); // cap work for very long files
        for (int i = start; i < end && i < n; i += stride)
        {
            const float v = 0.5f * (l[i] + r[i]);
            lo = jmin (lo, v);
            hi = jmax (hi, v);
        }
        peaks[(size_t) c] = { lo, hi };
    }
}

void WaveformView::timerCallback()
{
    if (processor.getSampleVersion() != seenVersion)
    {
        seenVersion = processor.getSampleVersion();
        sample = processor.getSample();
        rebuildPeaks();
    }

    if (messageTicks > 0 && --messageTicks == 0)
        message.clear();

    phase += 0.03f;
    repaint();
}

void WaveformView::setDropHighlight (bool b)
{
    dropHighlight = b;
    repaint();
}

void WaveformView::showMessage (const String& text)
{
    message = text;
    messageTicks = 30 * 5;
    repaint();
}

void WaveformView::paint (Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (Palette::well);
    g.fillRoundedRectangle (bounds, 10.0f);

    const auto area = waveArea();
    const float midY = area.getCentreY();
    const float halfH = area.getHeight() * 0.5f;

    // subtle grid
    g.setColour (Palette::panelEdge.withAlpha (0.35f));
    for (int i = 1; i < 16; ++i)
    {
        const float x = area.getX() + area.getWidth() * (float) i / 16.0f;
        g.drawVerticalLine ((int) x, area.getY(), area.getBottom());
    }
    g.drawHorizontalLine ((int) midY, area.getX(), area.getRight());

    const auto& apvts = processor.apvts;
    const float position = apvts.getRawParameterValue (ParamIDs::position)->load();
    const float spray = apvts.getRawParameterValue (ParamIDs::spray)->load();

    if (sample == nullptr || peaks.empty())
    {
        g.setColour (Palette::textDim);
        g.setFont (Font (FontOptions (16.0f)));
        const auto missing = processor.getMissingSample();
        g.drawFittedText (missing.isNotEmpty()
                              ? "Sample not found:\n" + missing + "\nDrop the file here again or use Load Sample."
                              : "Drop a WAV / AIFF / FLAC file here\nor click  LOAD SAMPLE",
                          getLocalBounds().reduced (20), Justification::centred, 4);
    }
    else
    {
        // Spray region (wraps around the ends)
        const float sprayW = spray * area.getWidth();
        const float px = area.getX() + position * area.getWidth();
        g.setColour (Palette::grain.withAlpha (0.10f));
        auto fillSpan = [&] (float x0, float x1)
        {
            x0 = jlimit (area.getX(), area.getRight(), x0);
            x1 = jlimit (area.getX(), area.getRight(), x1);
            if (x1 > x0) g.fillRect (x0, area.getY(), x1 - x0, area.getHeight());
        };
        fillSpan (px - sprayW * 0.5f, px + sprayW * 0.5f);
        if (px - sprayW * 0.5f < area.getX()) fillSpan (px - sprayW * 0.5f + area.getWidth(), area.getRight());
        if (px + sprayW * 0.5f > area.getRight()) fillSpan (area.getX(), px + sprayW * 0.5f - area.getWidth());

        // Waveform
        Path wave;
        const int cols = (int) peaks.size();
        for (int c = 0; c < cols; ++c)
        {
            const float x = area.getX() + (float) c;
            const float top = midY - peaks[(size_t) c].second * halfH;
            if (c == 0) wave.startNewSubPath (x, top); else wave.lineTo (x, top);
        }
        for (int c = cols - 1; c >= 0; --c)
            wave.lineTo (area.getX() + (float) c, midY - peaks[(size_t) c].first * halfH);
        wave.closeSubPath();

        g.setGradientFill (ColourGradient (Palette::grain.withAlpha (0.55f), 0.0f, area.getY(),
                                           Palette::window.withAlpha (0.45f), 0.0f, area.getBottom(), false));
        g.fillPath (wave);
        g.setColour (Palette::grain.withAlpha (0.25f));
        g.strokePath (wave, PathStrokeType (0.8f));

        // Voice playheads
        for (auto& ph : processor.engine.vizPlayhead)
        {
            const float v = ph.load (std::memory_order_relaxed);
            if (v < 0.0f) continue;
            const float x = area.getX() + v * area.getWidth();
            g.setColour (Palette::pitch.withAlpha (0.8f));
            g.drawLine (x, area.getY(), x, area.getBottom(), 1.2f);
        }

        // Grains
        const int numGrains = processor.engine.vizNumGrains.load (std::memory_order_acquire);
        for (int i = 0; i < numGrains; ++i)
        {
            const float gp = processor.engine.vizGrainPos[(size_t) i].load (std::memory_order_relaxed);
            const float amp = jlimit (0.0f, 1.0f, processor.engine.vizGrainAmp[(size_t) i].load (std::memory_order_relaxed));
            const float pan = processor.engine.vizGrainPan[(size_t) i].load (std::memory_order_relaxed);
            const float x = area.getX() + gp * area.getWidth();
            const float y = midY - pan * halfH * 0.8f;
            const float rad = 2.0f + amp * 6.0f;
            g.setColour (Palette::text.withAlpha (0.10f * amp));
            g.fillEllipse (x - rad * 2.0f, y - rad * 2.0f, rad * 4.0f, rad * 4.0f);
            g.setColour (Palette::grain.brighter (0.4f).withAlpha (0.35f + 0.65f * amp));
            g.fillEllipse (x - rad * 0.5f, y - rad * 0.5f, rad, rad);
        }

        // Position marker
        g.setColour (Palette::grain);
        g.drawLine (px, area.getY() - 4.0f, px, area.getBottom() + 4.0f, 2.0f);
        Path handle;
        handle.addTriangle (px - 6.0f, area.getY() - 10.0f, px + 6.0f, area.getY() - 10.0f, px, area.getY() - 3.0f);
        g.fillPath (handle);

        // Header text
        g.setFont (Font (FontOptions (13.0f, Font::bold)));
        g.setColour (Palette::text);
        g.drawText (sample->name, 14, 6, getWidth() / 2, 18, Justification::centredLeft);
        g.setFont (Font (FontOptions (12.0f)));
        g.setColour (Palette::textDim);
        g.drawText (String (sample->getLengthSeconds(), 2) + " s  |  " + String (sample->sampleRate / 1000.0, 1) + " kHz"
                        + "  |  " + String (processor.engine.getNumActiveVoices()) + " voices, "
                        + String (numGrains) + " grains",
                    getWidth() / 2, 6, getWidth() / 2 - 14, 18, Justification::centredRight);

        if (isMouseOver() && ! dragging)
        {
            g.setColour (Palette::textDim.withAlpha (0.8f));
            g.drawText ("click / drag = position   |   scroll = spray   |   drop a file to replace",
                        getLocalBounds().removeFromBottom (18).reduced (14, 0), Justification::centredRight);
        }
    }

    if (message.isNotEmpty())
    {
        auto box = getLocalBounds().toFloat().withSizeKeepingCentre (jmin (560.0f, (float) getWidth() - 40.0f), 44.0f);
        g.setColour (Palette::header.withAlpha (0.92f));
        g.fillRoundedRectangle (box, 8.0f);
        g.setColour (Palette::fx);
        g.drawRoundedRectangle (box, 8.0f, 1.0f);
        g.setColour (Palette::text);
        g.setFont (Font (FontOptions (13.0f)));
        g.drawFittedText (message, box.toNearestInt().reduced (10, 4), Justification::centred, 2);
    }

    if (dropHighlight)
    {
        const float pulse = 0.5f + 0.5f * std::sin (phase * 6.0f);
        g.setColour (Palette::grain.withAlpha (0.08f + 0.06f * pulse));
        g.fillRoundedRectangle (bounds, 10.0f);
        g.setColour (Palette::grain);
        g.drawRoundedRectangle (bounds.reduced (2.0f), 10.0f, 2.5f);
        g.setFont (Font (FontOptions (20.0f, Font::bold)));
        g.drawText ("Drop to load sample", getLocalBounds(), Justification::centred);
    }
    else
    {
        g.setColour (Palette::panelEdge);
        g.drawRoundedRectangle (bounds.reduced (0.5f), 10.0f, 1.0f);
    }
}

void WaveformView::setPositionFromX (float x)
{
    const auto area = waveArea();
    const float v = jlimit (0.0f, 1.0f, (x - area.getX()) / area.getWidth());
    if (auto* p = processor.apvts.getParameter (ParamIDs::position))
        p->setValueNotifyingHost (p->convertTo0to1 (v));
}

void WaveformView::mouseDown (const MouseEvent& e)
{
    if (sample == nullptr) return;
    dragging = true;
    if (auto* p = processor.apvts.getParameter (ParamIDs::position)) p->beginChangeGesture();
    setPositionFromX ((float) e.x);
}

void WaveformView::mouseDrag (const MouseEvent& e)
{
    if (dragging) setPositionFromX ((float) e.x);
}

void WaveformView::mouseUp (const MouseEvent&)
{
    if (! dragging) return;
    dragging = false;
    if (auto* p = processor.apvts.getParameter (ParamIDs::position)) p->endChangeGesture();
}

void WaveformView::mouseMove (const MouseEvent&)
{
    setMouseCursor (sample != nullptr ? MouseCursor::IBeamCursor : MouseCursor::NormalCursor);
}

void WaveformView::mouseWheelMove (const MouseEvent&, const MouseWheelDetails& wheel)
{
    if (auto* p = processor.apvts.getParameter (ParamIDs::spray))
    {
        const float delta = (std::abs (wheel.deltaY) > 0.0f ? wheel.deltaY : wheel.deltaX) * (wheel.isReversed ? -1.0f : 1.0f);
        p->beginChangeGesture();
        p->setValueNotifyingHost (jlimit (0.0f, 1.0f, p->getValue() + delta * 0.15f));
        p->endChangeGesture();
    }
}
