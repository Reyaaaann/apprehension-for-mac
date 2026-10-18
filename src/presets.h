// Apprehension - 20 factory presets for flipping samples (no JUCE dependency)
#pragma once
#include <vector>
#include "params.h"

struct PresetValue { Pid id; double v; };
struct Preset { const char* name; const char* desc; std::vector<PresetValue> values; };

// Index cheat-sheet used below:
//  Division 0:1/16 1:1/8T 2:1/8 3:1/4T 4:1/4 5:1/2 6:1 bar 7:2 bars      Duck division 0:1/16 1:1/8 2:1/4 3:1/2 4:1/1
//  Shape 0:6dB 1:12dB 2:18dB 3:24dB 4:36dB 5:48dB 6:72dB 7:96dB 8:Brickwall
//  Chorus 0:Classic 1:Ensemble 2:Dimension 3:Vibrato 4:Tape      Flangus 0:Fruity 1:Jet 2:Metallic 3:Soft 4:Wide
//  Reverb 0:Room 1:Hall 2:Plate 3:Chamber 4:Cathedral
//  Chop pattern 0:Auto 1-8:built-in 9:Custom     Chop rate 0:1/8 1:1/16 2:1/32     Stutter size 0:1/2 1:1/4 2:1/8
inline const std::vector<Preset>& presets() {
    static const std::vector<Preset> list = {
        {"Reverse Swell", "The classic riser: the sample plays backwards into every half-note beat, locked to the grid.",
         {{P_MIX, 100}, {P_DIVISION, 5}, {P_TIMEALIGN, 1}, {P_ATTACK, 150}, {P_SMOOTH, 25}, {P_FEEDBACK, 10}, {P_DIFFUSE, 30},
          {P_RVTYPE, 1}, {P_RVMIX, 18}, {P_RVSIZE, 55}, {P_RVDECAY, 2.2}, {P_LOWCUT, 90}, {P_LOWSLOPE, 1}, {P_OUTPUT, -1}}},

        {"Backwards Dream", "A slow, washed-out reverse bloom across a whole bar - for pads, vocals and long samples.",
         {{P_MIX, 75}, {P_DIVISION, 6}, {P_TIMEALIGN, 1}, {P_ATTACK, 450}, {P_SMOOTH, 80}, {P_FEEDBACK, 30}, {P_DIFFUSE, 45},
          {P_CHORUS, 30}, {P_CHORUSMODE, 1}, {P_HIGHCUT, 9500}, {P_HIGHSLOPE, 1}, {P_WIDTH, 130},
          {P_RVTYPE, 1}, {P_RVMIX, 40}, {P_RVSIZE, 70}, {P_RVDECAY, 5}, {P_RVDAMP, 55}, {P_RVMOD, 35}}},

        {"Half-Time Flip", "Drops parts of the sample into half-time on the 1/16 grid for that slowed, heavy bounce.",
         {{P_MIX, 35}, {P_DIVISION, 4}, {P_TIMEALIGN, 1}, {P_SIGNATURE, 1}, {P_CHOPPATTERN, 5}, {P_CHOPRATE, 1}, {P_CHOPINTENSITY, 85},
          {P_GPITCH, -2}, {P_HIGHCUT, 7500}, {P_HIGHSLOPE, 3}, {P_LOWCUT, 60}, {P_LOWSLOPE, 3}, {P_SOFTCLIP, 1},
          {P_RVTYPE, 0}, {P_RVMIX, 10}, {P_RVDECAY, 1.0}}},

        {"Chopped & Screwed", "Pitched down and chopped with tape-stops and stutters - the slowed-down Houston flip.",
         {{P_MIX, 45}, {P_DIVISION, 5}, {P_TIMEALIGN, 1}, {P_SIGNATURE, 1}, {P_CHOPPATTERN, 0}, {P_CHOPSEED, 12}, {P_CHOPINTENSITY, 75},
          {P_CGATE, 0}, {P_GPITCH, -4}, {P_HIGHCUT, 6500}, {P_HIGHSLOPE, 3}, {P_CHORUS, 20}, {P_CHORUSMODE, 4},
          {P_RVTYPE, 3}, {P_RVMIX, 25}, {P_RVDECAY, 3}, {P_RVDAMP, 60}, {P_SOFTCLIP, 1}}},

        {"Vocal Flip Up", "Raises a vocal sample up a few semitones with a light reverse shadow behind it.",
         {{P_MIX, 30}, {P_DIVISION, 4}, {P_TIMEALIGN, 1}, {P_ATTACK, 30}, {P_SMOOTH, 35}, {P_GPITCH, 4},
          {P_CHORUS, 20}, {P_CHORUSMODE, 0}, {P_LOWCUT, 180}, {P_LOWSLOPE, 1}, {P_HIGHCUT, 13000}, {P_HIGHSLOPE, 1},
          {P_RVTYPE, 2}, {P_RVMIX, 14}, {P_RVDECAY, 1.8}, {P_RVPRE, 30}, {P_WIDTH, 120}}},

        {"Soul Chop", "Rearranges a soul loop with gentle stutters, reverse slices and gaps. Change the Seed for new variations.",
         {{P_MIX, 20}, {P_DIVISION, 4}, {P_SIGNATURE, 1}, {P_CHOPPATTERN, 0}, {P_CHOPSEED, 7}, {P_CHOPINTENSITY, 70}, {P_CHOPRATE, 1},
          {P_CHOPSTUTTER, 1}, {P_HIGHCUT, 9000}, {P_HIGHSLOPE, 1}, {P_LOWCUT, 50}, {P_LOWSLOPE, 3}, {P_SOFTCLIP, 1},
          {P_RVTYPE, 0}, {P_RVMIX, 12}, {P_RVDECAY, 1.2}, {P_OUTPUT, -1}}},

        {"Stutter Roll", "Fast 1/32 stutters and gates for drum-roll style rolls and glitchy fills.",
         {{P_MIX, 15}, {P_SIGNATURE, 1}, {P_CHOPPATTERN, 8}, {P_CHOPRATE, 2}, {P_CHOPSTUTTER, 2}, {P_CHOPINTENSITY, 100}, {P_SOFTCLIP, 1}}},

        {"Tape Stop Drop", "Plays the sample straight, then a tape-stop slows it to a halt at the end of every bar.",
         {{P_MIX, 10}, {P_SIGNATURE, 1}, {P_CHOPPATTERN, 7}, {P_CHOPRATE, 1}, {P_CHOPINTENSITY, 100}, {P_SOFTCLIP, 1},
          {P_RVTYPE, 2}, {P_RVMIX, 10}, {P_RVDECAY, 1.4}}},

        {"Reverse Echo", "A backwards echo that repeats and darkens, ducking out of the way while the sample plays.",
         {{P_MIX, 60}, {P_DIVISION, 2}, {P_TIMEALIGN, 1}, {P_SMOOTH, 40}, {P_FEEDBACK, 55}, {P_DIFFUSE, 30},
          {P_LOWCUT, 200}, {P_LOWSLOPE, 1}, {P_HIGHCUT, 5500}, {P_HIGHSLOPE, 1}, {P_DUCKAMT, 40}, {P_DUCKREL, 250}, {P_DUCKMODE, 0},
          {P_RVTYPE, 0}, {P_RVMIX, 12}}},

        {"Pump Reverse", "Reversed sound that pumps on every quarter note, like sidechain compression. Great on pads.",
         {{P_MIX, 70}, {P_DIVISION, 5}, {P_TIMEALIGN, 1}, {P_ATTACK, 80}, {P_SMOOTH, 50}, {P_DUCKAMT, 70}, {P_DUCKMODE, 1},
          {P_DUCKDIV, 2}, {P_DUCKREL, 180}, {P_RVTYPE, 2}, {P_RVMIX, 30}, {P_RVDECAY, 2.5}}},

        {"Lo-Fi Flip", "Dusty, warm and slightly wobbly - tape chorus, rolled-off highs and soft clipping.",
         {{P_MIX, 40}, {P_DIVISION, 4}, {P_TIMEALIGN, 1}, {P_SMOOTH, 40}, {P_HIGHCUT, 4500}, {P_HIGHSLOPE, 4}, {P_LOWCUT, 90}, {P_LOWSLOPE, 3},
          {P_CHORUS, 40}, {P_CHORUSMODE, 4}, {P_SOFTCLIP, 1}, {P_RVTYPE, 0}, {P_RVMIX, 12}, {P_RVDECAY, 1.4}, {P_WIDTH, 70}, {P_OUTPUT, -1}}},

        {"Dusty Soul Loop", "A slightly lowered, warm loop with an occasional tape-stop. Quick to make a sample feel old.",
         {{P_MIX, 25}, {P_DIVISION, 5}, {P_TIMEALIGN, 1}, {P_GPITCH, -1}, {P_HIGHCUT, 7500}, {P_HIGHSLOPE, 1}, {P_LOWCUT, 60}, {P_LOWSLOPE, 1},
          {P_CHORUS, 25}, {P_CHORUSMODE, 4}, {P_SIGNATURE, 1}, {P_CHOPPATTERN, 7}, {P_CHOPINTENSITY, 40}, {P_CHOPMIX, 60}, {P_CHOPRATE, 1},
          {P_RVTYPE, 3}, {P_RVMIX, 18}, {P_RVDECAY, 2.2}, {P_SOFTCLIP, 1}}},

        {"Boom Bap Reverse", "Tight, punchy reverse hits that stop early - sits nicely under 90s-style drums.",
         {{P_MIX, 55}, {P_DIVISION, 5}, {P_TIMEALIGN, 1}, {P_SMOOTH, 15}, {P_CUTOFF, 60}, {P_RELEASE, 40}, {P_FEEDBACK, 15},
          {P_LOWCUT, 70}, {P_LOWSLOPE, 3}, {P_HIGHCUT, 8500}, {P_HIGHSLOPE, 3}, {P_SOFTCLIP, 1}}},

        {"Ambient Wash", "A huge two-bar reverse bloom with a cathedral tail. Turns any sample into a pad.",
         {{P_MIX, 85}, {P_DIVISION, 7}, {P_TIMEALIGN, 1}, {P_ATTACK, 800}, {P_SMOOTH, 90}, {P_FEEDBACK, 45}, {P_DIFFUSE, 60},
          {P_RVTYPE, 4}, {P_RVMIX, 60}, {P_RVSIZE, 100}, {P_RVDECAY, 10}, {P_RVDAMP, 60}, {P_RVMOD, 50}, {P_RVPRE, 40}, {P_WIDTH, 160},
          {P_CHORUS, 40}, {P_CHORUSMODE, 1}, {P_HIGHCUT, 8000}, {P_HIGHSLOPE, 1}, {P_OUTPUT, -2}}},

        {"Vaporwave Slow", "Pitched down with dimension chorus and a soft flange for a hazy, slowed-down mood.",
         {{P_MIX, 40}, {P_DIVISION, 6}, {P_TIMEALIGN, 1}, {P_ATTACK, 300}, {P_SMOOTH, 70}, {P_GPITCH, -3},
          {P_CHORUS, 50}, {P_CHORUSMODE, 2}, {P_FLANGUS, 20}, {P_FLANGUSMODE, 3},
          {P_RVTYPE, 1}, {P_RVMIX, 45}, {P_RVSIZE, 75}, {P_RVDECAY, 5}, {P_RVDAMP, 50},
          {P_LOWCUT, 100}, {P_LOWSLOPE, 1}, {P_HIGHCUT, 6000}, {P_HIGHSLOPE, 3}, {P_SOFTCLIP, 1}}},

        {"Jet Reverse", "A reversed sound with a deep, slow jet-plane flange sweeping through it.",
         {{P_MIX, 60}, {P_DIVISION, 3}, {P_TIMEALIGN, 1}, {P_ATTACK, 60}, {P_SMOOTH, 40}, {P_FEEDBACK, 35},
          {P_FLANGUS, 70}, {P_FLANGUSMODE, 1}, {P_LOWCUT, 150}, {P_LOWSLOPE, 1}, {P_RVTYPE, 2}, {P_RVMIX, 15}, {P_RVDECAY, 1.6}}},

        {"Metal Rise", "A reverse swell pitched up an octave with a metallic flange - a bright riser for transitions.",
         {{P_MIX, 70}, {P_DIVISION, 5}, {P_TIMEALIGN, 1}, {P_ATTACK, 250}, {P_SMOOTH, 60}, {P_PITCH, 12},
          {P_FLANGUS, 60}, {P_FLANGUSMODE, 2}, {P_HIGHCUT, 6000}, {P_HIGHSLOPE, 3}, {P_HIGHRES, 6},
          {P_RVTYPE, 0}, {P_RVMIX, 10}, {P_OUTPUT, -2}}},

        {"Frost Pad", "A frozen-feeling pad of endless reverse trails. Press Freeze on a moment you like to hold it.",
         {{P_MIX, 70}, {P_DIVISION, 6}, {P_TIMEALIGN, 1}, {P_SMOOTH, 100}, {P_FEEDBACK, 80}, {P_DIFFUSE, 60},
          {P_CHORUS, 50}, {P_CHORUSMODE, 1}, {P_RVTYPE, 4}, {P_RVMIX, 65}, {P_RVSIZE, 90}, {P_RVDECAY, 12}, {P_RVDAMP, 55}, {P_RVMOD, 45},
          {P_HIGHCUT, 7000}, {P_HIGHSLOPE, 1}, {P_LOWCUT, 120}, {P_LOWSLOPE, 1}, {P_WIDTH, 150}, {P_OUTPUT, -4}}},

        {"Glitch Flip", "Rapid 1/32 glitches with reversed layers underneath. Roll the Seed for endless variations.",
         {{P_MIX, 100}, {P_DIVISION, 4}, {P_TIMEALIGN, 1}, {P_SIGNATURE, 1}, {P_CHOPPATTERN, 0}, {P_CHOPSEED, 21}, {P_CHOPINTENSITY, 100},
          {P_CHOPRATE, 2}, {P_CHOPSTUTTER, 2}, {P_RVTYPE, 2}, {P_RVMIX, 20}, {P_RVDECAY, 1.5}, {P_SOFTCLIP, 1}}},

        {"Octave Down Flip", "Drops the whole sample an octave for a heavy, bassy flip, with the highs tucked away.",
         {{P_MIX, 25}, {P_DIVISION, 4}, {P_TIMEALIGN, 1}, {P_GPITCH, -12}, {P_HIGHCUT, 5000}, {P_HIGHSLOPE, 3},
          {P_CHORUS, 15}, {P_CHORUSMODE, 0}, {P_SOFTCLIP, 1}}},
    };
    return list;
}

// Value of parameter `id` with preset `index` applied on top of the defaults (index < 0 = plain defaults).
inline double presetValue(int index, int id) {
    double v = kSpec[id].def;
    if (index >= 0 && index < (int)presets().size())
        for (auto& pv : presets()[(size_t)index].values)
            if ((int)pv.id == id) v = pv.v;
    return clampToSpec(id, v);
}
