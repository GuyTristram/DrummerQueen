/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <format>


//==============================================================================
DrummerQueenAudioProcessorEditor::DrummerQueenAudioProcessorEditor (DrummerQueenAudioProcessor& p)
	: AudioProcessorEditor(&p), audioProcessor(p), m_grid(p.m_data),
    m_keyboard(m_keyboard_state, juce::MidiKeyboardComponent::horizontalKeyboard)
{



    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    addAndMakeVisible(m_grid);
    int vel_button_count = 8;

    m_velocity_buttons.resize(vel_button_count);

	int velocities[] = { 127, 112, 96, 80, 64, 48, 32, 16 };
    int i = 0;
    for (auto& vb : m_velocity_buttons)
    {
        vb = std::make_unique<VelocityButton>(velocities[i]);
        vb->setRadioGroupId(1);
        vb->onClick = [this, v = velocities[i]] {m_grid.set_new_velocity(v); };
        addAndMakeVisible(vb.get());
        ++i;
    }
	m_velocity_buttons.front()->setToggleState(true, juce::dontSendNotification);

    update_pattern_buttons();
	m_add_pattern_button.setButtonText("+");
	m_add_pattern_button.onClick = [this] {data().set_current_pattern(data().add_pattern()); update_pattern_buttons(); };
    addAndMakeVisible(m_add_pattern_button);

    addAndMakeVisible(m_sequence_editor);
	m_sequence_editor.onTextChange = [this] {data().set_sequence_str(m_sequence_editor.getText().toStdString()); };
	addAndMakeVisible(m_play_sequence_button);
	m_play_sequence_button.onClick = [this] {data().play_sequence(m_play_sequence_button.getToggleState()); };

	m_undo_button.setButtonText("Undo");
	addAndMakeVisible(m_undo_button);
    m_undo_button.onClick = [this] {data().undo(); m_grid.repaint(); resize_grid(); };
	m_redo_button.setButtonText("Redo");
	addAndMakeVisible(m_redo_button);
    m_redo_button.onClick = [this] {data().redo(); m_grid.repaint(); resize_grid(); };

    m_swing_slider.setSliderStyle(juce::Slider::LinearHorizontal);
    m_swing_slider.setRange(0.0, 1.0, 0.05);
    m_swing_slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 90, 0);
    m_swing_slider.setPopupDisplayEnabled(true, false, this);
    m_swing_slider.setTextValueSuffix(" Swing");
    m_swing_slider.setValue(0.5);
	m_swing_slider.addListener(this);

	addAndMakeVisible(m_swing_slider);

    //g.drawFittedText(std::format("Beat {}", audioProcessor.barPos()), getLocalBounds(), juce::Justification::topLeft, 1);
    for (int i = 0; i < data().lane_count(); ++i)
    {
		m_lane_name_buttons.push_back(std::make_unique<juce::TextButton>(data().get_lane_name(i)));
		m_lane_name_buttons.back()->onClick = [this, i] {edit_lane(i); };
        addAndMakeVisible(*m_lane_name_buttons.back());
    }

    addAndMakeVisible(m_note_editor);
    addAndMakeVisible(m_name_editor);

    m_set_button.setButtonText("Set");
	m_set_button.onClick = [this] {set_lane_data(); };
    addAndMakeVisible(m_set_button);

    m_delete_button.setButtonText("Delete");
    m_delete_button.onClick = [this] {delete_lane(); };
    addAndMakeVisible(m_delete_button);

    m_cancel_button.setButtonText("Cancel");
    m_cancel_button.onClick = [this] {set_editor_visible(false); };
    addAndMakeVisible(m_cancel_button);

    m_keyboard.setLowestVisibleKey(35);
	m_keyboard_state.addListener(this);
	addAndMakeVisible(m_keyboard);

    add_time_signature("4/4", 4, 4);
    add_time_signature("3/4", 3, 4);
    add_time_signature("4/3", 4, 3);
	select_time_signature();
    m_time_signature_box.onChange = [this]
    {
        int i = m_time_signature_box.getSelectedId() - 1;
        data().set_time_signature(m_time_signatures[i].beats, m_time_signatures[i].beat_divisions);
        resize_grid();
        m_grid.repaint();
    };
    addAndMakeVisible(m_time_signature_box);
 
    m_drag_button.setButtonText("Drag");
	m_drag_button.onStartDrag = [this]
		{
            juce::File tempDirectory = juce::File::getSpecialLocation(juce::File::tempDirectory);
			juce::File tempFile = tempDirectory.getChildFile("pattern.midi");
			juce::MidiMessageSequence sequence;
            int tpq = 960;
			data().get_events(0., data().beats(), data().beats() * tpq, sequence);
            juce::MidiFile midi_file;
            midi_file.setTicksPerQuarterNote(tpq);
            midi_file.addTrack(sequence);
            auto stream = tempFile.createOutputStream();
            if (stream)
            {
                stream->setPosition(0);
                stream->truncate();
                midi_file.writeTo(*stream);
				stream->flush();
				stream = nullptr;
                juce::DragAndDropContainer::performExternalDragDropOfFiles({ tempFile.getFullPathName()}, true);
            }
		};
    addAndMakeVisible(m_drag_button);
    set_editor_visible(false);


    setSize(500, 424);
    audioProcessor.addChangeListener(this);

}

DrummerQueenAudioProcessorEditor::~DrummerQueenAudioProcessorEditor()
{
    audioProcessor.removeChangeListener(this);
}

//==============================================================================
void DrummerQueenAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(15.0f));

    //g.drawFittedText(std::format("Beat {}", audioProcessor.barPos()), getLocalBounds(), juce::Justification::topLeft, 1);
	for (int i = 0; i < data().lane_count(); ++i)
	{
        //g.drawFittedText(data().get_lane_name(i), {8, 64 + i * 24, 80, 24}, juce::Justification::centredLeft, 1);
	}
}

void DrummerQueenAudioProcessorEditor::resize_grid()
{
    int width = data().total_divisions() * m_note_width + 1;
    int height = data().lane_count() * m_note_height + 1;
    m_grid.setBounds(m_grid_left, m_grid_top, width, height);
}


void DrummerQueenAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
	int width = data().total_divisions() * m_note_width;
	int height = data().lane_count() * m_note_height;
    resize_grid();
	int grid_bottom = m_grid_top + height;

	m_undo_button.setBounds(8, 8, 40, 24);
	m_redo_button.setBounds(8, 32, 40, 24);

    int x = m_grid_left;
    for (auto& vb : m_velocity_buttons)
    {
		vb->setBounds(x, 32, 24, 24);
		x += 26;
    }
    x += 80;
    m_time_signature_box.setBounds(x, 32, 64, 24);
    int y = 64;
    for (auto& name : m_lane_name_buttons)
    {
		name->setBounds(8, y, 72, 24);
        y += 24;
    }
    m_swing_slider.setBounds(m_grid_left, 8, 200, 24);

	int editor_left = m_grid_left + 6;
    m_name_editor.setBounds(editor_left, m_grid_top + 8, 72, 24);
    m_note_editor.setBounds(editor_left + 80, m_grid_top + 8, 40, 24);
	m_set_button.setBounds(editor_left + 128, m_grid_top + 8, 40, 24);
    m_delete_button.setBounds(editor_left + 176, m_grid_top + 8, 72, 24);
    m_cancel_button.setBounds(editor_left + 254, m_grid_top + 8, 72, 24);
    m_keyboard.setBounds(editor_left, m_grid_top + 40, width, 52);

    x = m_grid_left;
	y = grid_bottom + 8;
	for (auto& pb : m_pattern_buttons)
	{
		pb->setBounds(x, y, 24, 24);
		x += 26;
	}
	m_add_pattern_button.setBounds(x, y, 24, 24);

	m_play_sequence_button.setBounds(8, grid_bottom + 36, 24, 24);
	m_sequence_editor.setBounds(m_grid_left, grid_bottom + 36, width, 24);

    m_drag_button.setBounds(8, grid_bottom + 64, 64, 24);
}

void DrummerQueenAudioProcessorEditor::handleNoteOn(juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity)
{
    //auto note_name = juce::MidiMessage::getMidiNoteName(midiNoteNumber, true, true, 3);
    auto note_name = std::format("{}", midiNoteNumber);
    m_note_editor.setText(note_name, juce::dontSendNotification);
	audioProcessor.play_note(midiNoteNumber);
}

void DrummerQueenAudioProcessorEditor::set_lane_data()
{
	if (m_lane_editting != -1)
	{
		data().set_lane_name(m_lane_editting, m_name_editor.getText().toStdString());
		auto note = m_note_editor.getText().getIntValue();
		data().set_lane_note(m_lane_editting, note);
		m_lane_name_buttons[m_lane_editting]->setButtonText(m_name_editor.getText());
	}
	set_editor_visible(false);
	m_lane_editting = -1;
}

void DrummerQueenAudioProcessorEditor::delete_lane()
{
}

void DrummerQueenAudioProcessorEditor::set_editor_visible(bool visible)
{
    m_name_editor.setVisible(visible);
    m_note_editor.setVisible(visible);
    m_set_button.setVisible(visible);
    m_delete_button.setVisible(visible);
    m_cancel_button.setVisible(visible);
    m_keyboard.setVisible(visible);
    m_grid.setVisible(!visible);
}

void DrummerQueenAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
	if (slider == &m_swing_slider)
	{
		data().set_swing(m_swing_slider.getValue());
	}
}

void DrummerQueenAudioProcessorEditor::update_pattern_buttons()
{
	m_pattern_buttons.clear();

	int n_patterns = data().pattern_count();
    for (int i = 0; i < n_patterns; ++i)
    {
        m_pattern_buttons.emplace_back(std::make_unique<PatternButton>(i + 1));
        m_pattern_buttons.back()->onClick = [this, i] {data().set_current_pattern(i); m_grid.repaint(); };
		m_pattern_buttons.back()->setRadioGroupId(2);
		addAndMakeVisible(m_pattern_buttons.back().get());
    }
	m_pattern_buttons[data().get_current_pattern()]->setToggleState(true, juce::dontSendNotification);
    int height = data().lane_count() * m_note_height;
    int grid_bottom = m_grid_top + height;
    int x = m_grid_left;
    int y = grid_bottom + 8;
    for (auto& pb : m_pattern_buttons)
    {
        pb->setBounds(x, y, 24, 24);
        x += 26;
    }
    m_add_pattern_button.setBounds(x, y, 24, 24);

}

void DrummerQueenAudioProcessorEditor::edit_lane(int lane)
{
	m_lane_editting = lane;
	m_name_editor.setText(data().get_lane_name(lane), juce::dontSendNotification);
	auto note_name = std::format("{}", data().get_lane_note(lane));
	m_note_editor.setText(note_name, juce::dontSendNotification);
    set_editor_visible(true);
}

void DrummerQueenAudioProcessorEditor::add_time_signature(char const* name, int beats, int beat_divisions)
{
	m_time_signature_box.addItem(name, m_time_signatures.size() + 1);
	m_time_signatures.push_back({ beats, beat_divisions });
}

void DrummerQueenAudioProcessorEditor::select_time_signature()
{
	for (int i = 0; i < m_time_signatures.size(); ++i)
	{
		if (m_time_signatures[i].beats == data().beats() && m_time_signatures[i].beat_divisions == data().beat_divisions())
		{
			m_time_signature_box.setSelectedId(i + 1, juce::dontSendNotification);
			return;
		}
	}
}
