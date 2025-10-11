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
    g.drawVerticalLine(x_time, 0, m_lane_height * m_data.lane_count());

    // Draw grid
    for (int i = 0; i < total_divisions + 1; ++i)
    {
        if (i % beat_divisions == 0)
            g.setColour({ 255, 255, 255 });
        else
            g.setColour({ 0, 0, 0 });
        g.drawVerticalLine(i * m_note_width, 0, m_lane_height * m_data.lane_count());
    }
    for (int i = 0; i < m_data.lane_count() + 1; ++i)
    {
        g.setColour({ 255, 255, 255 });
        g.drawHorizontalLine(i * m_lane_height, 0, m_note_width * total_divisions);
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
				draw_note(g, lane, sub_beat, v);
            }
            x += m_note_width;
        }
        x = 0;
        y += m_lane_height;
    }

}

void DrumGrid::draw_note(juce::Graphics& g, int lane, int sub_beat, int velocity)
{
	int x = sub_beat * m_note_width;
	int y = lane * m_lane_height;
	int max_size = std::min(m_note_width, m_lane_height) - 8;
	int min_size = max_size / 7;
	int size = velocity / 127.0 * (max_size - min_size) + min_size;
	int border = (std::min(m_note_width, m_lane_height) - size) / 2;
	//g.fillRoundedRectangle(border, border, getWidth() - border * 2, getHeight() - border * 2, size / 2);
    if (m_style == 0) {
        g.fillEllipse(x + border, y + border, m_note_width - border * 2, m_lane_height - border * 2);
    }
    else {
        border -= 2;
        juce::Path p;
        p.startNewSubPath(x + 2, y + border);
        p.lineTo(x + m_note_width - border * 2, y + m_lane_height / 2);
        p.lineTo(x + 2, y + m_lane_height - border);
        g.fillPath(p);
    }
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

void VelocityButton::paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
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

	int max_size = std::min(getWidth(), getHeight()) - 8;

	int min_size = max_size / 7;

	int size = int(m_velocity / 127.0 * (max_size - min_size) + min_size);
	int border = (getWidth() - size) / 2;
    //g.fillRoundedRectangle(border, border, getWidth() - border * 2, getHeight() - border * 2, size / 2);
	g.fillEllipse(border, border, getWidth() - border * 2, getHeight() - border * 2);

}

