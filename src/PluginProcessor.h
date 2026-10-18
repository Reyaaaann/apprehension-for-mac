// Apprehension by Reyaan. - VST3 processor
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "params.h"
#include "presets.h"
#include "keydetect.h"

class ApprehensionProcessor final : public juce::AudioProcessor {
public:
    ApprehensionProcessor();

    const juce::String getName() const override { return "Apprehension"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 15.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    bool isBusesLayoutSupported(const BusesLayout& l) const override;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;

    void getStateInformation(juce::MemoryBlock& dest) override;
    void setStateInformation(const void* data, int size) override;

    float get(Pid p) const { return raw[p]->load(); }
    int chopStep() const { return engine.uiStep.load(); }
    int chopPattern() const { return engine.uiPattern.load(); }
    char chopAppliedAction() const { return (char)engine.uiAct.load(); }

    // custom chop pattern (16 steps, one of N G S H T R each); saved with the project
    char chopCell(int i) const { return engine.custom(i); }
    void setChopCell(int i, char c);
    void setChopMode(int index);

    // presets: index -1 = Init (all defaults). Reference Hz is left alone.
    void applyPreset(int index);
    int currentPreset() const;          // -1 = Init
    bool presetEdited() const;          // true once any knob differs from the loaded preset

    juce::AudioProcessorValueTreeState apvts;
    apprehension::KeyDetector keys;
    std::atomic<float> outLevel{0.f};   // recent output peak, for the UI

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void saveChopToState();
    void loadChopFromState();

    std::atomic<float>* raw[P_COUNT] = {};
    apprehension::Engine engine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ApprehensionProcessor)
};
