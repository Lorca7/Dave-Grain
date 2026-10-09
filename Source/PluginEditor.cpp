#include "PluginEditor.h"
#include "dsp/BuiltinSources.h"

using namespace juce;
namespace ID = ParamIDs;

// =============================================================================================
// Small visual helpers
// =============================================================================================
namespace
{
    /** Draws the current grain window shape. */
    class WindowPreview : public Component, private Timer
    {
    public:
        explicit WindowPreview (APVTS& s) : state (s) { startTimerHz (15); setInterceptsMouseClicks (false, false); }

        void paint (Graphics& g) override
        {
            auto r = getLocalBounds().toFloat().reduced (4.0f, 6.0f);
            g.setColour (Palette::well);
            g.fillRoundedRectangle (r, 6.0f);
            r = r.reduced (8.0f, 8.0f);

            Path p;
            const int steps = 80;
            for (int i = 0; i <= steps; ++i)
            {
                const float x = (float) i / (float) steps;
                const float y = GrainEngine::windowValue (shape, x);
                const float px = r.getX() + x * r.getWidth(), py = r.getBottom() - y * r.getHeight();
                if (i == 0) p.startNewSubPath (px, py); else p.lineTo (px, py);
            }
            Path fill (p);
            fill.lineTo (r.getRight(), r.getBottom());
            fill.lineTo (r.getX(), r.getBottom());
            fill.closeSubPath();

            g.setGradientFill (ColourGradient (Palette::window.withAlpha (0.45f), 0.0f, r.getY(),
                                               Palette::window.withAlpha (0.05f), 0.0f, r.getBottom(), false));
            g.fillPath (fill);
            g.setColour (Palette::window);
            g.strokePath (p, PathStrokeType (2.0f));
        }

    private:
        void timerCallback() override
        {
            const int s = (int) state.getRawParameterValue (ID::window)->load();
            if (s != shape) { shape = s; repaint(); }
        }

        APVTS& state;
        int shape = -1;
    };

    /** Draws the amp envelope. */
    class AdsrView : public Component, private Timer
    {
    public:
        explicit AdsrView (APVTS& s) : state (s) { startTimerHz (15); setInterceptsMouseClicks (false, false); }

        void paint (Graphics& g) override
        {
            auto r = getLocalBounds().toFloat().reduced (4.0f, 6.0f);
            g.setColour (Palette::well);
            g.fillRoundedRectangle (r, 6.0f);
            r = r.reduced (8.0f, 8.0f);

            // Compress times so short and long envelopes both read well.
            auto t = [] (float seconds) { return std::sqrt (seconds); };
            const float a = t (values[0]), d = t (values[1]), sus = values[2], rel = t (values[3]);
            const float hold = 0.6f;
            const float total = a + d + hold + rel;
            auto X = [&] (float v) { return r.getX() + v / total * r.getWidth(); };
            auto Y = [&] (float v) { return r.getBottom() - v * r.getHeight(); };

            Path p;
            p.startNewSubPath (X (0), Y (0));
            p.lineTo (X (a), Y (1));
            p.quadraticTo (X (a + d * 0.2f), Y (sus), X (a + d), Y (sus));
            p.lineTo (X (a + d + hold), Y (sus));
            p.quadraticTo (X (a + d + hold + rel * 0.2f), Y (0), X (total), Y (0));

            Path fill (p);
            fill.closeSubPath();
            g.setColour (Palette::amp.withAlpha (0.18f));
            g.fillPath (fill);
            g.setColour (Palette::amp);
            g.strokePath (p, PathStrokeType (2.0f));
        }

    private:
        void timerCallback() override
        {
            const float now[4] = { state.getRawParameterValue (ID::attack)->load(), state.getRawParameterValue (ID::decay)->load(),
                                   state.getRawParameterValue (ID::sustain)->load(), state.getRawParameterValue (ID::release)->load() };
            if (std::memcmp (now, values, sizeof (values)) != 0) { std::memcpy (values, now, sizeof (values)); repaint(); }
        }

        APVTS& state;
        float values[4] {};
    };

    /** Stereo peak meter. */
    class LevelMeter : public Component, private Timer
    {
    public:
        explicit LevelMeter (DaveGrainProcessor& p) : processor (p) { startTimerHz (30); }

        void paint (Graphics& g) override
        {
            auto r = getLocalBounds().toFloat().reduced (10.0f, 6.0f);
            const float w = (r.getWidth() - 4.0f) * 0.5f;
            drawBar (g, r.removeFromLeft (w), left);
            r.removeFromLeft (4.0f);
            drawBar (g, r, right);
        }

    private:
        static void drawBar (Graphics& g, Rectangle<float> r, float level)
        {
            g.setColour (Palette::well);
            g.fillRoundedRectangle (r, 3.0f);
            const float db = Decibels::gainToDecibels (level, -60.0f);
            const float h = jmap (db, -60.0f, 0.0f, 0.0f, r.getHeight());
            auto bar = r.withTop (r.getBottom() - h);
            g.setGradientFill (ColourGradient (Palette::grain, 0.0f, r.getBottom(), Palette::fx, 0.0f, r.getY(), false));
            g.fillRoundedRectangle (bar, 3.0f);
        }

        void timerCallback() override
        {
            left = processor.meterLeft.load();
            right = processor.meterRight.load();
            repaint();
        }

        DaveGrainProcessor& processor;
        float left = 0.0f, right = 0.0f;
    };

    /** Pulsing dot that shows an LFO's current value. */
    class LfoIndicator : public Component, private Timer
    {
    public:
        LfoIndicator (DaveGrainProcessor& p, int i) : processor (p), index (i) { startTimerHz (30); setInterceptsMouseClicks (false, false); }

        void paint (Graphics& g) override
        {
            auto r = getLocalBounds().toFloat().reduced (1.0f);
            g.setColour (Palette::well);
            g.fillEllipse (r);
            g.setColour (Palette::lfo.withAlpha (0.15f + 0.85f * (0.5f + 0.5f * value)));
            g.fillEllipse (r.reduced (2.0f));
        }

    private:
        void timerCallback() override
        {
            value = processor.lfoDisplay[index].load();
            repaint();
        }

        DaveGrainProcessor& processor;
        int index;
        float value = 0.0f;
    };
}

// =============================================================================================
MainView::MainView (DaveGrainProcessor& p)
    : waveform (p), processor (p), state (p.apvts), presetBar (p),
      grainPanel ("Grain", Palette::grain, 4),
      pitchPanel ("Pitch", Palette::pitch, 3),
      windowPanel ("Window", Palette::window, 3),
      ampPanel ("Amp Envelope", Palette::amp, 4),
      filterPanel ("Filter", Palette::filter, 2),
      lfo1Panel ("LFO 1", Palette::lfo, 3),
      lfo2Panel ("LFO 2", Palette::lfo, 3),
      fxPanel ("Delay  /  Reverb", Palette::fx, 4),
      outPanel ("Output", Palette::output, 1),

      position (state, ID::position, "Position", Palette::grain),
      spray (state, ID::spray, "Spray", Palette::grain),
      scan (state, ID::scan, "Scan", Palette::grain),
      size (state, ID::size, "Size", Palette::grain),
      sizeRand (state, ID::sizeRand, "Size Var", Palette::grain),
      density (state, ID::density, "Density", Palette::grain),
      jitter (state, ID::jitter, "Jitter", Palette::grain),
      freeze (state, ID::freeze, "Freeze", "Freeze", Palette::grain),

      pitch (state, ID::pitch, "Pitch", Palette::pitch),
      fine (state, ID::fine, "Fine", Palette::pitch),
      pitchRand (state, ID::pitchRand, "Random", Palette::pitch),
      rootNote (state, ID::rootNote, "Root", Palette::pitch),
      quantize (state, ID::quantize, "Scale", Palette::pitch),
      keyTrack (state, ID::keyTrack, "Key Track", "On", Palette::pitch),

      windowShape (state, ID::window, "Shape", Palette::window),
      reverse (state, ID::reverse, "Reverse", Palette::window),
      spread (state, ID::spread, "Spread", Palette::window),

      attack (state, ID::attack, "Attack", Palette::amp),
      decay (state, ID::decay, "Decay", Palette::amp),
      sustain (state, ID::sustain, "Sustain", Palette::amp),
      release (state, ID::release, "Release", Palette::amp),
      velocity (state, ID::velocity, "Velocity", Palette::amp),
      hold (state, ID::hold, "Hold", "Hold", Palette::amp),

      filterType (state, ID::filterType, "Type", Palette::filter),
      cutoff (state, ID::cutoff, "Cutoff", Palette::filter),
      resonance (state, ID::resonance, "Resonance", Palette::filter),

      lfo1Rate (state, ID::lfo1Rate, "Rate", Palette::lfo),
      lfo1Depth (state, ID::lfo1Depth, "Depth", Palette::lfo),
      lfo2Rate (state, ID::lfo2Rate, "Rate", Palette::lfo),
      lfo2Depth (state, ID::lfo2Depth, "Depth", Palette::lfo),
      lfo1Div (state, ID::lfo1Div, "Rate", Palette::lfo),
      lfo1Shape (state, ID::lfo1Shape, "Shape", Palette::lfo),
      lfo1Target (state, ID::lfo1Target, "Target", Palette::lfo),
      lfo2Div (state, ID::lfo2Div, "Rate", Palette::lfo),
      lfo2Shape (state, ID::lfo2Shape, "Shape", Palette::lfo),
      lfo2Target (state, ID::lfo2Target, "Target", Palette::lfo),
      lfo1Sync (state, ID::lfo1Sync, "Sync", "Sync", Palette::lfo),
      lfo2Sync (state, ID::lfo2Sync, "Sync", "Sync", Palette::lfo),

      delayTime (state, ID::delayTime, "Time", Palette::fx),
      delayFb (state, ID::delayFb, "Feedback", Palette::fx),
      delayMix (state, ID::delayMix, "Delay", Palette::fx),
      reverbSize (state, ID::reverbSize, "Size", Palette::fx),
      reverbDamp (state, ID::reverbDamp, "Damping", Palette::fx),
      reverbMix (state, ID::reverbMix, "Reverb", Palette::fx),
      delayPing (state, ID::delayPing, "Ping-Pong", "On", Palette::fx),

      output (state, ID::output, "Volume", Palette::output),
      keyboard (p.keyboardState, MidiKeyboardComponent::horizontalKeyboard)
{
    addAndMakeVisible (waveform);
    addAndMakeVisible (presetBar);
    presetBar.onMessage = [this] (const String& m) { waveform.showMessage (m); };

    loadButton.setColour (TextButton::buttonOnColourId, Palette::grain);
    loadButton.setTooltip ("Load a WAV / AIFF / FLAC file, or pick a built-in source. You can also drag files onto the plug-in.");
    loadButton.onClick = [this] { if (onLoadSample) onLoadSample(); };
    addAndMakeVisible (loadButton);

    // ---- Grain ----
    grainPanel.place (position, 0, 0); grainPanel.place (spray, 1, 0); grainPanel.place (scan, 2, 0); grainPanel.place (freeze, 3, 0);
    grainPanel.place (size, 0, 1);     grainPanel.place (sizeRand, 1, 1); grainPanel.place (density, 2, 1); grainPanel.place (jitter, 3, 1);

    // ---- Pitch ----
    pitchPanel.place (pitch, 0, 0); pitchPanel.place (fine, 1, 0); pitchPanel.place (pitchRand, 2, 0);
    pitchPanel.place (quantize, 0, 1); pitchPanel.place (keyTrack, 1, 1); pitchPanel.place (rootNote, 2, 1);

    // ---- Window ----
    auto windowPreview = std::make_unique<WindowPreview> (state);
    windowPanel.place (*windowPreview, 0, 0, 2); windowPanel.place (reverse, 2, 0);
    windowPanel.place (windowShape, 0, 1, 2);    windowPanel.place (spread, 2, 1);
    decorations.push_back (std::move (windowPreview));

    // ---- Amp ----
    auto adsr = std::make_unique<AdsrView> (state);
    ampPanel.place (attack, 0, 0); ampPanel.place (decay, 1, 0); ampPanel.place (sustain, 2, 0); ampPanel.place (release, 3, 0);
    ampPanel.place (velocity, 0, 1); ampPanel.place (hold, 1, 1); ampPanel.place (*adsr, 2, 1, 2);
    decorations.push_back (std::move (adsr));

    // ---- Filter ----
    filterPanel.place (filterType, 0, 0, 2);
    filterPanel.place (cutoff, 0, 1); filterPanel.place (resonance, 1, 1);

    // ---- LFOs ----
    auto setupLfo = [this] (Panel& panel, Knob& rate, Choice& div, Knob& depth, Toggle& sync, Choice& shape, Choice& target, int index)
    {
        panel.place (rate, 0, 0); panel.place (div, 0, 0); panel.place (depth, 1, 0); panel.place (sync, 2, 0);
        panel.place (shape, 0, 1); panel.place (target, 1, 1, 2);
        auto dot = std::make_unique<LfoIndicator> (processor, index);
        panel.addAndMakeVisible (*dot);
        decorations.push_back (std::move (dot));
    };
    setupLfo (lfo1Panel, lfo1Rate, lfo1Div, lfo1Depth, lfo1Sync, lfo1Shape, lfo1Target, 0);
    setupLfo (lfo2Panel, lfo2Rate, lfo2Div, lfo2Depth, lfo2Sync, lfo2Shape, lfo2Target, 1);

    // ---- FX ----
    fxPanel.place (delayTime, 0, 0);  fxPanel.place (delayFb, 1, 0);    fxPanel.place (delayMix, 2, 0);  fxPanel.place (delayPing, 3, 0);
    fxPanel.place (reverbSize, 0, 1); fxPanel.place (reverbDamp, 1, 1); fxPanel.place (reverbMix, 2, 1);

    // ---- Output ----
    auto meter = std::make_unique<LevelMeter> (processor);
    outPanel.place (output, 0, 0); outPanel.place (*meter, 0, 1);
    decorations.push_back (std::move (meter));

    for (auto* panel : { &grainPanel, &pitchPanel, &windowPanel, &ampPanel, &filterPanel, &lfo1Panel, &lfo2Panel, &fxPanel, &outPanel })
        addAndMakeVisible (panel);

    // ---- Keyboard ----
    keyboard.setAvailableRange (24, 96);
    keyboard.setLowestVisibleKey (24);
    keyboard.setColour (MidiKeyboardComponent::whiteNoteColourId, Colour (0xffd8dae3));
    keyboard.setColour (MidiKeyboardComponent::blackNoteColourId, Colour (0xff1b1c23));
    keyboard.setColour (MidiKeyboardComponent::keySeparatorLineColourId, Colour (0xff8b90a5));
    keyboard.setColour (MidiKeyboardComponent::keyDownOverlayColourId, Palette::grain);
    keyboard.setColour (MidiKeyboardComponent::mouseOverKeyOverlayColourId, Palette::grain.withAlpha (0.3f));
    keyboard.setColour (MidiKeyboardComponent::textLabelColourId, Colour (0xff4a4d5c));
    keyboard.setOctaveForMiddleC (3); // Ableton naming: middle C = C3
    keyboard.setWantsKeyboardFocus (false);
    addAndMakeVisible (keyboard);

    startTimerHz (15);
    timerCallback();
}

MainView::~MainView() = default;

void MainView::timerCallback()
{
    const bool sync1 = state.getRawParameterValue (ID::lfo1Sync)->load() > 0.5f;
    const bool sync2 = state.getRawParameterValue (ID::lfo2Sync)->load() > 0.5f;
    lfo1Rate.setVisible (! sync1); lfo1Div.setVisible (sync1);
    lfo2Rate.setVisible (! sync2); lfo2Div.setVisible (sync2);

    const bool filterOn = state.getRawParameterValue (ID::filterType)->load() > 0.5f;
    cutoff.setAlpha (filterOn ? 1.0f : 0.4f);
    resonance.setAlpha (filterOn ? 1.0f : 0.4f);

    const bool keyTrackOn = state.getRawParameterValue (ID::keyTrack)->load() > 0.5f;
    rootNote.setAlpha (keyTrackOn ? 1.0f : 0.4f);
}

void MainView::paint (Graphics& g)
{
    g.fillAll (Palette::background);

    auto header = getLocalBounds().removeFromTop (56);
    g.setColour (Palette::header);
    g.fillRect (header);
    g.setColour (Palette::panelEdge);
    g.drawHorizontalLine (header.getBottom() - 1, 0.0f, (float) getWidth());

    // Logo: a little cloud of grains + wordmark
    Random r (42);
    for (int i = 0; i < 14; ++i)
    {
        const float x = 22.0f + r.nextFloat() * 30.0f, y = 16.0f + r.nextFloat() * 24.0f, s = 2.0f + r.nextFloat() * 4.0f;
        g.setColour ((i % 3 == 0 ? Palette::window : Palette::grain).withAlpha (0.5f + 0.5f * r.nextFloat()));
        g.fillEllipse (x, y, s, s);
    }
    g.setColour (Palette::text);
    g.setFont (Font (FontOptions (22.0f, Font::bold)).withExtraKerningFactor (0.18f));
    g.drawText ("DAVE GRAIN", 64, 7, 220, 28, Justification::centredLeft);
    g.setColour (Palette::textDim);
    g.setFont (Font (FontOptions (11.0f)).withExtraKerningFactor (0.1f));
    g.drawText ("GRANULAR SYNTHESIZER", 66, 35, 220, 14, Justification::centredLeft);
}

void MainView::resized()
{
    auto r = getLocalBounds();
    auto header = r.removeFromTop (56).reduced (16, 12);
    header.removeFromLeft (240);
    loadButton.setBounds (header.removeFromRight (120));
    header.removeFromRight (16);
    presetBar.setBounds (header);

    r.reduce (16, 0);
    r.removeFromTop (10);
    waveform.setBounds (r.removeFromTop (210));
    r.removeFromTop (10);

    keyboard.setBounds (r.removeFromBottom (68).withTrimmedBottom (10));
    keyboard.setKeyWidth ((float) keyboard.getWidth() / 43.0f); // 43 white keys in C1..C7
    r.removeFromBottom (8);

    constexpr int gap = 10;
    auto layoutRow = [] (Rectangle<int> row, std::initializer_list<Panel*> panels)
    {
        int totalCols = 0;
        for (auto* p : panels) totalCols += p->getColumns();
        const float colW = (float) (row.getWidth() - gap * ((int) panels.size() - 1)) / (float) totalCols;
        float x = (float) row.getX();
        for (auto* p : panels)
        {
            const float w = colW * (float) p->getColumns();
            p->setBounds (Rectangle<float> (x, (float) row.getY(), w, (float) row.getHeight()).toNearestInt());
            x += w + (float) gap;
        }
    };

    const int rowH = (r.getHeight() - gap) / 2;
    layoutRow (r.removeFromTop (rowH), { &grainPanel, &pitchPanel, &windowPanel, &ampPanel });
    r.removeFromTop (gap);
    layoutRow (r, { &filterPanel, &lfo1Panel, &lfo2Panel, &fxPanel, &outPanel });

    // LFO indicators sit in the panel title bar
    for (auto& d : decorations)
        if (auto* dot = dynamic_cast<LfoIndicator*> (d.get()))
            if (auto* parent = dot->getParentComponent())
                dot->setBounds (parent->getWidth() - 26, 10, 14, 14);
}

// =============================================================================================
DaveGrainEditor::DaveGrainEditor (DaveGrainProcessor& p)
    : AudioProcessorEditor (&p), processor (p), view (p)
{
    setLookAndFeel (&lookAndFeel);
    LookAndFeel::setDefaultLookAndFeel (&lookAndFeel); // popup menus / alerts
    addAndMakeVisible (view);
    view.onLoadSample = [this] { showLoadMenu(); };

    constexpr int w = MainView::designWidth, h = MainView::designHeight;
    const float scale = jlimit (0.5f, 2.0f, processor.uiScale); // read before setResizeLimits() triggers resized()
    setResizable (true, true);
    setResizeLimits (w / 2, h / 2, w * 2, h * 2);
    getConstrainer()->setFixedAspectRatio ((double) w / (double) h);

    setSize (roundToInt ((float) w * scale), roundToInt ((float) h * scale));
}

DaveGrainEditor::~DaveGrainEditor()
{
    LookAndFeel::setDefaultLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

void DaveGrainEditor::paint (Graphics& g)
{
    g.fillAll (Palette::background);
}

void DaveGrainEditor::resized()
{
    const float scale = (float) getWidth() / (float) MainView::designWidth;
    processor.uiScale = scale;
    view.setBounds (0, 0, MainView::designWidth, MainView::designHeight);
    view.setTransform (AffineTransform::scale (scale));
}

bool DaveGrainEditor::isInterestedInFileDrag (const StringArray& files)
{
    for (auto& f : files)
        if (DaveGrainProcessor::isSupportedAudioFile (f))
            return true;
    return false;
}

void DaveGrainEditor::fileDragEnter (const StringArray&, int, int) { view.waveform.setDropHighlight (true); }
void DaveGrainEditor::fileDragExit (const StringArray&)            { view.waveform.setDropHighlight (false); }

void DaveGrainEditor::filesDropped (const StringArray& files, int, int)
{
    view.waveform.setDropHighlight (false);
    for (auto& f : files)
        if (DaveGrainProcessor::isSupportedAudioFile (f))
        {
            loadFile (File (f));
            return;
        }
}

void DaveGrainEditor::loadFile (const File& f)
{
    String error;
    if (processor.loadSampleFile (f, error))
        view.waveform.showMessage ("Loaded " + f.getFileName());
    else
        view.waveform.showMessage (error);
}

void DaveGrainEditor::showLoadMenu()
{
    PopupMenu menu;
    menu.addItem (1, "Browse for Audio File...");
    menu.addSeparator();
    menu.addSectionHeader ("Built-in sources");
    const auto builtins = BuiltinSources::names();
    for (int i = 0; i < builtins.size(); ++i)
        menu.addItem (100 + i, builtins[i]);

    if (processor.hasUserSample())
    {
        menu.addSeparator();
        menu.addItem (2, "Show Current Sample in " + String (SystemStats::getOperatingSystemType() & SystemStats::MacOSX ? "Finder" : "File Browser"));
    }

    menu.showMenuAsync (PopupMenu::Options().withTargetComponent (this).withMousePosition(), [this, builtins] (int result)
    {
        if (result == 1)
        {
            auto start = processor.hasUserSample() ? File (processor.getSample()->source).getParentDirectory()
                                                   : File::getSpecialLocation (File::userMusicDirectory);
            chooser = std::make_unique<FileChooser> ("Load a sample to granulate", start, "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3;*.caf;*.m4a");
            chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                                  [this] (const FileChooser& fc)
                                  {
                                      if (fc.getResult() != File()) loadFile (fc.getResult());
                                  });
        }
        else if (result == 2)
        {
            if (auto s = processor.getSample()) File (s->source).revealToUser();
        }
        else if (result >= 100)
        {
            processor.loadBuiltinSample (builtins[result - 100]);
        }
    });
}
