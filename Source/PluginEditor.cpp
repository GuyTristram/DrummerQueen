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

void DrummerQueenAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
	int width = data().total_divisions() * m_note_width;
	int height = data().lane_count() * m_note_height;
    m_grid.setBounds(m_grid_left, m_grid_top, width, height);
	int grid_bottom = m_grid_top + height;
	int grid_right = m_grid_left + width;

    int x = m_grid_left;
    for (auto& vb : m_velocity_buttons)
    {
		vb->setBounds(x, 32, 24, 24);
		x += 26;
    }
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
