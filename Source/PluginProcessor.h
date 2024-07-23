/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

enum Slope
{
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

// extract parameters form the audio processor vaule tree state 
// a data structure representing all parameter vaules
struct ChainSettings {
    float peakFreq{ 0 }, peakGainInDecibles{ 0 }, peakQuality{ 1.f };
    float lowCutFreq{ 0 }, highCutFreq{ 0 };
    //int lowCutSlope{ 0 }, highCutSlope{ 0 };
    Slope lowCutSlope{ Slope::Slope_12 }, highCutSlope{ Slope::Slope_12 };
};
// getter function for ChainSettings
ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);


//==============================================================================
/**
*/
class SimpleEQAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    SimpleEQAudioProcessor();
    ~SimpleEQAudioProcessor() override;

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

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts {*this, nullptr, "Parameters", createParameterLayout()};

private:
    // create type aliases for dsp namespaces to help ourselves out
    // this is a peak filter alias
    using Filter = juce::dsp::IIR::Filter<float>;

    // to make low/high cut filters we need to stack the filters if we want to multiply the db/octave value
    // a low/high pass filter has a 12 db slope
    // pass in 4 filters in a procesor chain, pass it as a single context, and process the audio at once
    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

    // define a chain representing the whole mono signal path
    // contains lowCutFilter, peak filter, highCutFilter
    using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;

    // we need 2 instances of the MonoChain for stereo processing
    MonoChain leftChain, rightChain;

    // our custom enum to describe chain positions
    // without using enum, we can use leftChain.get<int Index>() to get links in the processor chain
    enum ChainPositions
    {
        LowCut,
        Peak,
        HighCut
    };
    void updatePeakFilter(const ChainSettings& chainSettings);
    using Coefficients = Filter::CoefficientsPtr;
    static void updateCoefficients(Coefficients& old, const Coefficients& replacements);


    template<typename ChainType, typename CoefficientType>
    void updateCutFilter(ChainType& leftLowCut, const CoefficientType& cutCoefficients, const Slope& lowCutSlope)
    {
        //----------------------low cut and high cut coefficients -----------------------
        // possible orders: slope choice 0,1,2,3 -> 12, 24, 36, 48 db/oct -> order: 2, 4, 6, 8
        // calculate order from slope choice: (slope choice + 1) * 2

        /*auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq, sampleRate, 2 * (chainSettings.lowCutSlope + 1));


        auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();*/

        leftLowCut.template setBypassed<0>(true);
        leftLowCut.setBypassed<1>(true);
        leftLowCut.setBypassed<2>(true);
        leftLowCut.setBypassed<3>(true);

        switch (lowCutSlope) {
        case Slope_12: {
            *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<0>(false);
            break;
        }
        case Slope_24: {
            *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<0>(false);
            *leftLowCut.get<1>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<1>(false);
            break;
        }
        case Slope_36: {
            *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<0>(false);
            *leftLowCut.get<1>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<1>(false);
            *leftLowCut.get<2>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<2>(false);
            break;
        }
        case Slope_48: {
            *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<0>(false);
            *leftLowCut.get<1>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<1>(false);
            *leftLowCut.get<2>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<2>(false);
            *leftLowCut.get<3>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<3>(false);
            break;
        }
        }
    }
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessor)
};
