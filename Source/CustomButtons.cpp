/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "CustomButtons.h"
#include <format>
#include "json.hpp"
#include "share.c"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


void PatternButton::paintButton(juce::Graphics& g, bool, bool)
{
    g.fillAll(getToggleState() ? juce::Colours::white : juce::Colours::black);
    g.setColour(getToggleState() ? juce::Colours::black : juce::Colours::white);

    char label[2] = { char(m_pattern) + 'A', 0 };
    g.drawText(label, getLocalBounds(), juce::Justification::centred);
}

bool PatternButton::isInterestedInFileDrag(const juce::StringArray& files)
{
    if (files.size() > 0) {
        // Avoid dragging onto itself
        juce::File f(files[0]);
        if (!f.getFileNameWithoutExtension().endsWith(get_suffix())) {
            return true;
        }
    }
    return false;
}

void PatternButton::filesDropped(const juce::StringArray& files, int, int)
{
    if (files.size() > 0) {
		// Avoid dragging onto itself
		juce::File f(files[0]);
        if (f.getFileNameWithoutExtension().endsWith(get_suffix())) {
            return;
        }
        m_editor->drag_onto_pattern(m_pattern, files[0]);
    }
}

void PatternButton::mouseDrag(const juce::MouseEvent& event)
{
    ToggleButton::mouseDrag(event);
	auto& pattern = m_editor->data().get_pattern(m_pattern);
	std::vector<SequenceItem> sequence(1);
    sequence[0].pattern = m_pattern;
    sequence[0].end_beat = pattern.time_signature.beats;
	drag_midi_sequence(sequence, m_editor->data(), get_suffix().c_str());
}

std::string PatternButton::get_suffix() const
{
    return std::format("_{}", (char)('A' + m_pattern));
}

void LaneButton::paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto& lf = getLookAndFeel();

    lf.drawButtonBackground(g, *this,
        findColour(getToggleState() ? buttonOnColourId : buttonColourId),
        shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
    juce::Font font(lf.getTextButtonFont(*this, getHeight()));
    g.setFont(font);
    g.setColour(findColour(getToggleState() ? TextButton::textColourOnId
        : TextButton::textColourOffId)
        .withMultipliedAlpha(isEnabled() ? 1.0f : 0.5f));

    const int yIndent = std::min(4, proportionOfHeight(0.3f));
    const int cornerSize = std::min(getHeight(), getWidth()) / 2;

    const int fontHeight = juce::roundToInt(font.getHeight() * 0.6f);
    const int leftIndent = std::min(fontHeight, 2 + cornerSize / (isConnectedOnLeft() ? 4 : 2));
    const int rightIndent = std::min(fontHeight, 2 + cornerSize / (isConnectedOnRight() ? 4 : 2));
    const int textWidth = getWidth() - leftIndent - rightIndent;

    if (textWidth > 0)
        g.drawFittedText(getButtonText(),
            leftIndent, yIndent, textWidth, getHeight() - yIndent * 2,
            juce::Justification::centredLeft, 2);
}

DragButton::DragButton()
{
    int x, y, comp;
    auto share_img = stbi_load_from_memory(share_png, sizeof(share_png), &x, &y, &comp, 0);
	juce::Image img(juce::Image::ARGB, x, y, false);
    for (int ix = 0; ix < x; ++ix) {
        for (int iy = 0; iy < y; ++iy) {
            int index = (iy * x + ix) * comp;
            juce::Colour c;
            c = juce::Colour::fromRGBA(share_img[index], share_img[index + 1], share_img[index + 2], share_img[index + 3]);
            img.setPixelAt(ix, iy, c);
        }
    }
	setImages(false, true, true,
		img, 1.f, juce::Colours::transparentBlack,
		img, 1.f, juce::Colours::transparentBlack,
		img, 1.f, juce::Colours::transparentBlack);
}

void DrumNoteComboBox::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::white);
    g.drawRect(getLocalBounds());
    char text[2] = { 'v', 0 };
    g.drawText(text, getLocalBounds(), juce::Justification::centred);
}

void draw_note_in_style(juce::Graphics& g, int style, int velocity, float x, float y, float size)
{
    float max_size = size - 8.f;
    float min_size = max_size / 7.f;
    float size_inside = float(velocity) / 127.f * (max_size - min_size) + min_size;
    float border = (size - size_inside) / 2.f;
    if (style == 0) {
        g.fillEllipse(x + border, y + border, size - border * 2, size - border * 2);
    }
    else {
        border -= 2;
        juce::Path p;
        p.startNewSubPath(x + 2, y + border);
        p.lineTo(x + size - border * 2, y + size / 2);
        p.lineTo(x + 2, y + size - border);
        g.fillPath(p);
    }
}

void VelocityButton::paintButton(juce::Graphics& g, bool, bool)
{
    g.fillAll(juce::Colours::black);
    if (getToggleState())
    {
        g.setColour(juce::Colours::white);
    }
    else
    {
        g.setColour(juce::Colours::grey);
    }

    int max_size = std::min(getWidth(), getHeight());

    draw_note_in_style(g, 1, m_velocity, 0.f, 0.f, float(max_size));
}

