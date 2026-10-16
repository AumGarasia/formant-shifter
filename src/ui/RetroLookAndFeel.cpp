#include "RetroLookAndFeel.h"
#include "SegmentLayout.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

namespace
{

// --- Seven-segment geometry ---------------------------------------------------

juce::Path horizontalSegment (float xA, float xB, float y, float half)
{
    juce::Path p;
    p.startNewSubPath (xA, y);
    p.lineTo (xA + half, y - half);
    p.lineTo (xB - half, y - half);
    p.lineTo (xB, y);
    p.lineTo (xB - half, y + half);
    p.lineTo (xA + half, y + half);
    p.closeSubPath();
    return p;
}

juce::Path verticalSegment (float x, float yA, float yB, float half)
{
    juce::Path p;
    p.startNewSubPath (x, yA);
    p.lineTo (x + half, yA + half);
    p.lineTo (x + half, yB - half);
    p.lineTo (x, yB);
    p.lineTo (x - half, yB - half);
    p.lineTo (x - half, yA + half);
    p.closeSubPath();
    return p;
}

// The seven segments of one digit cell, in the order a b c d e f g.
std::array<juce::Path, 7> cellSegments (float x, float y, float w, float h, float t)
{
    const float half = t * 0.5f;
    const float gap = t * 0.12f;
    const float left = x + half;
    const float right = x + w - half;
    const float top = y + half;
    const float middle = y + h * 0.5f;
    const float bottom = y + h - half;

    return { horizontalSegment (left + half + gap, right - half - gap, top, half),       // a
             verticalSegment (right, top + gap, middle - gap, half),                     // b
             verticalSegment (right, middle + gap, bottom - gap, half),                  // c
             horizontalSegment (left + half + gap, right - half - gap, bottom, half),    // d
             verticalSegment (left, middle + gap, bottom - gap, half),                   // e
             verticalSegment (left, top + gap, middle - gap, half),                      // f
             horizontalSegment (left + half + gap, right - half - gap, middle, half) };  // g
}

/// Bit n of the mask lights segment n (a = bit 0 ... g = bit 6).
unsigned segmentMask (char ch)
{
    constexpr unsigned kA = 1, kB = 2, kC = 4, kD = 8, kE = 16, kF = 32, kG = 64;

    switch (ch)
    {
        case '0': return kA | kB | kC | kD | kE | kF;
        case '1': return kB | kC;
        case '2': return kA | kB | kG | kE | kD;
        case '3': return kA | kB | kG | kC | kD;
        case '4': return kF | kG | kB | kC;
        case '5': return kA | kF | kG | kC | kD;
        case '6': return kA | kF | kG | kE | kC | kD;
        case '7': return kA | kB | kC;
        case '8': return kA | kB | kC | kD | kE | kF | kG;
        case '9': return kA | kB | kC | kD | kF | kG;
        case '-': return kG;
        default:  return 0;
    }
}

} // namespace

void retro::drawSevenSegment (juce::Graphics& g, juce::Rectangle<float> area,
                              const juce::String& text, int numCells)
{
    const auto cellCount = static_cast<std::size_t> (std::max (numCells, 1));
    const auto cells = segmentlayout::layout (text.toStdString(), cellCount);

    // Cell proportions are multiples of the digit height; shrink to fit the area if needed.
    constexpr float cellWidth = 0.56f, cellSpacing = 0.30f;
    const float widthPerHeight = static_cast<float> (cellCount) * cellWidth
                                 + static_cast<float> (cellCount - 1) * cellSpacing;
    const float h = std::min (area.getHeight(), area.getWidth() / widthPerHeight);
    const float w = h * cellWidth;
    const float spacing = h * cellSpacing;
    const float t = h * 0.15f;
    const float left = area.getRight() - widthPerHeight * h;      // right-aligned
    const float top = area.getCentreY() - h * 0.5f;

    // The slight forward slant of old calculator displays, pivoting on the middle row.
    juce::Graphics::ScopedSaveState saved (g);
    g.addTransform (juce::AffineTransform::shear (-0.14f, 0.0f).translated (0.14f * area.getCentreY(), 0.0f));

    for (std::size_t i = 0; i < cellCount; ++i)
    {
        const float x = left + static_cast<float> (i) * (w + spacing);
        const auto segments = cellSegments (x, top, w, h, t);
        const unsigned mask = segmentMask (cells[i].glyph);
        const bool hasDotSpace = i + 1 < cellCount;
        const bool dotLit = hasDotSpace && cells[i].dot;
        const juce::Point<float> dotCentre (x + w + spacing * 0.5f, top + h - t * 0.5f);
        const float dotRadius = t * 0.6f;

        // Unlit segments stay faintly visible, like the real thing.
        g.setColour (retro::ledRed.withAlpha (0.10f));

        for (const auto& segment : segments)
            g.fillPath (segment);

        if (hasDotSpace)
            g.fillEllipse (dotCentre.x - dotRadius, dotCentre.y - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);

        // Lit segments, with a soft glow underneath.
        for (std::size_t s = 0; s < segments.size(); ++s)
        {
            if ((mask & (1u << static_cast<unsigned> (s))) == 0u)
                continue;

            g.setColour (retro::ledRed.withAlpha (0.25f));
            g.strokePath (segments[s], juce::PathStrokeType (t * 0.9f));
            g.setColour (retro::ledRed);
            g.fillPath (segments[s]);
        }

        if (dotLit)
        {
            g.setColour (retro::ledRed.withAlpha (0.25f));
            g.fillEllipse (dotCentre.x - dotRadius * 1.8f, dotCentre.y - dotRadius * 1.8f,
                           dotRadius * 3.6f, dotRadius * 3.6f);
            g.setColour (retro::ledRed);
            g.fillEllipse (dotCentre.x - dotRadius, dotCentre.y - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
        }
    }
}

RetroLookAndFeel::RetroLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, retro::ledRed);
    setColour (juce::Slider::textBoxBackgroundColourId, retro::ledGlass);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, retro::ledRed.withAlpha (0.35f));
}

void RetroLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float startAngle, float endAngle,
                                         juce::Slider& slider)
{
    const auto area = juce::Rectangle<int> (x, y, width, height).toFloat();
    const auto centre = area.getCentre();
    const float radius = std::min (area.getWidth(), area.getHeight()) * 0.5f - 16.0f;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    const bool bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;

    // Scale ticks, printed on the faceplate around the knob. Bipolar knobs get a centre mark.
    constexpr int numTicks = 11;

    for (int i = 0; i < numTicks; ++i)
    {
        const float t = static_cast<float> (i) / static_cast<float> (numTicks - 1);
        const float a = startAngle + t * (endAngle - startAngle);
        const bool major = (i == 0 || i == numTicks - 1 || (bipolar && i == numTicks / 2));
        const auto dir = juce::Point<float> (std::sin (a), -std::cos (a));

        g.setColour (major ? retro::cream : retro::creamDim);
        g.drawLine (juce::Line<float> (centre + dir * (radius + 6.0f),
                                       centre + dir * (radius + (major ? 14.0f : 10.0f))),
                    major ? 2.0f : 1.2f);
    }

    // Drop shadow
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (centre.x - radius + 2.0f, centre.y - radius + 4.0f, radius * 2.0f, radius * 2.0f);

    // Knob body
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a4a4a), centre.x - radius, centre.y - radius,
                                             juce::Colour (0xff0e0e0e), centre.x + radius, centre.y + radius, false));
    g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

    // Knurled grip ring
    g.setColour (juce::Colours::black.withAlpha (0.55f));

    for (int i = 0; i < 48; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * static_cast<float> (i) / 48.0f;
        const auto dir = juce::Point<float> (std::sin (a), -std::cos (a));
        g.drawLine (juce::Line<float> (centre + dir * (radius - 4.0f), centre + dir * radius), 1.0f);
    }

    // Cap
    const float capRadius = radius - 6.0f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6b6a66), centre.x - capRadius * 0.6f, centre.y - capRadius * 0.8f,
                                             juce::Colour (0xff1c1c1b), centre.x + capRadius * 0.6f, centre.y + capRadius, false));
    g.fillEllipse (centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.drawEllipse (centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f, 1.0f);

    // Pointer
    juce::Path pointer;
    pointer.addRoundedRectangle (-2.0f, -capRadius + 3.0f, 4.0f, capRadius * 0.5f, 1.5f);
    g.setColour (retro::cream);
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre));
}

void RetroLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                         bool highlighted, bool down)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (2.0f);
    const bool on = button.getToggleState();

    // Shadow and housing
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (bounds.translated (0.0f, 2.0f), 5.0f);

    g.setGradientFill (juce::ColourGradient (down ? juce::Colour (0xff2a2826) : juce::Colour (0xff4b4743),
                                             bounds.getX(), bounds.getY(),
                                             juce::Colour (0xff1f1d1b), bounds.getX(), bounds.getBottom(), false));
    g.fillRoundedRectangle (bounds, 5.0f);

    g.setColour (highlighted ? retro::cream.withAlpha (0.6f) : juce::Colours::black.withAlpha (0.6f));
    g.drawRoundedRectangle (bounds, 5.0f, 1.0f);

    // Lamp
    constexpr float lampRadius = 6.0f;
    const auto lampCentre = juce::Point<float> (bounds.getX() + 18.0f, bounds.getCentreY());

    if (on)
    {
        g.setColour (retro::ledRed.withAlpha (0.28f));
        g.fillEllipse (lampCentre.x - 11.0f, lampCentre.y - 11.0f, 22.0f, 22.0f);
    }

    g.setColour (on ? retro::ledRed : retro::ledRedDim);
    g.fillEllipse (lampCentre.x - lampRadius, lampCentre.y - lampRadius, lampRadius * 2.0f, lampRadius * 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawEllipse (lampCentre.x - lampRadius, lampCentre.y - lampRadius, lampRadius * 2.0f, lampRadius * 2.0f, 1.0f);

    // Label
    g.setColour (retro::cream);
    g.setFont (retro::boldFont (12.0f));
    g.drawText (button.getButtonText(), bounds.withTrimmedLeft (34.0f), juce::Justification::centredLeft);
}

// The value box under each knob: dark glass with seven-segment digits and a printed unit.
void RetroLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    const auto bounds = label.getLocalBounds().toFloat().reduced (1.0f);

    g.setColour (retro::ledGlass);
    g.fillRoundedRectangle (bounds, 3.0f);

    // A faint reflection across the top half of the glass
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.07f), bounds.getX(), bounds.getY(),
                                             juce::Colours::transparentWhite, bounds.getX(), bounds.getCentreY(), false));
    g.fillRoundedRectangle (bounds, 3.0f);

    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawRoundedRectangle (bounds, 3.0f, 1.0f);

    if (label.isBeingEdited())
        return;                       // the text editor draws itself while typing a value

    // The unit and the number of digit cells come from the slider that owns this box.
    juce::String unit;
    int cells = 5;

    if (auto* slider = dynamic_cast<juce::Slider*> (label.getParentComponent()))
    {
        unit = slider->getProperties().getWithDefault ("unit", juce::String()).toString().trim();
        cells = static_cast<int> (slider->getProperties().getWithDefault ("cells", 5));
    }

    auto content = bounds.reduced (7.0f, 4.0f);

    if (unit.isNotEmpty())
    {
        g.setColour (retro::creamDim);
        g.setFont (retro::boldFont (10.0f));
        g.drawText (unit, content.removeFromRight (22.0f), juce::Justification::centredRight);
    }

    retro::drawSevenSegment (g, content.withTrimmedRight (4.0f), label.getText(), cells);
}