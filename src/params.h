// Apprehension - parameter table (no JUCE dependency, so presets and tests can use it too)
#pragma once
#include "dsp.h"

enum Pid {
    P_MIX = 0, P_SYNC, P_DIVISION, P_TIME, P_ATTACK, P_CUTOFF, P_RELEASE, P_SMOOTH, P_FEEDBACK,
    P_LOWCUT, P_HIGHCUT, P_LOWSLOPE, P_LOWRES, P_HIGHSLOPE, P_HIGHRES,
    P_DIFFUSE, P_WIDTH, P_CHORUS, P_CHORUSMODE, P_FLANGUS, P_FLANGUSMODE, P_OUTPUT,
    P_PITCH, P_REFHZ, P_FREEZE, P_DUCKAMT, P_DUCKREL, P_DUCKMODE, P_DUCKDIV,
    P_RVTYPE, P_RVMIX, P_RVSIZE, P_RVDECAY, P_RVPRE, P_RVDAMP, P_RVWIDTH, P_RVMOD,
    P_SIGNATURE, P_CHOPPATTERN, P_CHOPRATE, P_CHOPINTENSITY, P_CHOPMIX, P_CHOPSEED, P_CHOPSTUTTER,
    P_CGATE, P_CSTUT, P_CHALF, P_CTAPE, P_CREV, P_SOFTCLIP, P_GPITCH, P_TIMEALIGN, P_COUNT
};

inline const char* const kIds[P_COUNT] = {
    "mix", "sync", "division", "time", "attack", "cutoff", "release", "smooth", "feedback",
    "lowcut", "highcut", "lowslope", "lowres", "highslope", "highres",
    "diffuse", "width", "chorus", "chorusmode", "flangus", "flangusmode", "output",
    "pitch", "refhz", "freeze", "duckamt", "duckrel", "duckmode", "duckdiv",
    "rvtype", "rvmix", "rvsize", "rvdecay", "rvpre", "rvdamp", "rvwidth", "rvmod",
    "signature", "chopmode", "choprate", "chopintensity", "chopmix", "chopseed", "chopstutter",
    "cgate", "cstut", "chalf", "ctape", "crev", "softclip", "gpitch", "timealign"};

enum ParamKind { K_FLOAT, K_BOOL, K_CHOICE };
enum ParamFmt { FMT_NONE, FMT_PCT, FMT_MS, FMT_SEC, FMT_HZ2, FMT_DB, FMT_ST, FMT_WHOLE, FMT_LOWCUT, FMT_HIGHCUT };

struct ParamSpec {
    const char* name;
    int kind;
    double mn, mx, def, step, skew;
    int fmt;
    const char* const* choices;
    int nChoices;
};


#define FL(name, mn, mx, def, step, skew, fmt) {name, K_FLOAT, mn, mx, def, step, skew, fmt, nullptr, 0}
#define BO(name, def) {name, K_BOOL, 0, 1, def, 1, 1, FMT_NONE, nullptr, 0}
#define CH(name, arr, n, def) {name, K_CHOICE, 0, (double)((n) - 1), def, 1, 1, FMT_NONE, arr, n}

inline const ParamSpec kSpec[P_COUNT] = {
    FL("Mix", 0, 100, 50, 0.1, 1, FMT_PCT),
    BO("Tempo Sync", 1),
    CH("Division", apprehension::kDivisionNames, apprehension::kNumDivisions, 4),
    FL("Time (free)", 10, 4000, 500, 1, 0.4, FMT_MS),
    FL("Attack", 0, 2000, 0, 1, 0.35, FMT_MS),
    FL("Cutoff", 5, 100, 100, 0.1, 1, FMT_PCT),
    FL("Release", 1, 2000, 30, 1, 0.35, FMT_MS),
    FL("Smooth", 0, 100, 50, 0.1, 1, FMT_PCT),
    FL("Feedback", 0, 95, 25, 0.1, 1, FMT_PCT),
    FL("Low Cut", 20, 2000, 20, 1, 0.4, FMT_LOWCUT),
    FL("High Cut", 70, 20000, 20000, 1, 0.3, FMT_HIGHCUT),
    CH("Low Cut Slope", apprehension::kSlopeNames, apprehension::kNumSlopes, 3),
    FL("Low Cut Resonance", 0, 12, 0, 0.1, 1, FMT_DB),
    CH("High Cut Slope", apprehension::kSlopeNames, apprehension::kNumSlopes, 3),
    FL("High Cut Resonance", 0, 12, 0, 0.1, 1, FMT_DB),
    FL("Diffusion", 0, 100, 20, 0.1, 1, FMT_PCT),
    FL("Width", 0, 200, 100, 0.1, 1, FMT_PCT),
    FL("Chorus", 0, 100, 0, 0.1, 1, FMT_PCT),
    CH("Chorus Mode", apprehension::kChorusNames, apprehension::kNumChorusModes, 0),
    FL("Flangus", 0, 100, 0, 0.1, 1, FMT_PCT),
    CH("Flangus Mode", apprehension::kFlangerNames, apprehension::kNumFlangerModes, 0),
    FL("Output", -24, 12, 0, 0.1, 1, FMT_DB),
    FL("Reverse Pitch", -24, 24, 0, 0.01, 1, FMT_ST),
    FL("Reference Hz", 20, 2000, 440, 0.01, 0.4, FMT_HZ2),
    BO("Freeze", 0),
    FL("Duck Amount", 0, 100, 0, 0.1, 1, FMT_PCT),
    FL("Duck Release", 10, 1000, 200, 1, 0.5, FMT_MS),
    BO("Duck Pump Mode", 0),
    CH("Duck Division", apprehension::kDuckNames, apprehension::kNumDuckDivs, 2),
    CH("Reverb Type", apprehension::kReverbNames, apprehension::kNumReverbTypes, 1),
    FL("Reverb Mix", 0, 100, 0, 0.1, 1, FMT_PCT),
    FL("Reverb Size", 0, 100, 50, 0.1, 1, FMT_PCT),
    FL("Reverb Decay", 0.2, 15, 2.5, 0.1, 0.5, FMT_SEC),
    FL("Reverb Pre-Delay", 0, 250, 20, 1, 0.5, FMT_MS),
    FL("Reverb Damping", 0, 100, 40, 0.1, 1, FMT_PCT),
    FL("Reverb Width", 0, 200, 100, 0.1, 1, FMT_PCT),
    FL("Reverb Modulation", 0, 100, 20, 0.1, 1, FMT_PCT),
    BO("@Reyaan. Signature Mode", 0),
    CH("Chop Pattern", apprehension::kChopPatternNames, apprehension::kNumChopPatterns, 0),
    CH("Chop Rate", apprehension::kChopRateNames, apprehension::kNumChopRates, 1),
    FL("Chop Intensity", 0, 100, 100, 0.1, 1, FMT_PCT),
    FL("Chop Mix", 0, 100, 100, 0.1, 1, FMT_PCT),
    FL("Chop Seed", 0, 99, 0, 1, 1, FMT_WHOLE),
    CH("Stutter Size", apprehension::kStutterNames, apprehension::kNumStutter, 1),
    BO("Chop: Gate", 1),
    BO("Chop: Stutter", 1),
    BO("Chop: Half-time", 1),
    BO("Chop: Tape-stop", 1),
    BO("Chop: Reverse", 1),
    BO("Soft Clip", 0),
    FL("Overall Pitch", -12, 12, 0, 0.01, 1, FMT_ST),
    BO("Time Align", 0),
};

#undef FL
#undef BO
#undef CH

inline double clampToSpec(int id, double v) {
    const ParamSpec& s = kSpec[id];
    v = std::min(std::max(v, s.mn), s.mx);
    if (s.kind != K_FLOAT) v = std::round(v);
    return v;
}

// Real-unit parameter values (as the host stores them) -> the engine's working units.
inline apprehension::Params toEngineParams(const double* v) {
    apprehension::Params p;
    p.mix = v[P_MIX] / 100.0;
    p.sync = v[P_SYNC];
    p.div = v[P_DIVISION];
    p.timeMs = v[P_TIME];
    p.attackMs = v[P_ATTACK];
    p.cutoff = v[P_CUTOFF] / 100.0;
    p.releaseMs = v[P_RELEASE];
    p.smooth = v[P_SMOOTH] / 100.0;
    p.feedback = v[P_FEEDBACK] / 100.0;
    p.lowCut = v[P_LOWCUT];
    p.highCut = v[P_HIGHCUT];
    p.lowSlope = v[P_LOWSLOPE];
    p.highSlope = v[P_HIGHSLOPE];
    p.lowRes = v[P_LOWRES];
    p.highRes = v[P_HIGHRES];
    p.diffuse = v[P_DIFFUSE] / 100.0;
    p.width = v[P_WIDTH] / 100.0;
    p.chorus = v[P_CHORUS] / 100.0;
    p.chorusMode = v[P_CHORUSMODE];
    p.flangus = v[P_FLANGUS] / 100.0;
    p.flangusMode = v[P_FLANGUSMODE];
    p.outputDb = v[P_OUTPUT];
    p.pitchSt = v[P_PITCH];
    p.gPitchSt = v[P_GPITCH];
    p.freeze = v[P_FREEZE];
    p.duckAmt = v[P_DUCKAMT] / 100.0;
    p.duckRelMs = v[P_DUCKREL];
    p.duckMode = v[P_DUCKMODE];
    p.duckDiv = v[P_DUCKDIV];
    p.rvType = v[P_RVTYPE];
    p.rvMix = v[P_RVMIX] / 100.0;
    p.rvSize = v[P_RVSIZE] / 100.0;
    p.rvDecay = v[P_RVDECAY];
    p.rvPreMs = v[P_RVPRE];
    p.rvDamp = v[P_RVDAMP] / 100.0;
    p.rvWidth = v[P_RVWIDTH] / 100.0;
    p.rvMod = v[P_RVMOD] / 100.0;
    p.signature = v[P_SIGNATURE];
    p.chopPattern = v[P_CHOPPATTERN];
    p.chopRate = v[P_CHOPRATE];
    p.chopIntensity = v[P_CHOPINTENSITY] / 100.0;
    p.chopMix = v[P_CHOPMIX] / 100.0;
    p.chopSeed = v[P_CHOPSEED];
    p.chopStutter = v[P_CHOPSTUTTER];
    p.cGate = v[P_CGATE];
    p.cStut = v[P_CSTUT];
    p.cHalf = v[P_CHALF];
    p.cTape = v[P_CTAPE];
    p.cRev = v[P_CREV];
    p.softClip = v[P_SOFTCLIP];
    p.timeAlign = v[P_TIMEALIGN];
    return p;
}
