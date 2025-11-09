#include "DrumData.h"

#include "json.hpp"

namespace
{
	std::vector<SequenceItem> parse_seq_impl(const char*& c) {
		std::vector<SequenceItem> result;
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
					result.push_back({ 0.f, 0.f, n });
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

	std::vector<SequenceItem> parse_seq(const char* c, DrumData::PatternArray const &patterns) {
		auto seq = parse_seq_impl(c);
		double beat_time = 0.f;
		for (auto& item : seq) {
			if (item.pattern >= 0 && item.pattern < patterns.size()) {
				item.start_beat = beat_time;
				item.end_beat = item.start_beat + patterns[item.pattern].time_signature.beats;
				beat_time = item.end_beat;
			}
			else {
				item.start_beat = beat_time;
			}
		}
		return seq;
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

DrumData::DrumData(DrumDataListener& listener)
	: m_listener(listener),
	m_midi_file_directory(juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getFullPathName().toStdString())
{
	load_kits();
	m_current_pattern_sequence.push_back({ 0.f, 4.f, 0 });
}

void DrumData::add_drum(std::string name, int note)
{
	do_action(
		[this, note]
		{
			m_patterns[m_current_pattern].lanes.emplace_back(m_patterns[m_current_pattern].time_signature.total_divisions());
			m_patterns[m_current_pattern].lanes.back().note = note;
		},
		[this, pattern = m_current_pattern]
		{
			m_patterns[pattern].lanes.pop_back();
		});
}

void DrumData::set_current_pattern(int pattern) 
{
	if (pattern < 0 || pattern >= m_patterns.size()) {
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
		[this, pattern_index, pattern] {
			m_patterns[pattern_index] = pattern;
			update_events();
		},
		[this, pattern_index, old_pattern = m_patterns[pattern_index]]
		{ m_patterns[pattern_index] = old_pattern;
			update_events();
		});
}

void DrumData::set_lane_note(int lane, int note)
{
	if (lane < 0 || lane >= lane_count()) {
		return;
	}

	int pattern = m_current_pattern;
	int old_note = m_patterns[pattern].lanes[lane].note;
	do_action(
		[this, pattern, lane, note] {
			m_patterns[pattern].lanes[lane].note = note;
			update_events();
		},
		[this, pattern, lane, old_note] {
			m_patterns[pattern].lanes[lane].note = old_note;
			update_events();
		});
}

void DrumData::set_sequence_str(std::string const& sequence)
{
	m_sequence_str.clear();
	m_sequence_length = 0;
	m_sequence.clear();
	for (auto c : sequence) {
		if (isalpha(c)) {
			c = char(toupper(c));
			if (c - 'A' < m_patterns.size()) {
				m_sequence_str.push_back(c);
			}
		}
		else if (isdigit(c) || c == '(' || c == ')') {
			m_sequence_str.push_back(c);
		}
	}
	const char* c = m_sequence_str.c_str();
	if (validate(c)) {
		auto len = count_seq(c);
		if (len < 10000) {
			m_sequence_length = int(len);
			c = m_sequence_str.c_str();
			m_sequence = parse_seq(c, m_patterns);
		}
	}
}

void DrumData::update_sequence()
{
	m_sequence = parse_seq(m_sequence_str.c_str(), m_patterns);
}

void DrumData::play_sequence(bool ps)
{
	m_play_sequence = ps;
}

void DrumData::set_time_signature(int new_beats, int new_beat_divisions)
{
	if (new_beats <= 0 || new_beat_divisions <= 0)
	{
		return;
	}
	int old_beats = beats();
	int old_beat_divisions = beat_divisions();
	do_action(
		[this, new_beats, new_beat_divisions, pattern_id = m_current_pattern] {
			auto& pattern = m_patterns[pattern_id];
			pattern.time_signature.beats = new_beats;
			pattern.time_signature.beat_divisions = new_beat_divisions;
			for (auto& lane : pattern.lanes)
			{
				lane.velocity.resize(new_beats * new_beat_divisions);
			}
			update_sequence();
			update_events();
		},
		[this, old_beats, old_beat_divisions, old_pattern = m_patterns[m_current_pattern], pattern_id = m_current_pattern] {
			m_patterns[pattern_id] = old_pattern;
			update_sequence();
			update_events();
		});
}

void DrumData::set_hit_at_time(double beat_time, int note, int velocity)
{
	auto& pattern = m_patterns[m_current_pattern];
	int division = static_cast<int>(std::round(beat_time * pattern.time_signature.beat_divisions)) % pattern.time_signature.total_divisions();
	for (int lane = 0; lane < pattern.lanes.size(); ++lane) {
		if (pattern.lanes[lane].note == note) {
			int old_velocity = pattern.lanes[lane].velocity[division];
			do_action(
				[this, lane, division, velocity, pattern_id = m_current_pattern] {
					m_patterns[pattern_id].lanes[lane].velocity[division] = velocity;
					update_events(pattern_id);
				},
				[this, lane, division, old_velocity, pattern_id = m_current_pattern] {
					m_patterns[pattern_id].lanes[lane].velocity[division] = old_velocity;
					update_events(pattern_id);
				});
			return;
		}
	}
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

std::vector<std::string> DrumData::get_kit_names() const
{
	std::vector<std::string> kit_names;
	for (auto& kit : m_kits) {
		kit_names.push_back(kit.name);
	}
	return kit_names;
}

std::vector<DrumInfo> const &DrumData::get_current_kit_drums() const
{
	return m_kits[m_current_kit].drums;
}

std::string DrumData::get_drum_name(int note) const
{
	for (int i = 0; i < m_kits[m_current_kit].drums.size(); ++i) {
		if (m_kits[m_current_kit].drums[i].note == note) {
			return m_kits[m_current_kit].drums[i].name;
		}
	}

	for (int i = 0; i < m_kits[0].drums.size(); ++i) {
		if (m_kits[0].drums[i].note == note) {
			return m_kits[0].drums[i].name + "*";
		}
	}
	return std::string();
}

void DrumData::update_events()
{
	for (int i = 0; i < m_patterns.size(); ++i) {
		update_events(i);
	}
}

void DrumData::set_hit(int lane, int division, int velocity)
{
	int old_velocity = m_patterns[m_current_pattern].lanes[lane].velocity[division];
	do_action(
		[this, lane, division, velocity, pattern = m_current_pattern] {
			m_patterns[pattern].lanes[lane].velocity[division] = velocity;
			update_events(pattern);
		},
		[this, lane, division, old_velocity, pattern = m_current_pattern] {
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
	for (auto& lane : m_patterns[m_current_pattern].lanes) {
		for (auto& v : lane.velocity) {
			v = 0;
		}
	}
	update_events(m_current_pattern);
}

std::vector<SequenceItem> const& DrumData::get_playing_sequence() const
{
	if (m_play_sequence) {
		return m_sequence;
	}
	m_current_pattern_sequence[0].pattern = m_current_pattern;
	m_current_pattern_sequence[0].end_beat = m_patterns[m_current_pattern].time_signature.beats;
	return m_current_pattern_sequence;
}

double DrumData::get_wrapped_time(double time_beats) const
{
	auto const& sequence = get_playing_sequence();
	if (sequence.size() == 0) {
		return 0.;
	}
	double sequence_length_beats = sequence.back().end_beat;
	return fmod(time_beats, sequence_length_beats);
}

int DrumData::get_sequence_index(double time_beats) const
{
	if (!m_play_sequence) {
		return 0;
	}
	if (m_sequence.size() == 0) {
		return 0;
	}
	double time_wrap = get_wrapped_time(time_beats);
	for (int seq_index = 0; seq_index < m_sequence.size(); ++seq_index) {
		if (m_sequence[seq_index].end_beat >= time_wrap) {
			return seq_index;
		}
	}
	return 0;
}


std::string DrumData::to_json() const
{
	using json = nlohmann::json;
	json j;
	j["version"] = 3;
	j["midi_file_directory"] = m_midi_file_directory;
	j["swing"] = m_swing;
	j["patterns"] = json::array();
	j["play_sequence"] = m_play_sequence;
	j["current_pattern"] = m_current_pattern;
	j["current_kit"] = m_kits[m_current_kit].name;
	for (auto& pattern : m_patterns) {
		json p;
		p["beats"] = pattern.time_signature.beats;
		p["beat_divisions"] = pattern.time_signature.beat_divisions;
		p["lanes"] = json::array();
		for (auto& lane : pattern.lanes) {
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
	int beats = 4;
	int beat_divisions = 4;
	if (version < 3) {
		beats = j["beats"];
		beat_divisions = j["beat_divisions"];
	}
	auto user_doc_dir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getFullPathName().toStdString();
	m_midi_file_directory = j.value("midi_file_directory", user_doc_dir);
	m_swing = j["swing"];
	DrumKit kit;
	if (version < 1) {
		kit.name = j["kit"]["name"];
		kit.drums.clear(); for (auto& d : j["kit"]["drums"])
		{
			kit.drums.emplace_back(d["note"], d["name"]);
		}
	}
	auto current_kit_name = j.value("current_kit", "General MIDI");
	for (int i = 0; i < m_kits.size(); ++i) {
		if (m_kits[i].name == current_kit_name) {
			m_current_kit = i;
			break;
		}
	}
	m_play_sequence = j.value("play_sequence", false);
	m_current_pattern = j.value("current_pattern", 0);
	int pattern_count = 0;
	for (auto& p : j["patterns"]) {
		DrumPattern pattern;
		if (version < 3) {
			pattern.time_signature.beats = beats;
			pattern.time_signature.beat_divisions = beat_divisions;
		}
		else {
			pattern.time_signature.beats = p["beats"];
			pattern.time_signature.beat_divisions = p["beat_divisions"];
		}
		for (auto& l : p["lanes"]) {
			DrumLane lane(0);
			if (version < 1) {
				lane.note = kit.drums[pattern.lanes.size()].note;
			}
			else {
				lane.note = l["note"];
			}

			for (auto v : l["velocity"]) {
				lane.velocity.push_back(v);
			}
			pattern.lanes.push_back(lane);
		}
		m_patterns[pattern_count] = pattern;
		update_events(pattern_count);
		++pattern_count;
	}
	set_sequence_str(j.value("sequence", ""));
}

void DrumData::update_events(int pattern_id)
{
	auto& pattern = m_patterns[pattern_id];
	pattern.m_events.clear();
	double beat_from_division = 1. / pattern.time_signature.beat_divisions;
	double swing_adjust1 = (0.5 - m_swing) * 2. * beat_from_division;
	double swing_adjust2 = -swing_adjust1;
	auto& lanes = pattern.lanes;
	for (int division = 0; division < pattern.time_signature.total_divisions(); ++division) {
		for (int lane = 0; lane < lanes.size(); ++lane) {
			if (lanes[lane].velocity[division] > 0) {
				DrumEvent e;
				e.beat_time = beat_from_division * division;
				if (division % 4 == 1) {
					e.beat_time += swing_adjust2;
				}
				if (division % 4 == 3) {
					e.beat_time += swing_adjust1;
				}
				e.note = lanes[lane].note;
				e.velocity = lanes[lane].velocity[division];
				pattern.m_events.push_back(e);
				e.velocity = 0;
				e.beat_time += .9 / pattern.time_signature.beat_divisions;
				pattern.m_events.push_back(e);
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
	if (m_undo_stack.empty()) {
		return;
	}
	auto action = m_undo_stack.back();
	m_undo_stack.pop_back();
	action.undo_action();
	m_redo_stack.push_back(action);
}

void DrumData::redo()
{
	if (m_redo_stack.empty()) {
		return;
	}
	auto action = m_redo_stack.back();
	m_redo_stack.pop_back();
	action.do_action();
	m_undo_stack.push_back(action);
}

void DrumData::load_kits()
{
	DrumKit fallback;
	fallback.name = "General MIDI";
	fallback.drums = std::vector<DrumInfo>{ {35, "Acoustic Bass Drum"}, {36, "Bass Drum"}, {37, "Side Stick"}, {38, "Acoustic Snare"}, {39, "Hand Clap"},
	  {40, "Electric Snare"}, {41, "Low Floor Tom"}, {42, "Closed Hi Hat"}, {43, "High Floor Tom"}, {44, "Pedal Hi - Hat"}, {45, "Low Tom"},
	  {46, "Open Hi - Hat"}, {47, "Low - Mid Tom"}, {48, "Hi - Mid Tom"}, {49, "Crash Cymbal 1"}, {50, "High Tom"}, {51, "Ride Cymbal 1"},
	  {52, "Chinese Cymbal"}, {53, "Ride Bell"}, {54, "Tambourine"}, {55, "Splash Cymbal"}, {56, "Cowbell"}, {57, "Crash Cymbal 2"}, {58, "Vibraslap"},
	  {59, "Ride Cymbal 2"}, {60, "Hi Bongo"}, {61, "Low Bongo"}, {62, "Mute Hi Conga"}, {63, "Open Hi Conga"}, {64, "Low Conga"}, {65, "Hi Timbale"},
	  {66, "Low Timbale"}, {67, "Hi Agogo"}, {68, "Low Agogo"}, {69, "Cabasa"}, {70, "Maracas"}, {71, "Short Whistle"}, {72, "Long Whistle"},
	  {73, "Short Guiro"}, {74, "Long Guiro"}, {75, "Claves"}, {76, "Hi Wood Block"}, {77, "Low Wood Block"}, {78, "Mute Cuica"}, {79, "Open Cuica"},
	  {80, "Mute Triangle"}, {81, "Open Triangle"} };

	m_kits.push_back(fallback);

	juce::File appDirectory = juce::File::getSpecialLocation(juce::File::currentApplicationFile);
	appDirectory = appDirectory.getParentDirectory();
	juce::File kitFile = appDirectory.getChildFile("DrumKits.json");

	std::ifstream in(kitFile.getFullPathName().getCharPointer());
	if (!in.is_open()) {
		return;
	}
	nlohmann::json j;
	in >> j;
	for (auto& k : j) {
		DrumKit kit;
		kit.name = k["name"];
		for (auto& d : k["drums"]) {
			kit.drums.emplace_back(d["note"], d["name"]);
		}
		m_kits.push_back(kit);
	}

}
