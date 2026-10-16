#include "PluginEditor.h"

#include "Parameters.h"
#include "dsp/FormantRouting.h"
#include "ui/ValueFormat.h"

#include <array>
#include <cmath>

namespace
{
constexpr int kFrame = 22;                                  // thickness of the wooden frame
constexpr int kEditorWidth = 624;
constexpr int kEditorHeight = 364;
constexpr int kColumnLeft = kFrame + 30;
constexpr int kColumnWidth = 130;
constexpr int kColumnTop = kFrame + 64;
constexpr int kColumnHeight = 144;

const juce::Colour woodLight { 0xffa8622d };
const juce::Colour woodDark  { 0xff5a2c14 };

void setUpKnob (juce::Slider& slider)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 108, 26);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                juce::MathConstants<float>::pi * 2.75f, true);
}

double parseNumber (const juce::String& text)
{
    return text.retainCharacters ("+-.0123456789").getDoubleValue();
}

void drawScrew (juce::Graphics& g, juce::Point<float> centre, float slotAngle)
{
    constexpr float r = 6.0f;

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff8a8a86), centre.x - r, centre.y - r,
                                             juce::Colour (0xff2e2e2c), centre.x + r, centre.y + r, false));
    g.fillEllipse (centre.x - r, centre.y - r, r * 2.0f, r * 2.0f);

    const auto d = juce::Point<float> (std::cos (slotAngle), std::sin (slotAngle)) * (r - 1.5f);
    g.setColour (juce::Colours::black.withAlpha (0.75f));
    g.drawLine (juce::Line<float> (centre - d, centre + d), 1.6f);
}

// An engraved rule: a dark line with a light line just below it.
void drawRule (juce::Graphics& g, float x1, float x2, float y)
{
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawLine (x1, y, x2, y, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawLine (x1, y + 1.0f, x2, y + 1.0f, 1.0f);
}

// One side of the frame: wood grain, clipped to `shape` (a mitred trapezoid).
void drawWoodBar (juce::Graphics& g, const juce::Path& shape, juce::Rectangle<float> area, bool grainAlongX)
{
    juce::Graphics::ScopedSaveState saved (g);
    g.reduceClipRegion (shape);

    g.setGradientFill (juce::ColourGradient (woodLight, area.getX(), area.getY(),
                                             woodDark,
                                             grainAlongX ? area.getX() : area.getRight(),
                                             grainAlongX ? area.getBottom() : area.getY(), false));
    g.fillRect (area);

    juce::Random random (grainAlongX ? 17 : 29);            // fixed seed: the grain never shimmers
    const float length = grainAlongX ? area.getWidth() : area.getHeight();
    const float across = grainAlongX ? area.getHeight() : area.getWidth();

    for (int i = 0; i < 30; ++i)
    {
        const float position = random.nextFloat() * across;
        const float amplitude = 0.5f + random.nextFloat() * 1.8f;
        const float phase = random.nextFloat() * juce::MathConstants<float>::twoPi;
        const float frequency = 0.008f + random.nextFloat() * 0.012f;
        const float alpha = 0.05f + random.nextFloat() * 0.16f;
        const bool dark = random.nextBool();

        juce::Path line;
        constexpr int steps = 30;

        for (int s = 0; s <= steps; ++s)
        {
            const float along = length * static_cast<float> (s) / static_cast<float> (steps);
            const float offset = amplitude * std::sin (phase + along * frequency);
            const auto p = grainAlongX ? juce::Point<float> (area.getX() + along, area.getY() + position + offset)
                                       : juce::Point<float> (area.getX() + position + offset, area.getY() + along);

            if (s == 0)
                line.startNewSubPath (p);
            else
                line.lineTo (p);
        }

        g.setColour ((dark ? juce::Colours::black : woodLight.brighter (0.4f)).withAlpha (alpha));
        g.strokePath (line, juce::PathStrokeType (0.6f + random.nextFloat() * 1.4f));
    }
}

// A picture-frame style wooden border with mitred corners.
void drawWoodFrame (juce::Graphics& g, juce::Rectangle<float> outer, float t)
{
    const float x0 = outer.getX(), y0 = outer.getY(), x1 = outer.getRight(), y1 = outer.getBottom();

    const auto quad = [] (juce::Point<float> a, juce::Point<float> b, juce::Point<float> c, juce::Point<float> d)
    {
        juce::Path p;
        p.startNewSubPath (a);
        p.lineTo (b);
        p.lineTo (c);
        p.lineTo (d);
        p.closeSubPath();
        return p;
    };

    drawWoodBar (g, quad ({ x0, y0 }, { x1, y0 }, { x1 - t, y0 + t }, { x0 + t, y0 + t }),
                 outer.withHeight (t), true);                                   // top
    drawWoodBar (g, quad ({ x0, y1 }, { x0 + t, y1 - t }, { x1 - t, y1 - t }, { x1, y1 }),
                 outer.withTop (y1 - t), true);                                 // bottom
    drawWoodBar (g, quad ({ x0, y0 }, { x0 + t, y0 + t }, { x0 + t, y1 - t }, { x0, y1 }),
                 outer.withWidth (t), false);                                   // left
    drawWoodBar (g, quad ({ x1, y0 }, { x1, y1 }, { x1 - t, y1 - t }, { x1 - t, y0 + t }),
                 outer.withLeft (x1 - t), false);                               // right

    // Mitre seams, and light catching the lacquered outer edge
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.drawLine (x0, y0, x0 + t, y0 + t, 1.2f);
    g.drawLine (x1, y0, x1 - t, y0 + t, 1.2f);
    g.drawLine (x0, y1, x0 + t, y1 - t, 1.2f);
    g.drawLine (x1, y1, x1 - t, y1 - t, 1.2f);

    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.drawRect (outer.reduced (0.5f), 1.0f);
}
} // namespace

PluginEditor::PluginEditor (PluginProcessor& p)
    : juce::AudioProcessorEditor (&p),
      pluginProcessor (p),
      pitchAttachment (p.getValueTreeState(), params::pitchId, pitchSlider),
      formantAttachment (p.getValueTreeState(), params::formantId, formantSlider),
      mixAttachment (p.getValueTreeState(), params::mixId, mixSlider),
      outputAttachment (p.getValueTreeState(), params::outputId, outputSlider),
      linkAttachment (p.getValueTreeState(), params::formantLinkId, linkButton),
      pitchValue (p.getValueTreeState().getRawParameterValue (params::pitchId)),
      formantValue (p.getValueTreeState().getRawParameterValue (params::formantId)),
      linkValue (p.getValueTreeState().getRawParameterValue (params::formantLinkId))
{
    setLookAndFeel (&lookAndFeel);

    for (auto* slider : { &pitchSlider, &formantSlider, &mixSlider, &outputSlider })
    {
        setUpKnob (*slider);
        addAndMakeVisible (*slider);
    }

    addAndMakeVisible (linkButton);

    // Readouts show digits only; the unit is printed beside them by the look-and-feel, which
    // reads it (and the number of digit cells) from the slider. These replace the text
    // functions the attachments install, so they are set afterwards.
    struct KnobStyle
    {
        juce::Slider* slider;
        const char* unit;
        int cells;
        int decimals;
    };

    const std::array<KnobStyle, 4> styles { { { &pitchSlider, "ST", 5, 2 },
                                              { &formantSlider, "ST", 5, 2 },
                                              { &mixSlider, "%", 3, 0 },
                                              { &outputSlider, "DB", 4, 1 } } };

    for (const auto& style : styles)
    {
        const int decimals = style.decimals;

        // The unit goes in a property, not setTextValueSuffix: JUCE appends the suffix to
        // the displayed text, which put the unit letters into the digit cells.
        style.slider->getProperties().set ("unit", style.unit);
        style.slider->getProperties().set ("cells", style.cells);
        style.slider->textFromValueFunction = [decimals] (double v)
        {
            return juce::String (valueFormat::formatValue (v, decimals));
        };
        style.slider->valueFromTextFunction = parseNumber;
        style.slider->updateText();
    }

    setResizable (false, false);
    setSize (kEditorWidth, kEditorHeight);

    timerCallback();      // fill in the display state before the first paint
    startTimerHz (30);
}

PluginEditor::~PluginEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void PluginEditor::timerCallback()
{
    const bool link = linkValue->load() >= 0.5f;
    const auto effective = fsdsp::effectiveFormant (pitchValue->load(), formantValue->load(), link);

    if (link != shownLink || effective.clamped != shownClamped
        || ! juce::exactlyEqual (effective.semitones, shownFormant))
    {
        shownLink = link;
        shownClamped = effective.clamped;
        shownFormant = effective.semitones;
        repaint();
    }
}

void PluginEditor::resized()
{
    const std::array<juce::Slider*, 4> sliders { &pitchSlider, &formantSlider, &mixSlider, &outputSlider };

    for (std::size_t i = 0; i < sliders.size(); ++i)
    {
        columns[i] = juce::Rectangle<int> (kColumnLeft + static_cast<int> (i) * kColumnWidth,
                                           kColumnTop, kColumnWidth, kColumnHeight);
        sliders[i]->setBounds (columns[i]);
    }

    linkButton.setBounds (kColumnLeft, kFrame + 258, 170, 36);
    displayBounds = juce::Rectangle<int> (kEditorWidth - kColumnLeft - 220, kFrame + 252, 220, 50);
}

void PluginEditor::paint (juce::Graphics& g)
{
    const auto outer = getLocalBounds().toFloat();
    const auto plate = outer.reduced (static_cast<float> (kFrame));

    // Faceplate
    g.setGradientFill (juce::ColourGradient (retro::faceplateTop, 0.0f, plate.getY(),
                                             retro::faceplateBottom, 0.0f, plate.getBottom(), false));
    g.fillRect (plate);

    drawWoodFrame (g, outer, static_cast<float> (kFrame));

    // The faceplate sits slightly recessed in the frame.
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawRect (plate, 2.0f);

    // Corner screws, each with a different slot angle
    constexpr float inset = 14.0f;
    drawScrew (g, { plate.getX() + inset, plate.getY() + inset }, 0.5f);
    drawScrew (g, { plate.getRight() - inset, plate.getY() + inset }, 2.1f);
    drawScrew (g, { plate.getX() + inset, plate.getBottom() - inset }, 1.2f);
    drawScrew (g, { plate.getRight() - inset, plate.getBottom() - inset }, 0.1f);

    // Title
    g.setColour (retro::cream);
    g.setFont (retro::boldFont (22.0f));
    g.drawText ("F O R M A N T   S H I F T E R",
                juce::Rectangle<float> (plate.getX(), plate.getY() + 12.0f, plate.getWidth(), 28.0f),
                juce::Justification::centred);
    drawRule (g, plate.getX() + 30.0f, plate.getRight() - 30.0f, plate.getY() + 48.0f);

    // Knob labels
    g.setFont (retro::boldFont (12.0f));
    const std::array<const char*, 4> labels { "PITCH", shownLink ? "FORMANT OFFSET" : "FORMANT", "MIX", "OUTPUT" };

    for (std::size_t i = 0; i < labels.size(); ++i)
    {
        g.setColour (retro::cream);
        g.drawText (labels[i], columns[i].withY (columns[i].getBottom() + 4).withHeight (18),
                    juce::Justification::centred);
    }

    drawRule (g, plate.getX() + 30.0f, plate.getRight() - 30.0f, plate.getY() + 240.0f);

    // Effective formant display
    const auto display = displayBounds.toFloat();

    g.setColour (retro::creamDim);
    g.setFont (retro::boldFont (10.0f));
    g.drawText ("FORMANT TOTAL", displayBounds.withY (displayBounds.getY() - 12).withHeight (12),
                juce::Justification::centredLeft);

    g.setColour (retro::ledGlass);
    g.fillRoundedRectangle (display, 4.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.07f), display.getX(), display.getY(),
                                             juce::Colours::transparentWhite, display.getX(), display.getCentreY(), false));
    g.fillRoundedRectangle (display, 4.0f);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (display, 4.0f, 1.5f);

    const auto inner = display.reduced (10.0f, 8.0f);

    retro::drawSevenSegment (g, inner.withWidth (120.0f),
                             juce::String (valueFormat::formatValue (static_cast<double> (shownFormant), 2)), 5);

    g.setColour (retro::creamDim);
    g.setFont (retro::boldFont (11.0f));
    g.drawText ("ST", juce::Rectangle<float> (inner.getX() + 126.0f, inner.getY(), 24.0f, inner.getHeight()),
                juce::Justification::centredLeft);

    // Limit lamp
    const auto lamp = juce::Point<float> (display.getRight() - 30.0f, display.getCentreY() - 6.0f);

    if (shownClamped)
    {
        g.setColour (retro::ledRed.withAlpha (0.28f));
        g.fillEllipse (lamp.x - 9.0f, lamp.y - 9.0f, 18.0f, 18.0f);
    }

    g.setColour (shownClamped ? retro::ledRed : retro::ledRedDim);
    g.fillEllipse (lamp.x - 5.0f, lamp.y - 5.0f, 10.0f, 10.0f);
    g.setColour (shownClamped ? retro::ledRed : retro::creamDim.withAlpha (0.6f));
    g.setFont (retro::boldFont (9.0f));
    g.drawText ("LIMIT", juce::Rectangle<float> (lamp.x - 20.0f, lamp.y + 7.0f, 40.0f, 12.0f),
                juce::Justification::centred);
}