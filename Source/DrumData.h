#pragma once

#include <JuceHeader.h>

#include <vector>
#include <memory>
#include <string>
#include <fstream>

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
	int pattern = 0;
};


class DrumData
{
public:
    DrumData(DrumDataListener &listener)
		: m_listener(listener), m_patterns(1),
		m_midi_file_directory(juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getFullPathName().toStdString())
    {
	}

    void add_drum(std::string name, int note);

	int add_pattern();
	void set_current_pattern(int pattern);
	int get_current_pattern_id() const { return m_current_pattern; }
	const DrumPattern & get_current_pattern() { return m_patterns[m_current_pattern]; }
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
	int pattern_count() const { return (int)m_patterns.size(); }

	void set_swing(float swing);

	int lane_count() const;

	void set_hit(int lane, int division, int velocity);
	int get_hit(int lane, int division) const;

	void clear_hits();

	template <typename MB>
	void get_events(int pattern, double start_time, double end_time, double offset_time, int num_samples, MB& midiMessages)
	{
		for (auto& e : m_patterns[pattern].m_events)
		{
			if (e.beat_time >= start_time && e.beat_time < end_time)
			{
				double time_fraction = (e.beat_time + offset_time - start_time) / (end_time - start_time);
				auto sample_time = static_cast<int>(time_fraction * num_samples);
				midiMessages.addEvent(juce::MidiMessage::noteOn(1, e.note, juce::uint8(e.velocity)), sample_time);
			}
		}
	}

	template <typename MB>
	void get_events(double start_time, double end_time, int num_samples, MB& midiMessages)
	{
		get_events(m_current_pattern, start_time, end_time, 0.0, num_samples, midiMessages);
	}

	std::string to_json() const;
	void from_json(std::string const& json);

	friend std::ostream& operator<<(std::ostream& out, const DrumData& data);
	friend std::istream& operator>>(std::istream& in, DrumData& data);

	void undo();
	void redo();

	std::string m_midi_file_directory;

private:
	void update_events();
	void update_events(int pattern);
	std::vector<DrumPattern> m_patterns;
	int m_current_pattern = 0;
	float m_swing = 0.5f;

	std::string m_sequence_str;
	std::vector<SequenceItem> m_sequence;
	bool m_play_sequence = false;
	int m_sequence_length = 0;


	DrumDataListener& m_listener;

	std::vector<Action> m_undo_stack;
	std::vector<Action> m_redo_stack;

	void do_action(std::function<void()> do_action, std::function<void()> undo_action);
};
