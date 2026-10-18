// Apprehension by Reyaan. - VST3 processor
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace apprehension;

ApprehensionProcessor::ApprehensionProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "STATE", createLayout()) {
    for (int i = 0; i < P_COUNT; ++i) raw[i] = apvts.getRawParameterValue(kIds[i]);
}

bool ApprehensionProcessor::isBusesLayoutSupported(const BusesLayout& l) const {
    return l.getMainInputChannelSet() == juce::AudioChannelSet::stereo() &&
           l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void ApprehensionProcessor::prepareToPlay(double sampleRate, int) {
    engine.prepare(sampleRate);
    keys.prepare(sampleRate);
    setLatencySamples(engine.latencySamples());   // fixed 25 ms from the overall pitch shifter; the host compensates
}

void ApprehensionProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    if (buffer.getNumChannels() < 2) return;

    double bpm = 0.0, ppq = -1.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition()) {
            if (auto b = pos->getBpm()) bpm = *b;
            if (pos->getIsPlaying())
                if (auto q = pos->getPpqPosition()) ppq = *q;
        }

    double v[P_COUNT];
    for (int i = 0; i < P_COUNT; ++i) v[i] = (double)raw[i]->load();
    const Params p = toEngineParams(v);

    float* l = buffer.getWritePointer(0);
    float* r = buffer.getWritePointer(1);
    const int n = buffer.getNumSamples();
    for (int i = 0; i < n; ++i) keys.push(0.5f * (l[i] + r[i]));   // key detector listens to the dry input

    const float peak = engine.process(l, r, l, r, n, p, bpm, ppq);
    outLevel.store(std::max(peak, outLevel.load() * 0.85f));
}

juce::AudioProcessorEditor* ApprehensionProcessor::createEditor() { return new ApprehensionEditor(*this); }

// ------------------------------------------------------------------ custom chop pattern
void ApprehensionProcessor::setChopCell(int i, char c) {
    engine.setCustom(i, c);
    saveChopToState();
}

void ApprehensionProcessor::setChopMode(int index) {
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(kIds[P_CHOPPATTERN]))) *p = index;
}

void ApprehensionProcessor::saveChopToState() {
    juce::String s;
    for (int i = 0; i < 16; ++i) s += juce::String::charToString((juce::juce_wchar)engine.custom(i));
    apvts.state.setProperty("chop", s, nullptr);
}

void ApprehensionProcessor::loadChopFromState() {
    const juce::String s = apvts.state.getProperty("chop", "").toString();
    if (s.length() != 16) return;
    for (int i = 0; i < 16; ++i) {
        const char c = (char)s[i];
        if (c == 'N' || c == 'G' || c == 'S' || c == 'H' || c == 'T' || c == 'R') engine.setCustom(i, c);
    }
}

// ------------------------------------------------------------------ presets
void ApprehensionProcessor::applyPreset(int index) {
    const int n = (int)presets().size();
    if (index >= n) index = -1;
    for (int i = 0; i < P_COUNT; ++i) {
        if (i == P_REFHZ) continue;   // the reference frequency is the user's own, not part of a sound
        if (auto* prm = apvts.getParameter(kIds[i]))
            prm->setValueNotifyingHost(prm->convertTo0to1((float)presetValue(index, i)));
    }
    apvts.state.setProperty("preset", index, nullptr);
}

int ApprehensionProcessor::currentPreset() const {
    const int i = (int)apvts.state.getProperty("preset", -1);
    return (i >= 0 && i < (int)presets().size()) ? i : -1;
}

bool ApprehensionProcessor::presetEdited() const {
    const int idx = currentPreset();
    for (int i = 0; i < P_COUNT; ++i) {
        if (i == P_REFHZ) continue;
        const ParamSpec& s = kSpec[i];
        const double tol = std::max(s.kind == K_FLOAT ? s.step * 0.6 : 0.4, 1e-4);
        if (std::fabs((double)raw[i]->load() - presetValue(idx, i)) > tol) return true;
    }
    return false;
}

// ------------------------------------------------------------------ state
void ApprehensionProcessor::getStateInformation(juce::MemoryBlock& dest) {
    saveChopToState();
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, dest);
}

void ApprehensionProcessor::setStateInformation(const void* data, int size) {
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(apvts.state.getType())) {
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
            loadChopFromState();
        }
}

// ------------------------------------------------------------------ parameters (built from the shared table in params.h)
juce::AudioProcessorValueTreeState::ParameterLayout ApprehensionProcessor::createLayout() {
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    using Fmt = std::function<String(float, int)>;

    auto formatter = [](int fmt) -> Fmt {
        switch (fmt) {
            case FMT_PCT:     return [](float v, int) { return String(v, 0) + " %"; };
            case FMT_MS:      return [](float v, int) { return String(v, 0) + " ms"; };
            case FMT_SEC:     return [](float v, int) { return String(v, 1) + " s"; };
            case FMT_HZ2:     return [](float v, int) { return String(v, 2) + " Hz"; };
            case FMT_DB:      return [](float v, int) { return String(v, 1) + " dB"; };
            case FMT_ST:      return [](float v, int) { return String(v >= 0.f ? "+" : "") + String(v, 2) + " st"; };
            case FMT_WHOLE:   return [](float v, int) { return String((int)std::lround(v)); };
            case FMT_LOWCUT:  return [](float v, int) { return v <= 20.5f ? String("Off") : String(v, 0) + " Hz"; };
            case FMT_HIGHCUT: return [](float v, int) { return v >= 19999.5f ? String("Off") : String(v, 0) + " Hz"; };
            default:          return [](float v, int) { return String(v, 2); };
        }
    };

    for (int i = 0; i < P_COUNT; ++i) {
        const ParamSpec& s = kSpec[i];
        const ParameterID pid{kIds[i], 1};
        if (s.kind == K_BOOL) {
            layout.add(std::make_unique<AudioParameterBool>(pid, s.name, s.def >= 0.5));
        } else if (s.kind == K_CHOICE) {
            StringArray names;
            for (int k = 0; k < s.nChoices; ++k) names.add(s.choices[k]);
            layout.add(std::make_unique<AudioParameterChoice>(pid, s.name, names, (int)s.def));
        } else {
            layout.add(std::make_unique<AudioParameterFloat>(
                pid, s.name, NormalisableRange<float>((float)s.mn, (float)s.mx, (float)s.step, (float)s.skew), (float)s.def,
                AudioParameterFloatAttributes().withStringFromValueFunction(formatter(s.fmt))));
        }
    }
    return layout;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ApprehensionProcessor(); }
