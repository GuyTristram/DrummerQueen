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
const int MAX_LANES = 12;
const int MAX_DIVISIONS = 16;
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
std::vector<DrumInfo> mt_power = {
    {36, "Kick"},
    {38, "Snare"},
    {37, "Side Stick"},
    {42, "HH Closed"},
    {44, "HH Half Open"},
    {46, "HH Open"},
    {65, "HH Pedal"},
    {48, "Tom High"},
    {45, "Tom Mid"},
    {41, "Tom Low"},
    {51, "Ride"},
    {53, "Bell"},
    {49, "Crash L"},
    {57, "Crash R"},
    {58, "Crash Choked"},
    {52, "China"},
    {55, "Splash"}
};

}
//==============================================================================
DrummerQueenAudioProcessorEditor::DrummerQueenAudioProcessorEditor (DrummerQueenAudioProcessor& p)
	: AudioProcessorEditor(&p), audioProcessor(p), m_grid(p.m_data),
    m_midi_file_filter("*.mid;*.midi", "*.*", "MIDI Files (*.mid;*.midi)"),
    /*
    m_time_slice_thread("File IO Thread"),
	m_directory_contents(&m_midi_file_filter, m_time_slice_thread),
    m_file_list(m_directory_contents)
    */
	m_file_list(juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::openMode, juce::File(), &m_midi_file_filter, nullptr)
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

    addAndMakeVisible(m_pattern_button_parent);
    update_pattern_buttons();
	m_add_pattern_button.setButtonText("+");
	m_add_pattern_button.onClick = [this] {data().set_current_pattern(data().add_pattern()); update_pattern_buttons(); };
    m_pattern_button_parent.addAndMakeVisible(m_add_pattern_button);

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
    m_undo_button.onClick = [this] {data().undo(); set_pattern(data().get_current_pattern_id()); m_grid.repaint(); resize_grid(); };
	m_redo_button.setButtonText("Redo");
	addAndMakeVisible(m_redo_button);
    m_redo_button.onClick = [this] {data().redo(); set_pattern(data().get_current_pattern_id());  m_grid.repaint(); resize_grid(); };

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

	//m_time_slice_thread.startThread();
	//m_directory_contents.setDirectory(juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), true, true);
	addAndMakeVisible(m_file_list);
	m_file_list.setRoot(juce::File(data().m_midi_file_directory));
	m_file_list.addListener(this);

    setSize(770, 448);
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
	int width = MAX_DIVISIONS * m_note_width;
	int height = MAX_LANES * m_note_height;
    resize_grid();
	int grid_bottom = m_grid_top + height;
	m_file_list.setBounds(m_grid_left + width + 8, 32, getWidth() - (m_grid_left + width + 16), getHeight() - 62);

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
    m_swing_slider.setBounds(m_grid_left, 8, 200, 24);

	m_play_sequence_button.setBounds(8, grid_bottom + 36, 24, 24);
    m_sequence_editor.setBounds(m_grid_left, grid_bottom + 36, width-80, 24);
    m_sequence_length_label.setBounds(m_grid_left + width - 80, grid_bottom + 36, 80, 24);

    m_drag_button.setBounds(8, grid_bottom + 64, 64, 24);
}

void DrummerQueenAudioProcessorEditor::delete_lane()
{
}

void DrummerQueenAudioProcessorEditor::drag_onto_pattern(int pattern_index, const juce::String& file)
{

   juce::FileInputStream stream(file);
    if (!stream.openedOk()) {
        return;
    }

    juce::MidiFile midi_file;
    if (!midi_file.readFrom(stream)) {
        return;
    }

    auto ticks_per_beat = midi_file.getTimeFormat();
    if (ticks_per_beat <= 0) {
        return;
    }

    std::set<int> notes;
    std::vector<int> note_on_times;
    auto num_tracks = midi_file.getNumTracks();
	bool waltz = false;
    for (int i = 0; i < num_tracks; ++i)
    {
        const juce::MidiMessageSequence* track = midi_file.getTrack(i);
        for (int j = 0; j < track->getNumEvents(); ++j)
        {
            auto& e = track->getEventPointer(j)->message;
			if (e.isMetaEvent() && e.getMetaEventType() == 0x59)
			{
                auto data = e.getMetaEventData();
                if (data[0] == 3) {
                    waltz = true;
                }
			}
            if (e.isNoteOn())
            {
                int note = e.getNoteNumber();
                notes.insert(note);
				note_on_times.push_back((int)e.getTimeStamp());
            }
        }
    }

    //Decide if pattern should be quantized as shuffle
	float best_error = 1e10f;
	int best_divisions = 16;
    for (auto divisions : { 12, 16 }) {
        float error = 0.f;
		for (auto t : note_on_times) {
            float de = divisions * t / float(ticks_per_beat);
			de = de - std::floor(de);
			if (de > 0.5f) {
				de = 1.f - de;
			}
			error += de * de;
        }
        if (error <= best_error) {
            best_error = error;
            best_divisions = divisions;
        }
    }

    DrumPattern pattern;
	pattern.time_signature.beats = 4;
	pattern.time_signature.beat_divisions = best_divisions / pattern.time_signature.beats;

    std::map<int, int> lane_from_note;
    for (auto note : notes)
    {
        auto it = std::find_if(mt_power.begin(), mt_power.end(), [note](auto const& d) { return d.note == note; });
        if (it != mt_power.end())
        {
            pattern.lanes.push_back({ pattern.time_signature.total_divisions() });
			pattern.lanes.back().note = note;
			lane_from_note[note] = (int)pattern.lanes.size() - 1;
        } // TODO What to do here?
    }

    for (int i = 0; i < num_tracks; ++i)
    {
        const juce::MidiMessageSequence* track = midi_file.getTrack(i);
        for (int j = 0; j < track->getNumEvents(); ++j)
        {
            auto& e = track->getEventPointer(j)->message;
            if (e.isNoteOn())
            {
                int note = e.getNoteNumber();
				int lane = lane_from_note[note];  // TODO What to do here?
				int division = static_cast<int>(pattern.time_signature.beat_divisions * e.getTimeStamp() / ticks_per_beat);
				if (division >= 0 && division < pattern.time_signature.total_divisions())
				{
					pattern.lanes[lane].velocity[division] = e.getVelocity();
				}
            }
        }
    }

	data().set_pattern(pattern_index, pattern);
	set_pattern(pattern_index);
    m_pattern_buttons[pattern_index]->setToggleState(true, juce::dontSendNotification);


	m_grid.repaint();
}

void DrummerQueenAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
	if (slider == &m_swing_slider) {
		data().set_swing(float(m_swing_slider.getValue()));
	}
}

void DrummerQueenAudioProcessorEditor::set_pattern(int index)
{
    data().set_current_pattern(index);
	auto const &pattern = data().get_current_pattern();
    for (auto& pb : m_lane_combo_boxes)
    {
        removeChildComponent(pb.get());
    }
    for (auto& pb : m_lane_name_buttons)
    {
        removeChildComponent(pb.get());
    }

    m_lane_combo_boxes.clear();
	m_lane_name_buttons.clear();
    int y = 64;
    for (int i = 0; i < data().lane_count(); ++i) {
        m_lane_combo_boxes.push_back(std::make_unique<DrumNoteComboBox>());
        for (auto const& drum : mt_power) {
            m_lane_combo_boxes.back()->addItem(drum.name, drum.note);
        }
        m_lane_combo_boxes.back()->onChange = [this, i]
        {
            data().set_lane_note(i, m_lane_combo_boxes[i]->getSelectedId());
            m_lane_name_buttons[i]->setButtonText(m_lane_combo_boxes[i]->getText());
        };
        m_lane_combo_boxes.back()->setSelectedId(pattern.lanes[i].note, juce::dontSendNotification);
        m_lane_combo_boxes.back()->setBounds(8 + 109, y, 24, 25);
        addAndMakeVisible(*m_lane_combo_boxes.back());
        
        m_lane_name_buttons.push_back(std::make_unique<juce::TextButton>(m_lane_combo_boxes.back()->getText()));
        m_lane_name_buttons.back()->setBounds(8, y, 108, 25);
		m_lane_name_buttons.back()->setConnectedEdges(juce::Button::ConnectedOnLeft | juce::Button::ConnectedOnRight);
		m_lane_name_buttons.back()->onStateChange = [this, i]
			{
				if (m_lane_name_buttons[i]->getState() == juce::Button::buttonDown)
				{
                    audioProcessor.play_note(data().get_current_pattern().lanes[i].note);;
				}
			};
		addAndMakeVisible(*m_lane_name_buttons.back());
        y += 24;
    }
	if (data().lane_count() < MAX_LANES) {
		m_add_lane_button.setButtonText("+");
		m_add_lane_button.setBounds(8, y, 108, 25);
		m_add_lane_button.onClick = [this]
			{
				if (data().lane_count() < MAX_LANES)
				{
					data().add_drum("Drum", mt_power[0].note);
					set_pattern(data().get_current_pattern_id());
					resize_grid();
					m_grid.repaint();
				}
			};
		addAndMakeVisible(m_add_lane_button);
	}
	else
	{
		removeChildComponent(&m_add_lane_button);
	}
    resize_grid();
    m_grid.repaint();
}

void DrummerQueenAudioProcessorEditor::update_pattern_buttons()
{
	for (auto& pb : m_pattern_buttons)
	{
		removeChildComponent(pb.get());
	}
	m_pattern_buttons.clear();

	int n_patterns = data().pattern_count();
    for (int i = 0; i < n_patterns; ++i)
    {
        m_pattern_buttons.emplace_back(std::make_unique<PatternButton>(this, i));
        m_pattern_buttons.back()->onClick = [this, i] {set_pattern(i);};
		m_pattern_buttons.back()->setRadioGroupId(2);
        m_pattern_button_parent.addAndMakeVisible(m_pattern_buttons.back().get());
    }
	m_pattern_buttons[data().get_current_pattern_id()]->setToggleState(true, juce::dontSendNotification);
    int height = MAX_LANES * m_note_height;
    int grid_bottom = m_grid_top + height;
    m_pattern_button_parent.setBounds(m_grid_left, grid_bottom + 8, 24 * 16, 24);
    int x = 0;
    int y = 0;
    for (auto& pb : m_pattern_buttons)
    {
        pb->setBounds(x, y, 24, 24);
        x += 26;
    }
    m_add_pattern_button.setBounds(x, y, 24, 24);

}

void DrummerQueenAudioProcessorEditor::add_time_signature(char const* name, int beats, int beat_divisions)
{
	m_time_signature_box.addItem(name, int(m_time_signatures.size() + 1));
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

void DrummerQueenAudioProcessorEditor::selectionChanged()
{
    if (m_file_list.getNumSelectedFiles() > 0) {
        drag_onto_pattern(data().get_current_pattern_id(), m_file_list.getSelectedFile(0).getFullPathName());
    }
}

void DrummerQueenAudioProcessorEditor::browserRootChanged(const juce::File& newRoot)
{
	data().m_midi_file_directory = newRoot.getFullPathName().toStdString();
}

void DrummerQueenAudioProcessorEditor::drag_midi()
{
    juce::File tempDirectory = juce::File::getSpecialLocation(juce::File::tempDirectory);
    juce::File tempFile = tempDirectory.getChildFile("pattern.midi");
    juce::MidiMessageSequence midi_sequence;
    auto sequence = data().is_playing_sequence() ? data().get_sequence() : std::vector<SequenceItem>{ {.0f,4.f, data().get_current_pattern_id() } };
    int tpq = 960;
	for (auto p : sequence)
	{
        // TODO fix this!
        data().get_events(p.pattern, 0., data().beats(), p.start_beat, data().beats() * tpq, midi_sequence);
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

void DrumNoteComboBox::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::white);
    g.drawRect(getLocalBounds());
    char text[2] = { 'v', 0 };
    g.drawText(text, getLocalBounds(), juce::Justification::centred);
}

void PatternButton::paintButton(juce::Graphics& g, bool, bool)
{
    g.fillAll(juce::Colours::black);
	g.setColour(getToggleState() ? juce::Colours::white : juce::Colours::grey);

    char label[2] = { char(m_pattern) + 'A', 0 };
    g.drawText(label, getLocalBounds(), juce::Justification::centred);
}



bool PatternButton::isInterestedInFileDrag(const juce::StringArray&)
{
    return true;
}

void PatternButton::filesDropped(const juce::StringArray& files, int, int)
{
    if (files.size() > 0) {
        m_editor->drag_onto_pattern(m_pattern, files[0]);
    }
}
