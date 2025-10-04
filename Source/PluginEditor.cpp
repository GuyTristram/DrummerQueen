/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <format>

namespace
{
std::vector<DrumInfo> general_midi = {
    {35,        "Acoustic Bass Drum"},
    {36,		"Bass Drum"},
    {37,		"Side Stick"},
    {38,		"Acoustic Snare"},
    {39,		"Hand Clap"},
    {40,		"Electric Snare"},
    {41,		"Low Floor Tom"},
    {42,		"Closed Hi Hat"},
    {43,		"High Floor Tom"},
    {44,		"Pedal Hi - Hat"},
    {45,		"Low Tom"},
    {46,		"Open Hi - Hat"},
    {47,		"Low - Mid Tom"},
    {48,		"Hi - Mid Tom"},
    {49,		"Crash Cymbal 1"},
    {50,		"High Tom"},
    {51,		"Ride Cymbal 1"},
    {52,		"Chinese Cymbal"},
    {53,		"Ride Bell"},
    {54,		"Tambourine"},
    {55,		"Splash Cymbal"},
    {56,		"Cowbell"},
    {57,		"Crash Cymbal 2"},
    {58,		"Vibraslap"},
    {59,		"Ride Cymbal 2"},
    {60,		"Hi Bongo"},
    {61,		"Low Bongo"},
    {62,		"Mute Hi Conga"},
    {63,		"Open Hi Conga"},
    {64,        "Low Conga"},
    {65,		"Hi Timbale"},
    {66,		"Low Timbale"},
    {67,		"Hi Agogo"},
    {68,		"Low Agogo"},
    {69,		"Cabasa"},
    {70,		"Maracas"},
    {71,		"Short Whistle"},
    {72,		"Long Whistle"},
    {73,		"Short Guiro"},
    {74,		"Long Guiro"},
    {75,		"Claves"},
    {76,		"Hi Wood Block"},
    {77,		"Low Wood Block"},
    {78,		"Mute Cuica"},
    {79,		"Open Cuica"},
    {80,		"Mute Triangle"},
    {81,		"Open Triangle"}
};
}
//==============================================================================
DrummerQueenAudioProcessorEditor::DrummerQueenAudioProcessorEditor (DrummerQueenAudioProcessor& p)
	: AudioProcessorEditor(&p), audioProcessor(p), m_grid(p.m_data)
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

	m_sequence_editor.setText(data().get_sequence_str());
    addAndMakeVisible(m_sequence_editor);
	m_sequence_editor.onTextChange = [this]
        {
            data().set_sequence_str(m_sequence_editor.getText().toStdString());
			m_sequence_editor.setText(data().get_sequence_str(), juce::dontSendNotification);
			m_sequence_length_label.setText(std::format("Len: {}", data().sequence_length()), juce::dontSendNotification);
        };
	addAndMakeVisible(m_play_sequence_button);
	m_play_sequence_button.onClick = [this] {data().play_sequence(m_play_sequence_button.getToggleState()); };
    addAndMakeVisible(m_sequence_length_label);

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
	m_drag_button.onStartDrag = [this] { drag_midi(); };
    addAndMakeVisible(m_drag_button);

	set_pattern(0);


    setSize(540, 424);
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
		name->setBounds(8, y, 132, 24);
        y += 24;
    }
    m_swing_slider.setBounds(m_grid_left, 8, 200, 24);

    x = m_grid_left;
	y = grid_bottom + 8;
	for (auto& pb : m_pattern_buttons)
	{
		pb->setBounds(x, y, 24, 24);
		x += 26;
	}
	m_add_pattern_button.setBounds(x, y, 24, 24);

	m_play_sequence_button.setBounds(8, grid_bottom + 36, 24, 24);
    m_sequence_editor.setBounds(m_grid_left, grid_bottom + 36, width-80, 24);
    m_sequence_length_label.setBounds(m_grid_left + width - 80, grid_bottom + 36, 80, 24);

    m_drag_button.setBounds(8, grid_bottom + 64, 64, 24);
}

void DrummerQueenAudioProcessorEditor::handleNoteOn(juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity)
{
    //auto note_name = juce::MidiMessage::getMidiNoteName(midiNoteNumber, true, true, 3);
    auto note_name = std::format("{}", midiNoteNumber);
	audioProcessor.play_note(midiNoteNumber);
}

void DrummerQueenAudioProcessorEditor::delete_lane()
{
}

void DrummerQueenAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
	if (slider == &m_swing_slider)
	{
		data().set_swing(m_swing_slider.getValue());
	}
}

void DrummerQueenAudioProcessorEditor::set_pattern(int i)
{
    data().set_current_pattern(i);
	auto const &pattern = data().get_current_pattern();
    m_lane_name_buttons.clear();
    int y = 64;
    for (int i = 0; i < data().lane_count(); ++i)
    {
        m_lane_name_buttons.push_back(std::make_unique<juce::ComboBox>());
        for (auto const& drum : general_midi)
        {
            m_lane_name_buttons.back()->addItem(drum.name, drum.note);
        }
        m_lane_name_buttons.back()->onChange = [this, i]
            {
                data().get_current_pattern().lanes[i].note = m_lane_name_buttons[i]->getSelectedId();
            };
        m_lane_name_buttons.back()->setSelectedId(pattern.lanes[i].note, juce::dontSendNotification);
        addAndMakeVisible(*m_lane_name_buttons.back());
        m_lane_name_buttons.back()->setBounds(8, y, 132, 24);
        y += 24;
    }
    m_grid.repaint();
}

void DrummerQueenAudioProcessorEditor::update_pattern_buttons()
{
	m_pattern_buttons.clear();

	int n_patterns = data().pattern_count();
    for (int i = 0; i < n_patterns; ++i)
    {
        m_pattern_buttons.emplace_back(std::make_unique<PatternButton>(i));
        m_pattern_buttons.back()->onClick = [this, i] {set_pattern(i); };
		m_pattern_buttons.back()->setRadioGroupId(2);
		addAndMakeVisible(m_pattern_buttons.back().get());
    }
	m_pattern_buttons[data().get_current_pattern_id()]->setToggleState(true, juce::dontSendNotification);
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

void DrummerQueenAudioProcessorEditor::drag_midi()
{
    juce::File tempDirectory = juce::File::getSpecialLocation(juce::File::tempDirectory);
    juce::File tempFile = tempDirectory.getChildFile("pattern.midi");
    juce::MidiMessageSequence midi_sequence;
	auto sequence = data().is_playing_sequence() ? data().get_sequence() : std::vector<int>{ data().get_current_pattern_id() };
    int tpq = 960;
	double offset = 0.;
	for (auto p : sequence)
	{
        data().get_events(p, 0., data().beats(), offset, data().beats() * tpq, midi_sequence);
		offset += data().beats();
	}
    juce::MidiFile midi_file;
    midi_file.setTicksPerQuarterNote(tpq);
    midi_file.addTrack(midi_sequence);
    auto stream = tempFile.createOutputStream();
    if (stream)
    {
        stream->setPosition(0);
        stream->truncate();
        midi_file.writeTo(*stream);
        stream->flush();
        stream = nullptr;
        juce::DragAndDropContainer::performExternalDragDropOfFiles({ tempFile.getFullPathName() }, true);
    }
}
