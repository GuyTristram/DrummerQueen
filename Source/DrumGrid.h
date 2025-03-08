/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include <vector>
#include <memory>

//==============================================================================
/**
*/
class DrumData;

class VelocityButton : public juce::ToggleButton
{
public:
	VelocityButton(int velocity) : m_velocity(velocity) {}
    void paintButton(juce::Graphics& g,
        bool	shouldDrawButtonAsHighlighted,
        bool	shouldDrawButtonAsDown) override;
	int m_velocity;
};

class DrumGrid : public juce::Component
{
public:
    DrumGrid(DrumData &data);

    //==============================================================================
    void paint (juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void setPosition(double p) { m_position = p; }

	void set_new_velocity(int v) { m_new_velocity = v; }

private:
    DrumData& m_data;

    int m_lane_height = 24;
    int m_note_width = 24;

    double m_position = 0.;

	int m_new_velocity = 127;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumGrid)
};
