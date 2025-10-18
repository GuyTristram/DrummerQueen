/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <fstream>

//==============================================================================
DrummerQueenAudioProcessor::DrummerQueenAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
    m_data(*this)
{
    addParameter(m_swing = new juce::AudioParameterFloat("swing", // parameterID
        "Swing", // parameter name
        0.0f, // minimum value
        1.0f, // maximum value
        0.5f)); // default value

    m_data.add_drum("Kick", 36);
    m_data.add_drum("Side Stick", 37);
    m_data.add_drum("Snare", 38);
    m_data.add_drum("HH Closed", 42);
    m_data.add_drum("HH Half", 44);
    m_data.add_drum("HH Open", 46);
    m_data.add_drum("HH Pedal", 65);
    m_data.add_drum("Ride", 51);
    m_data.add_drum("Bell", 53);
    m_data.add_drum("Crash", 49);
}

DrummerQueenAudioProcessor::~DrummerQueenAudioProcessor()
{
}

//==============================================================================
const juce::String DrummerQueenAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DrummerQueenAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DrummerQueenAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DrummerQueenAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DrummerQueenAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DrummerQueenAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DrummerQueenAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DrummerQueenAudioProcessor::setCurrentProgram (int)
{
}

const juce::String DrummerQueenAudioProcessor::getProgramName (int)
{
    return {};
}

void DrummerQueenAudioProcessor::changeProgramName (int, const juce::String&)
{
}

//==============================================================================
void DrummerQueenAudioProcessor::prepareToPlay (double, int)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void DrummerQueenAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DrummerQueenAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void DrummerQueenAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    auto num_samples = buffer.getNumSamples();
    auto buffer_length_seconds = num_samples / getSampleRate();

	if (m_play_note != -1)
	{
		midiMessages.addEvent(juce::MidiMessage::noteOn(1, m_play_note, juce::uint8(127)), 0);
		m_play_note = -1;
	}

    auto head = getPlayHead();
    if (head)
    {
        auto pos = head->getPosition();
        if (pos && pos->getIsPlaying())
        {
            auto beat_pos_begin = pos->getPpqPosition();
            if (beat_pos_begin)
            {
                //m_bar_pos_beats = fmod(*beat_pos_begin, static_cast<double>(m_data.beats()));
                m_bar_pos_beats = *beat_pos_begin;
                auto bpm = pos->getBpm();
                if (bpm)
                {
                    auto beat_length_seconds = 60. / *bpm;
					auto buffer_length_beats = buffer_length_seconds / beat_length_seconds;
					m_data.get_events(*beat_pos_begin, *beat_pos_begin + buffer_length_beats, num_samples, midiMessages);
                }
                sendChangeMessage();
            }
        }
        else
		{
			m_bar_pos_beats = 0.;
            sendChangeMessage();
        }
    }
}

//==============================================================================
bool DrummerQueenAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DrummerQueenAudioProcessor::createEditor()
{
    return new DrummerQueenAudioProcessorEditor (*this);
}

//==============================================================================
void DrummerQueenAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
	auto str = m_data.to_json();
    std::ofstream debug("c:/Temp/getStateInformation.txt");
    debug << str;
    destData.append(str.c_str(), str.size());
}

void DrummerQueenAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::ofstream debug("c:/Temp/setStateInformation.txt");
    debug << "Size: " << sizeInBytes << "\n";
    if (sizeInBytes == 0)
		return;

    std::string state(static_cast<const char*>(data), sizeInBytes);
    debug << state;
    if (state[0] == '{')
	{
		m_data.from_json(state);
	}
    else
    {
        std::istringstream in(state);
        in >> m_data;
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DrummerQueenAudioProcessor();
}
