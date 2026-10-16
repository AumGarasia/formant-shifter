#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace retro
{

inline const juce::Colour faceplateTop    { 0xff3b3631 };
inline const juce::Colour faceplateBottom { 0xff26231f };
inline const juce::Colour cream           { 0xffeadfc8 };
inline const juce::Colour creamDim        { 0xff9c9382 };
inline const juce::Colour ledRed          { 0xffff3326 };
inline const juce::Colour ledRedDim       { 0xff3a0b08 };
inline const juce::Colour ledGlass        { 0xff140605 };

inline juce::Font boldFont (float height)
{
    return juce::Font (juce::FontOptions (height, juce::Font::bold));
}

/** Draws `text` as old-calculator seven-segment digits, right-aligned in `numCells` cells
    that are scaled to fit `area`. Understands the digits, '-' and '.', where a '.' lights
    the decimal point of the digit before it. Unlit segments stay faintly visible. */
void drawSevenSegment (juce::Graphics&, juce::Rectangle<float> area,
                       const juce::String& text, int numCells);

} // namespace retro

/** Retro hardware styling: knurled black knobs with a scale, red LED readouts, and a
    pushbutton with a lamp. */
class RetroLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    RetroLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawLabel (juce::Graphics&, juce::Label&) override;
};