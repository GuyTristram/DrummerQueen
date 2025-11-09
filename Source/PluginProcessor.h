#pragma once

#include <JuceHeader.h>
#include "DrumData.h"

//==============================================================================
/**
*/
class DrummerQueenAudioProcessor : public juce::AudioProcessor, public juce::ChangeBroadcaster, public DrumDataListener
{
public:
    //==============================================================================
    DrummerQueenAudioProcessor();
    ~DrummerQueenAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    double barPos() const { return m_bar_pos_beats; }
	double bpm() const { return m_bpm; }

	void changed() override { }

	void play_note(int note) { m_play_note = note; }
    std::vector<DrumEvent> get_recorded_midi() {
        std::vector<DrumEvent> midi_messages;
		midi_messages.reserve(MAX_MIDI_MESSAGES);
		midi_messages.swap(m_midi_messages);
		return midi_messages;
	}

    DrumData m_data;

private:
    double m_bar_pos_beats = -1.;
	double m_bpm = 120.;
    juce::AudioParameterFloat* m_swing;
	int m_play_note = -1;
	static constexpr int MAX_MIDI_MESSAGES = 32;
	std::vector<DrumEvent> m_midi_messages;
    bool m_recording = false;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrummerQueenAudioProcessor)
};
