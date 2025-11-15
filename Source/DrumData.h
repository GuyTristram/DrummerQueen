#pragma once

#include <JuceHeader.h>

#include <vector>
#include <memory>
#include <string>
#include <fstream>

const int MAX_LANES = 12;
const int MAX_DIVISIONS = 32;

struct DrumInfo
{
	int note;
	std::string name;
};

struct DrumKit
{
	std::string name;
	std::vector<DrumInfo> drums;
};

struct DrumLane
{
	DrumLane(int divisions) : velocity(divisions) {}
	int note = 0;
    std::vector<int> velocity;
};

struct DrumEvent
{
	double beat_time;
	int note;
	int velocity;
};

struct TimeSignature
{
	int beats = 4;
	int beat_divisions = 4;
	int total_divisions() const { return beats * beat_divisions; }
};

struct DrumPattern
{
	TimeSignature time_signature;
	std::vector<DrumLane> lanes;
	std::vector<DrumEvent> m_events;
};

class DrumDataListener
{
public:
	virtual void changed() = 0;
};

struct Action
{
	std::function<void()> do_action;
	std::function<void()> undo_action;
};

struct SequenceItem
{
	double start_beat = 0.f;
	double end_beat = 0.f;
	int pattern = 0;
};


class DrumData
{
public:
    DrumData(DrumDataListener &listener);

    void add_drum(std::string name, int note);

	void set_current_pattern(int pattern);
	int get_current_pattern_id() const { return m_current_pattern; }
	const DrumPattern& get_current_pattern() { return m_patterns[m_current_pattern]; }
	const DrumPattern& get_pattern(int i) { return m_patterns[i]; }
	void set_pattern(int pattern_index, DrumPattern const &pattern);
	void set_lane_note(int lane, int note);

	void set_sequence_str(std::string const& sequence);
	std::string get_sequence_str() const { return m_sequence_str; }
	int sequence_length() const { return m_sequence_length; }
	void play_sequence(bool ps);
	bool is_playing_sequence() const { return m_play_sequence; }
	std::vector<SequenceItem> const& get_sequence() const { return m_sequence; }

    int beats() const { return m_patterns[m_current_pattern].time_signature.beats; }
    int beat_divisions() const { return m_patterns[m_current_pattern].time_signature.beat_divisions; }
	int total_divisions() const { return m_patterns[m_current_pattern].time_signature.total_divisions(); }
	void set_time_signature(int beats, int beat_divisions);
	bool set_hit_at_time(double beat_time, int note, int velocity);


	int pattern_count() const { return (int)m_patterns.size(); }

	void set_swing(float swing);

	int lane_count() const;

	void set_hit(int lane, int division, int velocity);
	int get_hit(int lane, int division) const;

	void clear_hits();
	void clear_all();

	std::vector<SequenceItem> const& get_playing_sequence() const;
	double get_wrapped_time(double time_beats) const;
	int get_sequence_index(double time_beats) const;

	template <typename MB>
	void get_events(int pattern, double start_time, double end_time, double offset_time, int num_samples, MB& midiMessages);
	template <typename MB>
	void get_events(double start_time, double end_time, int num_samples, MB& midiMessages);

	std::string to_json() const;
	void from_json(std::string const& json);

	void undo();
	void redo();

	std::string m_midi_file_directory;

	int get_current_kit() const { return m_current_kit; }
	void set_current_kit(int kit_index) { m_current_kit = kit_index; }
	std::vector<std::string> get_kit_names() const;
	std::vector<DrumInfo> const &get_current_kit_drums() const;
	std::string get_drum_name(int note) const;
	static const int NUM_PATTERNS = 16;
	using PatternArray = std::array<DrumPattern, NUM_PATTERNS>;

private:
	void update_events();
	void update_events(int pattern);
	void update_sequence();
	PatternArray m_patterns;
	int m_current_pattern = 0;
	float m_swing = 0.5f;

	std::string m_sequence_str;
	std::vector<SequenceItem> m_sequence;
	mutable std::vector<SequenceItem> m_current_pattern_sequence;
	bool m_play_sequence = false;
	int m_sequence_length = 0;

	DrumDataListener& m_listener;

	std::vector<Action> m_undo_stack;
	std::vector<Action> m_redo_stack;

	void do_action(std::function<void()> do_action, std::function<void()> undo_action);

	std::vector<DrumKit> m_kits;
	int m_current_kit = 0;
	void load_kits();

};

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Template Impementations
//
///////////////////////////////////////////////////////////////////////////////////////////////////

template <typename MB>
inline void DrumData::get_events(int pattern, double start_time, double end_time, double offset_time, int num_samples, MB& midiMessages)
{
	for (auto& e : m_patterns[pattern].m_events) {
		if (e.beat_time >= start_time && e.beat_time < end_time) {
			double time_fraction = (e.beat_time + offset_time - start_time) / (end_time - start_time);
			auto sample_time = static_cast<int>(time_fraction * num_samples);
			midiMessages.addEvent(juce::MidiMessage::noteOn(1, e.note, juce::uint8(e.velocity)), sample_time);
		}
	}
}

template <typename MB>
inline void DrumData::get_events(double start_time, double end_time, int num_samples, MB& midiMessages)
{
	auto const& sequence = get_playing_sequence();
	if (sequence.size() == 0) {
		return;
	}
	double sequence_length_beats = sequence.back().end_beat;
	double start_time_wrap = fmod(start_time, sequence_length_beats);
	double num_wraps = std::floor(start_time / sequence_length_beats);
	double end_time_wrap = end_time - start_time + start_time_wrap;
	int seq_index = get_sequence_index(start_time);

	double time = start_time;
	while (time < end_time) {
		auto& item = sequence[seq_index];
		get_events(item.pattern, start_time_wrap - item.start_beat, end_time_wrap - item.start_beat, item.start_beat + num_wraps * sequence_length_beats, num_samples, midiMessages);
		time += item.end_beat - start_time_wrap;
		seq_index = (seq_index + 1) % sequence.size();
		if (seq_index == 0) {
			num_wraps += 1.;
			start_time_wrap -= sequence_length_beats;
			end_time_wrap -= sequence_length_beats;
		}
	}
}
