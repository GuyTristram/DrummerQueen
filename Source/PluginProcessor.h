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

	void changed() override { }

	void play_note(int note) { m_play_note = note; }

    DrumData m_data;

private:
    double m_bar_pos_beats = -1.;
    juce::AudioParameterFloat* m_swing;
	int m_play_note = -1;
	static constexpr int MAX_MIDI_MESSAGES = 32;
	std::array <DrumEvent, MAX_MIDI_MESSAGES> m_midi_messages;
	int m_midi_message_count = 0;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrummerQueenAudioProcessor)
};
