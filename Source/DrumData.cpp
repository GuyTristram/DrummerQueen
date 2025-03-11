#include "DrumData.h"

#include "json.hpp"

void DrumData::add_drum(std::string name, int note)
{
	m_kit.drums.emplace_back(name, note);
	for (auto& pattern : m_patterns)
	{
		pattern.lanes.emplace_back(m_beats * m_beat_divisions);
	}
}

int DrumData::add_pattern()
{
	//m_patterns.emplace_back((int)m_kit.drums.size(), m_beats * m_beat_divisions);
	m_patterns.emplace_back(m_patterns[m_current_pattern]);
	m_listener.changed();
	return m_patterns.size() - 1;
}

void DrumData::set_current_pattern(int pattern) 
{
	if (pattern < 0 || pattern >= m_patterns.size())
	{
		return;
	}
	m_current_pattern = pattern;
}

void DrumData::set_swing(float swing)
{
	m_swing = swing;
	for (int i = 0; i < m_patterns.size(); ++i)
	{
		update_events(i);
	}
}

void DrumData::set_hit(int lane, int division, int velocity)
{
	m_patterns[m_current_pattern].lanes[lane].velocity[division] = velocity;
	m_listener.changed();
	update_events(m_current_pattern);
}

int DrumData::get_hit(int lane, int division) const
{
	return m_patterns[m_current_pattern].lanes[lane].velocity[division];
}

std::string DrumData::get_lane_name(int lane) const
{
	return m_kit.drums[lane].name;
}

void DrumData::set_lane_name(int lane, std::string const &name)
{
	m_kit.drums[lane].name = name;
}

int DrumData::get_lane_note(int lane) const
{
	return m_kit.drums[lane].note;
}

void DrumData::set_lane_note(int lane, int note)
{
	m_kit.drums[lane].note = note;
}

void DrumData::clear_hits()
{
	for (auto& lane : m_patterns[m_current_pattern].lanes)
	{
		for (auto& v : lane.velocity)
		{
			v = 0;
		}
	}
	update_events(m_current_pattern);
}

void DrumData::get_events(double start_time, double end_time, int num_samples, juce::MidiBuffer& midiMessages)
{
	for (auto& e : m_patterns[m_current_pattern].m_events)
	{
		if (e.beat_time >= start_time && e.beat_time < end_time)
		{
			double time_fraction = (e.beat_time - start_time) / (end_time - start_time);
			auto sample_time = static_cast<int>(time_fraction * num_samples);
			midiMessages.addEvent(juce::MidiMessage::noteOn(1, m_kit.drums[e.lane].note, juce::uint8(e.velocity)), sample_time);
		}
	}
}

std::string DrumData::to_json() const
{
	using json = nlohmann::json;
	json j;
	j["beats"] = m_beats;
	j["beat_divisions"] = m_beat_divisions;
	j["swing"] = m_swing;
	j["kit"] = json::object();
	j["kit"]["name"] = m_kit.name;
	j["kit"]["drums"] = json::array();
	for (auto& drum : m_kit.drums)
	{
		json d;
		d["name"] = drum.name;
		d["note"] = drum.note;
		j["kit"]["drums"].push_back(d);
	}
	j["patterns"] = json::array();
	for (auto& pattern : m_patterns)
	{
		json p;
		p["lanes"] = json::array();
		for (auto& lane : pattern.lanes)
		{
			json l;
			l["velocity"] = lane.velocity;
			p["lanes"].push_back(l);
		}
		j["patterns"].push_back(p);
	}
	return j.dump(3);
}

void DrumData::from_json(std::string const& json_string)
{
	using json = nlohmann::json;
	auto j = json::parse(json_string);
	m_beats = j["beats"];
	m_beat_divisions = j["beat_divisions"];
	m_swing = j["swing"];
	m_kit.name = j["kit"]["name"];
	m_kit.drums.clear();
	for (auto& d : j["kit"]["drums"])
	{
		m_kit.drums.emplace_back(d["name"], d["note"]);
	}
	m_patterns.clear();
	int pattern_count = 0;
	for (auto& p : j["patterns"])
	{
		DrumPattern pattern;
		for (auto& l : p["lanes"])
		{
			DrumLane lane(0);
			for (auto v : l["velocity"])
			{
				lane.velocity.push_back(v);
			}
			pattern.lanes.push_back(lane);
		}
		m_patterns.push_back(pattern);
		update_events(pattern_count);
		++pattern_count;
	}
}

void DrumData::update_events(int pattern)
{
	m_patterns[pattern].m_events.clear();
	double beat_from_division = 1. / m_beat_divisions;
	double swing_adjust1 = (0.5 - m_swing) * 2. * beat_from_division;
	double swing_adjust2 = -swing_adjust1;
	auto& lanes = m_patterns[pattern].lanes;
	for (int division = 0; division < total_divisions(); ++division)
	{
		for (int lane = 0; lane < lanes.size(); ++lane)
		{
			if (lanes[lane].velocity[division] > 0)
			{
				DrumEvent e;
				e.beat_time = beat_from_division * division;
				if (division % 4 == 1)
				{
					e.beat_time += swing_adjust2;
				}
				if (division % 4 == 3)
				{
					e.beat_time += swing_adjust1;
				}
				e.lane = lane;
				e.velocity = lanes[lane].velocity[division];
				m_patterns[pattern].m_events.push_back(e);
			}
		}
	}
}

std::ostream& operator<<(std::ostream& out, const DrumData& data)
{
	auto& lanes = data.m_patterns[0].lanes;

	out << data.m_beats << " " << data.m_beat_divisions << "\n";
	out << lanes.size() << "\n";
	for (int i = 0; i < lanes.size(); ++i)
	{
		out << data.m_kit.drums[i].name << "\n";
		out << data.m_kit.drums[i].note << "\n";
		for (auto v : lanes[i].velocity)
		{
			out << v << " ";
		}
		out << "\n";
	}
	return out;
}

std::istream& operator>>(std::istream& in, DrumData& data)
{
	std::ofstream debug("c:/Temp/load_drum.txt");
	in >> data.m_beats >> data.m_beat_divisions;
	int lane_count;
	in >> lane_count;
	auto& lanes = data.m_patterns[0].lanes;
	lanes.clear();
	data.m_kit.drums.clear();
	for (int i = 0; i < lane_count; ++i)
	{
		std::string name;
		int note;
		in >> std::ws;
		std::getline(in, name);
		in >> note;
		data.add_drum(name, note);
		debug << "name " << name << " note" << note << "\n";
		for (auto& v : lanes.back().velocity)
		{
			in >> v;
		}
	}
	data.update_events(0);
	return in;
}
