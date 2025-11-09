#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <format>
#include "json.hpp"
#include "share.c"

namespace
{
const int MAX_LANES = 12;
const int MAX_DIVISIONS = 32;
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
    for (auto& vb : m_velocity_buttons) {
        vb = std::make_unique<VelocityButton>(velocities[i]);
        vb->setRadioGroupId(1);
        vb->onClick = [this, i, v = velocities[i]] {
            m_grid.set_new_velocity(v); m_velocity_button_selected = i; };
        addAndMakeVisible(vb.get());
        ++i;
    }
	m_velocity_buttons.front()->setToggleState(true, juce::dontSendNotification);

    int n_patterns = data().pattern_count();
    for (int p_index = 0; p_index < n_patterns; ++p_index) {
        m_pattern_buttons.emplace_back(std::make_unique<PatternButton>(this, p_index));
        m_pattern_buttons.back()->onClick = [this, p_index] {set_pattern(p_index, false); };
        m_pattern_buttons.back()->setRadioGroupId(2);
        m_pattern_button_parent.addAndMakeVisible(m_pattern_buttons.back().get());
    }
    addAndMakeVisible(m_pattern_button_parent);
    update_pattern_buttons();

	m_sequence_editor.setText(data().get_sequence_str());
    addAndMakeVisible(m_sequence_editor);
	m_sequence_editor.onTextChange = [this] {
            data().set_sequence_str(m_sequence_editor.getText().toStdString());
			m_sequence_editor.setText(data().get_sequence_str(), juce::dontSendNotification);
			m_sequence_length_label.setText(std::format("Len: {}", data().sequence_length()), juce::dontSendNotification);
        };
	addAndMakeVisible(m_play_sequence_button);
    m_play_sequence_button.setToggleState(data().is_playing_sequence(), juce::dontSendNotification);
	m_play_sequence_button.onClick = [this] {data().play_sequence(m_play_sequence_button.getToggleState()); };
    addAndMakeVisible(m_sequence_length_label);

	m_undo_button.setButtonText("Undo");
	m_undo_button.setConnectedEdges(juce::Button::ConnectedOnRight);
	addAndMakeVisible(m_undo_button);
    m_undo_button.onClick = [this] {data().undo(); set_pattern(data().get_current_pattern_id()); m_grid.repaint(); resize_grid(); };
	m_redo_button.setButtonText("Redo");
    m_redo_button.setConnectedEdges(juce::Button::ConnectedOnLeft);
    addAndMakeVisible(m_redo_button);
    m_redo_button.onClick = [this] {data().redo(); set_pattern(data().get_current_pattern_id());  m_grid.repaint(); resize_grid(); };

	// TODO: decide whether to keep swing
    if (false) {
        m_swing_slider.setSliderStyle(juce::Slider::LinearHorizontal);
        m_swing_slider.setRange(0.0, 1.0, 0.05);
        m_swing_slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 90, 0);
        m_swing_slider.setPopupDisplayEnabled(true, false, this);
        m_swing_slider.setTextValueSuffix(" Swing");
        m_swing_slider.setValue(0.5);
        m_swing_slider.addListener(this);
        addAndMakeVisible(m_swing_slider);
    }

    add_time_signature("4/4", 4, 4);
    add_time_signature("3/4", 3, 4);
    add_time_signature("4/3", 4, 3);
    add_time_signature("4/4 * 2", 8, 4);
    add_time_signature("3/4 * 2", 6, 4);
    add_time_signature("4/3 * 2", 8, 3);
    select_time_signature();
    m_time_signature_box.onChange = [this] {
        int i = m_time_signature_box.getSelectedId() - 1;
        data().set_time_signature(m_time_signatures[i].beats, m_time_signatures[i].beat_divisions);
        resize_grid();
        m_grid.repaint();
    };
    addAndMakeVisible(m_time_signature_box);

	auto kit_names = data().get_kit_names();
	for (int k = 0; k < kit_names.size(); ++k) {
		m_drum_kit_box.addItem(kit_names[k], k + 1);
	}
	m_drum_kit_box.setSelectedId(data().get_current_kit() + 1);
	m_drum_kit_box.onChange = [this] {
        int kit_index = m_drum_kit_box.getSelectedId() - 1;
	    data().set_current_kit(kit_index);
		set_pattern(data().get_current_pattern_id(), false);
	};

    addAndMakeVisible(m_drum_kit_box);


    m_drag_button.setButtonText("Drag");
	m_drag_button.onStartDrag = [this] { drag_midi(); };
    addAndMakeVisible(m_drag_button);

	set_pattern(0);

	//m_time_slice_thread.startThread();
	//m_directory_contents.setDirectory(juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), true, true);
	addAndMakeVisible(m_file_list);
	m_file_list.setRoot(juce::File(data().m_midi_file_directory));
	m_file_list.addListener(this);

    layout_components();
    setSize(1124, 448);
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
}

void DrummerQueenAudioProcessorEditor::resize_grid()
{
    const int width = data().total_divisions() * m_note_width + 1;
    const int height = data().lane_count() * m_note_height + 1;
    m_grid.setBounds(m_grid_left, m_grid_top, width, height);
    const int width_min = std::max(data().total_divisions(), 16) * m_note_width + 1;
    setSize(m_grid_left + width_min + 8, 448);
}


void DrummerQueenAudioProcessorEditor::layout_components()
{
	const int width = MAX_DIVISIONS * m_note_width;
    resize_grid();

	m_file_list.setBounds(0, 0, m_lane_button_left, getHeight());

	m_undo_button.setBounds(m_lane_button_left, 8, 40, 24);
	m_redo_button.setBounds(m_lane_button_left + 40, 8, 40, 24);

    const int butt_size = 24;
    const int butt_spacing = butt_size + 2;

    const int n_patterns = data().pattern_count();
    const int butt_per_line = n_patterns / 2;
    m_pattern_button_parent.setBounds(m_grid_left, 8, butt_spacing * n_patterns / 2, butt_spacing * 2);
    int i = 0;
    for (auto& pb : m_pattern_buttons) {
        pb->setBounds(butt_spacing * (i % butt_per_line), butt_spacing * (i / butt_per_line), butt_size, butt_size);
        ++i;
    }

    int x = m_grid_left;
    for (auto& vb : m_velocity_buttons) {
		vb->setBounds(x, m_grid_top - 26, 24, 24);
		x += 26;
    }
    x += 24;
    m_time_signature_box.setBounds(x, m_grid_top - 26, 120, 24);
    m_swing_slider.setBounds(m_grid_left, 8, 200, 24);
	m_drum_kit_box.setBounds(m_lane_button_left, m_grid_top-24, m_lane_button_width, 24);

	const int seq_y = butt_size * 2 + 16;
    m_drag_button.setBounds(m_lane_button_left, seq_y, 24, 24);
    m_play_sequence_button.setBounds(m_lane_button_left + butt_spacing, seq_y, 24, 24);
    m_sequence_editor.setBounds(m_lane_button_left + butt_spacing*2, seq_y, 430, 24);
    m_sequence_length_label.setBounds(m_grid_left + width - 80, seq_y, 80, 24);

}

void DrummerQueenAudioProcessorEditor::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
	if (!m_grid.isMouseOver()) {
		return;
	}
    if (wheel.deltaY < 0 && m_velocity_button_selected < m_velocity_buttons.size() - 1) {
		m_velocity_buttons[m_velocity_button_selected + 1]->setToggleState(true, juce::sendNotification);
	}
    else if (wheel.deltaY > 0 && m_velocity_button_selected > 0) {
        m_velocity_buttons[m_velocity_button_selected - 1]->setToggleState(true, juce::sendNotification);
    }
}

void DrummerQueenAudioProcessorEditor::changeListenerCallback(juce::ChangeBroadcaster*)
{
	auto midi_events = audioProcessor.get_recorded_midi();
	for (auto& e : midi_events) {
		data().set_hit_at_time(e.beat_time, e.note, e.velocity);
	}
    
    auto bar_pos_beats = audioProcessor.barPos();
    m_grid.set_position(bar_pos_beats);
    if (bar_pos_beats > 0.f && data().is_playing_sequence()) {
        auto index = data().get_sequence_index(bar_pos_beats);
        auto pattern = data().get_playing_sequence()[index].pattern;
        if (pattern != data().get_current_pattern_id()) {
            set_pattern(pattern);
        }
    }
    repaint();
}

void DrummerQueenAudioProcessorEditor::delete_lane()
{
}

namespace {
	DrumPattern import_midi_file(const juce::String& file)
	{
		DrumPattern pattern;


        juce::MidiFile midi_file;
        juce::FileInputStream stream(file);
        if (!stream.openedOk() || !midi_file.readFrom(stream) || midi_file.getTimeFormat() <= 0) {
            return pattern;
        }

        auto ticks_per_beat = midi_file.getTimeFormat();

        std::set<int> notes;
        std::vector<double> note_on_times;
        auto num_tracks = midi_file.getNumTracks();
        for (int i = 0; i < num_tracks; ++i) {
            const juce::MidiMessageSequence* track = midi_file.getTrack(i);
            for (int j = 0; j < track->getNumEvents(); ++j) {
                auto& e = track->getEventPointer(j)->message;
                if (e.isNoteOn()) {
                    int note = e.getNoteNumber();
                    notes.insert(note);
                    note_on_times.push_back((int)e.getTimeStamp());
                }
            }
        }

        double max_time = 0.;
        for (auto t : note_on_times) {
            max_time = std::max(max_time, t);
        }

        //Decide if pattern should be quantized as shuffle
        double best_error = 1e10f;
        int best_divisions = 4;
        for (auto divisions : { 3, 4 }) {
            double error = 0.f;
            for (auto t : note_on_times) {
                double de = divisions * t / double(ticks_per_beat);
                de = de - std::round(de);
                error += de * de;
            }
            if (error <= best_error) {
                best_error = error;
                best_divisions = divisions;
            }
        }

        pattern.time_signature.beats = max_time / ticks_per_beat > 4 ? 8 : 4;
        pattern.time_signature.beat_divisions = best_divisions;

        std::map<int, int> lane_from_note;
        for (auto note : notes) {
            pattern.lanes.push_back({ pattern.time_signature.total_divisions() });
            pattern.lanes.back().note = note;
            lane_from_note[note] = (int)pattern.lanes.size() - 1;
        }

        for (int i = 0; i < num_tracks; ++i) {
            const juce::MidiMessageSequence* track = midi_file.getTrack(i);
            for (int j = 0; j < track->getNumEvents(); ++j) {
                auto& e = track->getEventPointer(j)->message;
                if (e.isNoteOn()) {
                    int note = e.getNoteNumber();
                    int lane = lane_from_note[note];
                    int division = static_cast<int>(pattern.time_signature.beat_divisions * e.getTimeStamp() / ticks_per_beat);
                    if (division >= 0 && division < pattern.time_signature.total_divisions()) {
                        pattern.lanes[lane].velocity[division] = e.getVelocity();
                    }
                }
            }
        }
        return pattern;
	}
}


void DrummerQueenAudioProcessorEditor::drag_onto_pattern(int pattern_index, const juce::String& file)
{
	juce::File f(file);
	if (!f.existsAsFile()) {
		return;
	}
	data().set_pattern(pattern_index, import_midi_file(file));
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

void DrummerQueenAudioProcessorEditor::set_pattern(int index, bool update_button)
{
    data().set_current_pattern(index);
    if (update_button) {
        m_pattern_buttons[index]->setToggleState(true, juce::dontSendNotification);
    }
    auto const &pattern = data().get_current_pattern();
    for (auto& pb : m_lane_combo_boxes) {
        removeChildComponent(pb.get());
    }
    for (auto& pb : m_lane_name_buttons) {
        removeChildComponent(pb.get());
    }

    m_lane_combo_boxes.clear();
	m_lane_name_buttons.clear();
	auto drums = data().get_current_kit_drums();
    int y = m_grid_top;
    for (int i = 0; i < data().lane_count(); ++i) {
        m_lane_combo_boxes.push_back(std::make_unique<DrumNoteComboBox>());
        for (auto const& drum : drums) {
            m_lane_combo_boxes.back()->addItem(drum.name, drum.note);
        }
        m_lane_combo_boxes.back()->onChange = [this, i] {
            data().set_lane_note(i, m_lane_combo_boxes[i]->getSelectedId());
            m_lane_name_buttons[i]->setButtonText(m_lane_combo_boxes[i]->getText());
        };
        m_lane_combo_boxes.back()->setSelectedId(pattern.lanes[i].note, juce::dontSendNotification);
        m_lane_combo_boxes.back()->setBounds(m_grid_left-24, y, 24, 25);
        addAndMakeVisible(*m_lane_combo_boxes.back());
        
        m_lane_name_buttons.push_back(std::make_unique<LaneButton>(data().get_drum_name(pattern.lanes[i].note)));
        m_lane_name_buttons.back()->setBounds(m_lane_button_left, y, m_lane_button_width, 25);
		m_lane_name_buttons.back()->setConnectedEdges(juce::Button::ConnectedOnLeft | juce::Button::ConnectedOnRight);
		m_lane_name_buttons.back()->onStateChange = [this, i] {
				if (m_lane_name_buttons[i]->getState() == juce::Button::buttonDown) {
                    audioProcessor.play_note(data().get_current_pattern().lanes[i].note);;
				}
			};
		addAndMakeVisible(*m_lane_name_buttons.back());
        y += 24;
    }
	if (data().lane_count() < MAX_LANES) {
		m_add_lane_button.setButtonText("+");
		m_add_lane_button.setBounds(m_lane_button_left, y, m_lane_button_width, 25);
		m_add_lane_button.onClick = [this] {
				if (data().lane_count() < MAX_LANES) {
					data().add_drum("Drum", 36);
					set_pattern(data().get_current_pattern_id(), false);
					resize_grid();
					m_grid.repaint();
                    m_lane_combo_boxes.back()->showPopup();
				}
			};
		addAndMakeVisible(m_add_lane_button);
	}
	else {
		removeChildComponent(&m_add_lane_button);
	}
    resize_grid();
    m_grid.repaint();
    select_time_signature();
}

void DrummerQueenAudioProcessorEditor::update_pattern_buttons()
{
    m_pattern_buttons[data().get_current_pattern_id()]->setToggleState(true, juce::dontSendNotification);
}

void DrummerQueenAudioProcessorEditor::add_time_signature(char const* name, int beats, int beat_divisions)
{
	m_time_signature_box.addItem(name, int(m_time_signatures.size() + 1));
	m_time_signatures.push_back({ beats, beat_divisions });
}

void DrummerQueenAudioProcessorEditor::select_time_signature()
{
	for (int i = 0; i < m_time_signatures.size(); ++i) {
		if (m_time_signatures[i].beats == data().beats() && m_time_signatures[i].beat_divisions == data().beat_divisions()) {
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

void drag_midi_sequence(std::vector<SequenceItem> const& sequence, DrumData &data, const char *suffix)
{
    juce::File tempDirectory = juce::File::getSpecialLocation(juce::File::tempDirectory);
	std::string filename = "dq_pattern";
    juce::File tempFile = tempDirectory.getChildFile(std::format("pattern{}.midi", suffix));
    juce::MidiMessageSequence midi_sequence;
    int tpq = 960;
    for (auto p : sequence) {
        auto pattern_length = p.end_beat - p.start_beat;
        data.get_events(p.pattern, 0., pattern_length, p.start_beat, int(pattern_length * tpq), midi_sequence);
    }
    juce::MidiFile midi_file;
    midi_file.setTicksPerQuarterNote(tpq);
    midi_file.addTrack(midi_sequence);
    auto stream = tempFile.createOutputStream();
    if (stream) {
        stream->setPosition(0);
        stream->truncate();
        midi_file.writeTo(*stream);
        stream->flush();
        stream = nullptr;
        juce::DragAndDropContainer::performExternalDragDropOfFiles({ tempFile.getFullPathName() }, true);
    }
}

void DrummerQueenAudioProcessorEditor::drag_midi()
{
	drag_midi_sequence(data().get_playing_sequence(), data());
}
