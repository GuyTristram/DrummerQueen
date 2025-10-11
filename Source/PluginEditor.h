/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

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

private:
	DrummerQueenAudioProcessorEditor* m_editor = nullptr;
    int m_pattern;
};


class DragButton : public juce::TextButton
{
public:
    std::function< void()> 	onStartDrag;

protected:
    void mouseDown(const juce::MouseEvent& event) override { TextButton::mouseDown(event);  onStartDrag(); }
};

class DrumNoteComboBox : public juce::ComboBox
{
    void paint(juce::Graphics&) override;
};

//==============================================================================
/**
*/
class DrummerQueenAudioProcessorEditor
  : public juce::AudioProcessorEditor,
    public juce::ChangeListener,
    public juce::Slider::Listener,
    public juce::FileBrowserListener
{
public:
    DrummerQueenAudioProcessorEditor (DrummerQueenAudioProcessor&);
    ~DrummerQueenAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resize_grid();
    void resized() override;

    void changeListenerCallback(juce::ChangeBroadcaster* source) override
    {
        m_grid.setPosition(audioProcessor.barPos());
        repaint();
    }

    void delete_lane();

	void drag_onto_pattern(int pattern, const juce::StringArray& files);
private:
    void drag_midi();
    void sliderValueChanged(juce::Slider* slider) override;
    void set_pattern(int i);
    void update_pattern_buttons();
	DrumData& data() { return audioProcessor.m_data; }
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    DrummerQueenAudioProcessor& audioProcessor;

    DrumGrid m_grid;
    std::vector<std::unique_ptr<juce::ToggleButton>> m_velocity_buttons;
    juce::Component m_pattern_button_parent;
    std::vector<std::unique_ptr<PatternButton>> m_pattern_buttons;
    juce::TextButton m_add_pattern_button;

    std::vector<std::unique_ptr<juce::TextButton>> m_lane_name_buttons;
    std::vector<std::unique_ptr<juce::ComboBox>> m_lane_combo_boxes;
    juce::TextButton m_add_lane_button;

    juce::Slider m_swing_slider;

    juce::TextButton m_undo_button;
    juce::TextButton m_redo_button;

    juce::TextEditor m_sequence_editor;
	juce::Label m_sequence_length_label;
	juce::ToggleButton m_play_sequence_button;

    int m_velocity = 127;

	juce::ComboBox m_time_signature_box;
    std::vector<TimeSignature> m_time_signatures;
	void add_time_signature(char const *name, int beats, int beat_divisions);
	void select_time_signature();

    DragButton m_drag_button;

    juce::WildcardFileFilter m_midi_file_filter;
    /*
	juce::TimeSliceThread m_time_slice_thread;
	juce::WildcardFileFilter m_midi_file_filter;
    juce::DirectoryContentsList m_directory_contents;
    juce::FileListComponent m_file_list;
*/
	juce::FileBrowserComponent m_file_list;
    void selectionChanged() override;
    void fileClicked(const juce::File & file, const juce::MouseEvent & e) override;
    void fileDoubleClicked(const juce::File & file) override {}
    void browserRootChanged(const juce::File & newRoot) override;


    int m_grid_top = 64;
    int m_grid_left = 140;

    int m_note_width = 24;
    int m_note_height = 24;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrummerQueenAudioProcessorEditor)
};
