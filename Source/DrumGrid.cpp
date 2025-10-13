/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "DrumGrid.h"
#include "DrumData.h"

DrumGrid::DrumGrid(DrumData& data): m_data(data)
{
}

void DrumGrid::paint(juce::Graphics& g)
{
    int total_divisions = m_data.total_divisions();
	int beat_divisions = m_data.beat_divisions();
    // Fill background
    g.setColour({ 92, 92, 128 });
    g.fillRect(0, 0, total_divisions * m_note_width, m_data.lane_count() * m_lane_height);

    // Draw time
    int x_time = int(m_note_width * m_data.beat_divisions() * m_position);
    g.setColour({ 200, 200, 0 });
    g.drawVerticalLine(x_time, 0.f, float(m_lane_height * m_data.lane_count()));

    // Draw grid
    for (int i = 0; i < total_divisions + 1; ++i)
    {
        if (i % beat_divisions == 0)
            g.setColour({ 255, 255, 255 });
        else
            g.setColour({ 0, 0, 0 });
        g.drawVerticalLine(i * m_note_width, 0.f, float(m_lane_height * m_data.lane_count()));
    }
    for (int i = 0; i < m_data.lane_count() + 1; ++i)
    {
        g.setColour({ 255, 255, 255 });
        g.drawHorizontalLine(i * m_lane_height, 0.f, float(m_note_width * total_divisions));
    }

    // Draw notes
    g.setColour({ 255, 255, 255 });
    int x = 0, y = 0;
    for (int lane = 0; lane <  m_data.lane_count(); ++lane)
    {
        for (int sub_beat = 0; sub_beat < m_data.total_divisions(); ++sub_beat)
        {
			int v = m_data.get_hit(lane, sub_beat);
            if (v > 0)
            {
                if (sub_beat < m_position * m_data.beat_divisions() && sub_beat + 1 > m_position * m_data.beat_divisions())
                {
                    g.setColour(juce::Colours::white);
                }
                else
                {
                    g.setColour(juce::Colour(0xffa0a0a0));
                }
                draw_note(g, lane, sub_beat, v);
            }
            x += m_note_width;
        }
        x = 0;
        y += m_lane_height;
    }

}

void draw_note_in_style(juce::Graphics& g, int style, int velocity, float x, float y, float size)
{
    float max_size = size - 8.f;
    float min_size = max_size / 7.f;
    float size_inside = float(velocity) / 127.f * (max_size - min_size) + min_size;
    float border = (size - size_inside) / 2.f;
    if (style == 0) {
        g.fillEllipse(x + border, y + border, size - border * 2, size - border * 2);
    }
    else {
        border -= 2;
        juce::Path p;
        p.startNewSubPath(x + 2, y + border);
        p.lineTo(x + size - border * 2, y + size / 2);
        p.lineTo(x + 2, y + size - border);
        g.fillPath(p);
    }
}

void DrumGrid::draw_note(juce::Graphics& g, int lane, int sub_beat, int velocity)
{
	float x = float(sub_beat * m_note_width);
    float y = float(lane * m_lane_height);
    draw_note_in_style(g, m_style, velocity, x, y, float(std::min(m_note_width, m_lane_height)));
}

void DrumGrid::mouseDown(const juce::MouseEvent& event)
{
    auto pos = event.getMouseDownPosition();
    const int lane = pos.getY() / m_lane_height;
    if (lane >= 0 && lane < m_data.lane_count())
    {
        int beat = pos.getX() / m_note_width;
        if (beat >= 0 && beat < m_data.total_divisions())
        {
			if (event.mods.isLeftButtonDown())
			{
                const int current_vel = m_data.get_hit(lane, beat);
                const int new_vel = current_vel != m_new_velocity ? m_new_velocity : 0;
                m_data.set_hit(lane, beat, new_vel);
            }
			else if (event.mods.isMiddleButtonDown())
			{
				m_data.set_hit(lane, beat, 0);
			}
         }
    }
    repaint();
}

void VelocityButton::paintButton(juce::Graphics& g, bool, bool )
{
    g.fillAll(juce::Colours::black);
    if (getToggleState())
    {
        g.setColour(juce::Colours::white);
    }
    else
    {
        g.setColour(juce::Colours::grey);
    }

	int max_size = std::min(getWidth(), getHeight());

	draw_note_in_style(g, 1, m_velocity, 0.f, 0.f, float(max_size));
}

