/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include <vector>
#include <memory>
#include <string>
#include <fstream>
//==============================================================================
/**
*/
class DrumLane
{
public:
    DrumLane(std::string name, int note, int divisions) : m_name(name), m_note(note) { m_velocity.resize(divisions); }
    std::string m_name;
    int m_note;
    std::vector<int> m_velocity;
};

struct DrumEvent
{
    double beat_time;
    int lane;
	int velocity;
};

class DrumDataListener
{
public:
	virtual void changed() = 0;
};

class DrumData
{
public:
    DrumData(int beats, int beat_divisions, DrumDataListener &listener)
		: m_beats(beats), m_beat_divisions(beat_divisions), m_listener(listener)
    {}

    void add_drum(std::string name, int note)
    {
        m_lanes.emplace_back(name, note, m_beats * m_beat_divisions);
    }

    int beats() const { return m_beats; }
    int beat_divisions() const { return m_beat_divisions; }
	int total_divisions() const { return m_beats * m_beat_divisions; }
	int lane_count() const { return m_lanes.size(); }

	void set_swing(float swing);

	void set_hit(int lane, int division, int velocity);
	int get_hit(int lane, int division) const;

    std::string get_lane_name(int lane) const;
    void set_lane_name(int lane, std::string const& name);

    int get_lane_note(int lane) const;
    void set_lane_note(int lane, int note);

	void clear_hits();

	void get_events(double start_time, double end_time, int num_samples, juce::MidiBuffer& midiMessages);

	friend std::ostream& operator<<(std::ostream& out, const DrumData& data);
	friend std::istream& operator>>(std::istream& in, DrumData& data);

private:
	void update_events();
    std::vector<DrumLane> m_lanes;
	std::vector<DrumEvent> m_events;
    int m_beats = 4;
    int m_beat_divisions = 4;
	float m_swing = 0.5f;

	DrumDataListener& m_listener;
};
