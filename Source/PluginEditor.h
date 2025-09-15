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


class DragButton : public juce::TextButton
{
public:
    std::function< void()> 	onStartDrag;

protected:
    void mouseDown(const juce::MouseEvent& event) override { TextButton::mouseDown(event);  onStartDrag(); }
};

//==============================================================================
/**
*/
class DrummerQueenAudioProcessorEditor
  : public juce::AudioProcessorEditor,
    public juce::ChangeListener,
    public juce::Slider::Listener,
	public juce::MidiKeyboardStateListener
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
    void handleNoteOn(juce::MidiKeyboardState* source,
        int	midiChannel,
        int	midiNoteNumber,
        float	velocity) override;

	void handleNoteOff(juce::MidiKeyboardState* source,
		int	midiChannel,
		int	midiNoteNumber,
		float	velocity) override
	{
	}

    void set_lane_data();
    void delete_lane();

	void set_editor_visible(bool visible);

private:
    void drag_midi();
    void sliderValueChanged(juce::Slider* slider) override;
    void update_pattern_buttons();
	DrumData& data() { return audioProcessor.m_data; }
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    DrummerQueenAudioProcessor& audioProcessor;

    DrumGrid m_grid;
    std::vector<std::unique_ptr<juce::ToggleButton>> m_velocity_buttons;
    std::vector<std::unique_ptr<juce::ToggleButton>> m_pattern_buttons;
    juce::TextButton m_add_pattern_button;
    std::vector<std::unique_ptr<juce::TextButton>> m_lane_name_buttons;
    juce::Slider m_swing_slider;

    juce::TextButton m_undo_button;
    juce::TextButton m_redo_button;

    juce::TextEditor m_sequence_editor;
	juce::ToggleButton m_play_sequence_button;

    int m_lane_editting = -1;
    juce::MidiKeyboardState m_keyboard_state;
	juce::MidiKeyboardComponent m_keyboard;
	juce::TextEditor m_name_editor;
	juce::TextEditor m_note_editor;
    juce::TextButton m_set_button;
    juce::TextButton m_delete_button;
    juce::TextButton m_cancel_button;
    void edit_lane(int lane);
 
	juce::TextButton m_add_lane_button;
    int m_velocity = 127;

	juce::ComboBox m_time_signature_box;
    std::vector<TimeSignature> m_time_signatures;
	void add_time_signature(char const *name, int beats, int beat_divisions);
	void select_time_signature();

    DragButton m_drag_button;


    int m_grid_top = 64;
    int m_grid_left = 80;

    int m_note_width = 24;
    int m_note_height = 24;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrummerQueenAudioProcessorEditor)
};
