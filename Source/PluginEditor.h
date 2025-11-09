#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "CustomButtons.h"
#include "DrumGrid.h"

#include <vector>
#include <memory>

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
    void mouseWheelMove(const juce::MouseEvent& event,
        const juce::MouseWheelDetails& wheel) override;

    void changeListenerCallback(juce::ChangeBroadcaster*) override;

    void resize_grid();
    void layout_components();


    void delete_lane();

	void drag_onto_pattern(int pattern, const juce::String& files);
    DrumData& data() { return audioProcessor.m_data; }
private:
    void drag_midi();
    void sliderValueChanged(juce::Slider* slider) override;
    void set_pattern(int i, bool update_button = true);
    void update_pattern_buttons();
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    DrummerQueenAudioProcessor& audioProcessor;

    DrumGrid m_grid;
    std::vector<std::unique_ptr<juce::ToggleButton>> m_velocity_buttons;
	int m_velocity_button_selected = 0;
    juce::Component m_pattern_button_parent;
    std::vector<std::unique_ptr<PatternButton>> m_pattern_buttons;

    std::vector<std::unique_ptr<LaneButton>> m_lane_name_buttons;
    std::vector<std::unique_ptr<juce::ComboBox>> m_lane_combo_boxes;
    juce::TextButton m_add_lane_button;

    juce::Slider m_swing_slider;

    juce::TextButton m_undo_button;
    juce::TextButton m_redo_button;

    juce::TextButton m_clear_notes_button;
    juce::TextButton m_clear_all_button;

    juce::TextEditor m_sequence_editor;
	juce::Label m_sequence_length_label;
	juce::ToggleButton m_play_sequence_button;
    juce::TextEditor m_bpm_editor;

	RecordButton m_record_button;

    int m_velocity = 127;

	juce::ComboBox m_time_signature_box;
    std::vector<TimeSignature> m_time_signatures;
	void add_time_signature(char const *name, int beats, int beat_divisions);
	void select_time_signature();

    juce::ComboBox m_drum_kit_box;


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
    void fileClicked(const juce::File&, const juce::MouseEvent&) override {};
    void fileDoubleClicked(const juce::File &) override {}
    void browserRootChanged(const juce::File & newRoot) override;

    int m_note_width = 24;
    int m_note_height = 24;

    int m_grid_top = 121;
    int m_grid_left = 340;
    int m_lane_button_width = 108;
	int m_lane_button_left = m_grid_left - m_lane_button_width - m_note_width;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrummerQueenAudioProcessorEditor)
};

void drag_midi_sequence(std::vector<SequenceItem> const& sequence, DrumData& data, const char* suffix = "");