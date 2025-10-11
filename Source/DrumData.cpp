#include "DrumData.h"

#include "json.hpp"

namespace
{
	std::vector<int> parse_seq_impl(const char*& c) {
		std::vector<int> result;
		int repeat = 1;
		while (*c) {
			if (isdigit(*c)) {
				repeat = 0;
				while (*c && isdigit(*c)) {
					repeat = repeat * 10 + (*c - '0');
					++c;
				}
			}
			else if (isalpha(*c)) {
				auto n = tolower(*c) - 'a';
				for (int i = 0; i < repeat; ++i) {
					result.push_back(n);
				}
				repeat = 1;
				++c;
			}
			else if (*c == '(') {
				++c;
				auto nested = parse_seq_impl(c);
				for (int i = 0; i < repeat; ++i) {
					result.insert(result.end(), nested.begin(), nested.end());
				}
				repeat = 1;
			}
			else if (*c == ')') {
				++c;
				return result;
			}
			else {
				++c;
			}
		}
		return result;
	}

	std::vector<int> parse_seq(const char* c) {
		return parse_seq_impl(c);
	}

	bool validate(const char* c) {
		int paren_level = 0;
		while (*c) {
			if (*c == '(') {
				++paren_level;
			}
			else if (*c == ')') {
				--paren_level;
				if (paren_level < 0) {
					return false;
				}
			}
			++c;
		}
		return paren_level == 0;
	}

	double count_seq_impl(const char*& c) {
		double result = 0.;
		int repeat = 1;
		while (*c) {
			if (isdigit(*c)) {
				repeat = 0;
				while (*c && isdigit(*c)) {
					repeat = repeat * 10 + (*c - '0');
					++c;
				}
			}
			else if (isalpha(*c)) {
				result += repeat;
				repeat = 1;
				++c;
			}
			else if (*c == '(') {
				++c;
				auto nested = count_seq_impl(c);
				result += repeat * nested;
				repeat = 1;
			}
			else if (*c == ')') {
				++c;
				return result;
			}
			else {
				++c;
			}
		}
		return result;
	}
	double count_seq(const char* c) {
		return count_seq_impl(c);
	}
}

void DrumData::add_drum(std::string name, int note)
{
	do_action(
		[this, note]
		{
			m_patterns[m_current_pattern].lanes.emplace_back(m_beats * m_beat_divisions);
			m_patterns[m_current_pattern].lanes.back().note = note;
		},
		[this, pattern = m_current_pattern]
		{
			m_patterns[pattern].lanes.pop_back();
		});
}

int DrumData::add_pattern()
{
	//m_patterns.emplace_back((int)m_kit.drums.size(), m_beats * m_beat_divisions);
	m_patterns.emplace_back(m_patterns[m_current_pattern]);
	m_listener.changed();
	return (int)m_patterns.size() - 1;
}

void DrumData::set_current_pattern(int pattern) 
{
	if (pattern < 0 || pattern >= m_patterns.size())
	{
		return;
	}
	m_current_pattern = pattern;
}

void DrumData::set_pattern(int pattern_index, DrumPattern const& pattern)
{
	if (pattern_index < 0 || pattern_index >= m_patterns.size()) {
		return;
	}

	do_action(
		[this, pattern_index, pattern]
		{
			m_patterns[pattern_index] = pattern;
			update_events();
		},
		[this, pattern_index, old_pattern = m_patterns[pattern_index]]
		{
			m_patterns[pattern_index] = old_pattern;
			update_events();
		});
}

void DrumData::set_lane_note(int lane, int note)
{
	if (lane < 0 || lane >= lane_count())
	{
		return;
	}

	int pattern = m_current_pattern;
	int old_note = m_patterns[pattern].lanes[lane].note;
	do_action(
		[this, pattern, lane, note]
		{
			m_patterns[pattern].lanes[lane].note = note;
			update_events();
		},
		[this, pattern, lane, old_note]
		{
			m_patterns[pattern].lanes[lane].note = old_note;
			update_events();
		});
}

void DrumData::set_sequence_str(std::string const& sequence)
{
	m_sequence_str.clear();
	m_sequence_length = 0;
	m_sequence.clear();
	for (auto c : sequence)
	{
		if (isalpha(c))
		{
			c = char(toupper(c));
			if (c - 'A' < m_patterns.size())
			{
				m_sequence_str.push_back(c);
			}
		}
		else if (isdigit(c) || c == '(' || c == ')')
		{
			m_sequence_str.push_back(c);
		}
	}
	const char* c = m_sequence_str.c_str();
	if (validate(c))
	{
		auto len = count_seq(c);
		if (len < 10000)
		{
			m_sequence_length = len;
			c = m_sequence_str.c_str();
			m_sequence = parse_seq(c);
		}
	}
}

void DrumData::play_sequence(bool ps)
{
	m_play_sequence = ps;
}

void DrumData::set_time_signature(int beats, int beat_divisions)
{
	if (beats <= 0 || beat_divisions <= 0)
	{
		return;
	}
	int old_beats = m_beats;
	int old_beat_divisions = m_beat_divisions;
	do_action(
		[this, beats, beat_divisions]
		{
			m_beats = beats;
			m_beat_divisions = beat_divisions;
			for (auto& pattern : m_patterns)
			{
				for (auto& lane : pattern.lanes)
				{
					lane.velocity.resize(m_beats * m_beat_divisions);
				}
			}
			update_events();
		},
		[this, old_beats, old_beat_divisions, patterns = m_patterns]
		{
			m_beats = old_beats;
			m_beat_divisions = old_beat_divisions;
			m_patterns = patterns;
			update_events();
		});
}

void DrumData::set_swing(float swing)
{
	m_swing = swing;
	update_events();
}

int DrumData::lane_count() const
{
	return (int)m_patterns[m_current_pattern].lanes.size();
}

void DrumData::update_events()
{
	for (int i = 0; i < m_patterns.size(); ++i)
	{
		update_events(i);
	}
}

void DrumData::set_hit(int lane, int division, int velocity)
{
	int old_velocity = m_patterns[m_current_pattern].lanes[lane].velocity[division];
	do_action(
		[this, lane, division, velocity, pattern = m_current_pattern]
		{
			m_patterns[pattern].lanes[lane].velocity[division] = velocity;
			update_events(pattern);
		},
		[this, lane, division, old_velocity, pattern = m_current_pattern]
		{
			m_patterns[pattern].lanes[lane].velocity[division] = old_velocity;
			update_events(pattern);
		});
}

int DrumData::get_hit(int lane, int division) const
{
	return m_patterns[m_current_pattern].lanes[lane].velocity[division];
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


std::string DrumData::to_json() const
{
	using json = nlohmann::json;
	json j;
	j["version"] = 2;
	j["beats"] = m_beats;
	j["beat_divisions"] = m_beat_divisions;
	j["midi_file_directory"] = m_midi_file_directory;
	j["swing"] = m_swing;
	j["patterns"] = json::array();
	for (auto& pattern : m_patterns)
	{
		json p;
		p["lanes"] = json::array();
		for (auto& lane : pattern.lanes)
		{
			json l;
			l["note"] = lane.note;
			l["velocity"] = lane.velocity;
			p["lanes"].push_back(l);
		}
		j["patterns"].push_back(p);
	}
	j["sequence"] = m_sequence_str;
	return j.dump(3);
}

void DrumData::from_json(std::string const& json_string)
{
	using json = nlohmann::json;
	auto j = json::parse(json_string);
	int version = j.value("version", 0);
	m_beats = j["beats"];
	m_beat_divisions = j["beat_divisions"];
	m_midi_file_directory = j.value("midi_file_directory", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getFullPathName().toStdString());
	m_swing = j["swing"];
	DrumKit kit;
	if (version == 0)
	{
		kit.name = j["kit"]["name"];
		kit.drums.clear();
		for (auto& d : j["kit"]["drums"])
		{
			kit.drums.emplace_back(d["note"], d["name"]);
		}
	}
	m_patterns.clear();
	int pattern_count = 0;
	for (auto& p : j["patterns"])
	{
		DrumPattern pattern;
		for (auto& l : p["lanes"])
		{
			DrumLane lane(0);
			if (version == 0)
			{
				lane.note = kit.drums[pattern.lanes.size()].note;
			}
			else
			{
				lane.note = l["note"];
			}

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
	if (j.find("sequence") != j.end())
	{
		set_sequence_str(j["sequence"]);
	}
	else
	{
		set_sequence_str("");
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
				e.note = lanes[lane].note;
				e.velocity = lanes[lane].velocity[division];
				m_patterns[pattern].m_events.push_back(e);
				e.velocity = 0;
				e.beat_time += .9 / m_beat_divisions;
				m_patterns[pattern].m_events.push_back(e);
			}
		}
	}
}

void DrumData::do_action(std::function<void()> do_action, std::function<void()> undo_action)
{
	m_undo_stack.push_back({ do_action, undo_action });
	do_action();
	m_redo_stack.clear();
}

void DrumData::undo()
{
	if (m_undo_stack.empty())
	{
		return;
	}
	auto action = m_undo_stack.back();
	m_undo_stack.pop_back();
	action.undo_action();
	m_redo_stack.push_back(action);
}

void DrumData::redo()
{
	if (m_redo_stack.empty())
	{
		return;
	}
	auto action = m_redo_stack.back();
	m_redo_stack.pop_back();
	action.do_action();
	m_undo_stack.push_back(action);
}

std::ostream& operator<<(std::ostream& out, const DrumData& data)
{
	auto& lanes = data.m_patterns[0].lanes;

	out << data.m_beats << " " << data.m_beat_divisions << "\n";
	out << lanes.size() << "\n";
	for (int i = 0; i < lanes.size(); ++i)
	{
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
