

#include "DrumData.h"


void DrumData::add_drum(std::string name, int note)
{
	m_kit.drums.emplace_back(name, note);
	m_lanes.emplace_back(m_beats * m_beat_divisions);
}

void DrumData::set_swing(float swing)
{
	m_swing = swing;
	update_events();
}

void DrumData::set_hit(int lane, int division, int velocity)
{
	m_lanes[lane].velocity[division] = velocity;
	m_listener.changed();
	update_events();
}

int DrumData::get_hit(int lane, int division) const
{
	return m_lanes[lane].velocity[division];
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
	for (auto& lane : m_lanes)
	{
		for (auto& v : lane.velocity)
		{
			v = 0;
		}
	}
	update_events();
}

void DrumData::get_events(double start_time, double end_time, int num_samples, juce::MidiBuffer& midiMessages)
{
	for (auto& e : m_events)
	{
		if (e.beat_time >= start_time && e.beat_time < end_time)
		{
			double time_fraction = (e.beat_time - start_time) / (end_time - start_time);
			auto sample_time = static_cast<int>(time_fraction * num_samples);
			midiMessages.addEvent(juce::MidiMessage::noteOn(1, m_kit.drums[e.lane].note, juce::uint8(e.velocity)), sample_time);
		}
	}
}

void DrumData::update_events()
{
	m_events.clear();
	double beat_from_division = 1. / m_beat_divisions;
	double swing_adjust1 = (0.5 - m_swing) * 2. * beat_from_division;
	double swing_adjust2 = -swing_adjust1;
	for (int division = 0; division < total_divisions(); ++division)
	{
		for (int lane = 0; lane < m_lanes.size(); ++lane)
		{
			if (m_lanes[lane].velocity[division] > 0)
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
				e.velocity = m_lanes[lane].velocity[division];
				m_events.push_back(e);
			}
		}
	}
}

std::ostream& operator<<(std::ostream& out, const DrumData& data)
{
	out << data.m_beats << " " << data.m_beat_divisions << "\n";
	out << data.m_lanes.size() << "\n";
	for (int i = 0; i < data.m_lanes.size(); ++i)
	{
		out << data.m_kit.drums[i].name << "\n";
		out << data.m_kit.drums[i].note << "\n";
		for (auto v : data.m_lanes[i].velocity)
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
	data.m_lanes.clear();
	for (int i = 0; i < lane_count; ++i)
	{
		std::string name;
		int note;
		in >> std::ws;
		std::getline(in, name);
		in >> note;
		data.add_drum(name, note);
		debug << "name " << name << " note" << note << "\n";
		for (auto& v : data.m_lanes.back().velocity)
		{
			in >> v;
		}
	}
	data.update_events();
	return in;
}
