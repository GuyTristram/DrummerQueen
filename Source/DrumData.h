#pragma once

#include <JuceHeader.h>

#include <vector>
#include <memory>
#include <string>
#include <fstream>

struct DrumInfo
{
	std::string name;
	int note;
};

struct DrumKit
{
	std::string name;
	std::vector<DrumInfo> drums;
};

struct DrumLane
{
	DrumLane(int divisions) : velocity(divisions) {}
    std::vector<int> velocity;
};

struct DrumEvent
{
	double beat_time;
	int lane;
	int velocity;
};

struct DrumPattern
{
	DrumPattern() = default;
	DrumPattern(int drum_count, int divisions)
	{
		for (int i = 0; i < drum_count; ++i)
		{
			lanes.emplace_back(divisions);
		}
	}
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

class DrumData
{
public:
    DrumData(int beats, int beat_divisions, DrumDataListener &listener)
		: m_beats(beats), m_beat_divisions(beat_divisions), m_listener(listener), m_patterns(1)
    {
	}

    void add_drum(std::string name, int note);

	int add_pattern();
	void set_current_pattern(int pattern);
	int get_current_pattern() const { return m_current_pattern; }

	void set_sequence_str(std::string const& sequence);
	void play_sequence(bool ps);
	bool is_playing_sequence() const { return m_play_sequence; }
	std::vector<int> const& get_sequence() const { return m_sequence; }

    int beats() const { return m_beats; }
    int beat_divisions() const { return m_beat_divisions; }
	int total_divisions() const { return m_beats * m_beat_divisions; }
	int lane_count() const { return m_kit.drums.size(); }
	int pattern_count() const { return m_patterns.size(); }

	void set_swing(float swing);

	void set_hit(int lane, int division, int velocity);
	int get_hit(int lane, int division) const;

    std::string get_lane_name(int lane) const;
    void set_lane_name(int lane, std::string const& name);

    int get_lane_note(int lane) const;
    void set_lane_note(int lane, int note);

	void clear_hits();

	void get_events(double start_time, double end_time, int num_samples, juce::MidiBuffer& midiMessages);

	std::string to_json() const;
	void from_json(std::string const& json);

	friend std::ostream& operator<<(std::ostream& out, const DrumData& data);
	friend std::istream& operator>>(std::istream& in, DrumData& data);

	void undo();
	void redo();

private:
	void update_events(int pattern);
	DrumKit m_kit;
	std::vector<DrumPattern> m_patterns;
	int m_current_pattern = 0;
    int m_beats = 4;
    int m_beat_divisions = 4;
	float m_swing = 0.5f;

	std::string m_sequence_str;
	std::vector<int> m_sequence;
	bool m_play_sequence = false;

	DrumDataListener& m_listener;

	std::vector<Action> m_undo_stack;
	std::vector<Action> m_redo_stack;

	void do_action(std::function<void()> do_action, std::function<void()> undo_action);
};
