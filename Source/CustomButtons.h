#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "DrumGrid.h"

#include <vector>
#include <memory>

class DrummerQueenAudioProcessorEditor;

class PatternButton : public juce::ToggleButton, public juce::FileDragAndDropTarget
{
public:
    PatternButton(DrummerQueenAudioProcessorEditor *editor, int pattern) : m_editor(editor), m_pattern(pattern) {}

    void paintButton(juce::Graphics& g,
        bool	shouldDrawButtonAsHighlighted,
        bool	shouldDrawButtonAsDown) override;

	bool isInterestedInFileDrag(const juce::StringArray& files) override;
	void filesDropped(const juce::StringArray& files, int x, int y) override;
    void mouseDrag(const juce::MouseEvent& event) override;

private:
	std::string get_suffix() const;
	DrummerQueenAudioProcessorEditor* m_editor = nullptr;
    int m_pattern;
};


class DragButton : public juce::ImageButton
{
public:
    DragButton();
    std::function< void()> 	onStartDrag;

protected:
    void mouseDown(const juce::MouseEvent& event) override { ImageButton::mouseDown(event);  onStartDrag(); }
};

class DrumNoteComboBox : public juce::ComboBox
{
    void paint(juce::Graphics&) override;
};


class LaneButton : public juce::TextButton
{
public:
	LaneButton(const juce::String& name) : TextButton(name) {}
    void paintButton(juce::Graphics& g,
        bool	shouldDrawButtonAsHighlighted,
        bool	shouldDrawButtonAsDown) override;
};

void draw_note_in_style(juce::Graphics& g, int style, int velocity, float x, float y, float size);

class VelocityButton : public juce::ToggleButton
{
public:
    VelocityButton(int velocity) : m_velocity(velocity) {}
    void paintButton(juce::Graphics& g,
        bool	shouldDrawButtonAsHighlighted,
        bool	shouldDrawButtonAsDown) override;
    int m_velocity;
};

class RecordButton : public juce::ToggleButton
{
public:
    void paintButton(juce::Graphics& g,
        bool	shouldDrawButtonAsHighlighted,
        bool	shouldDrawButtonAsDown) override;
};
