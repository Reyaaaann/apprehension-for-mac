#!/usr/bin/env python3
"""Generates assets/manual.html (embedded in the plugin and shipped with the installer) and src/HelpText.h (tooltips)
from the real parameter table, so ranges/defaults in the manual always match the plugin.
Run from the project folder:  g++ -std=c++17 tools/dump_specs.cpp -o /tmp/dump && python3 tools/gen_docs.py /tmp/dump"""
import html, json, subprocess, sys, pathlib

root = pathlib.Path(__file__).resolve().parent.parent
specs = json.loads(subprocess.check_output([sys.argv[1]]))
P = {p["id"]: p for p in specs["params"]}

# --------------------------------------------------------------------------- text for every control: (tooltip, manual text)
D = {
 "mix": ("Dry/wet balance. 0% = only your original sound, 100% = only the reversed sound.",
         "Blends your original (dry) sound with the reversed (wet) sound. It is an equal-power crossfade, so the loudness stays steady as you turn it. At 100% you hear only the reversed sound - turn on Time Align so it stays on the beat. The reversed sound is what the reverb, chorus, flangus, filters, ducking and width act on, so at 0% those are not heard."),
 "sync": ("On: each reversed chunk lasts a note length from Division and follows your project tempo. Off: it uses Time in milliseconds.",
          "Chooses how the length of each reversed chunk is set. On = a musical note length (Division) that follows your project's tempo. Off = a fixed time in milliseconds (Time). If your host does not send a tempo, Sync falls back to Time."),
 "timealign": ("Locks the reversed chunks to your project's beat grid, so the sample stays on the beat even at Mix 100%.",
               "Without Time Align, the reversed chunks start whenever you press play and slowly drift against the beat (each chunk overlaps the next a little). With Time Align on, every chunk starts exactly on a grid line of your project, and the reversed sound mirrors around each beat: something that was 100 ms before a beat comes back 100 ms after it, and a hit that was exactly on a beat comes back exactly on a beat. It needs the project to be playing (it uses the host's beat position). Keep Smooth low if you want hits to stay sharp."),
 "division": ("Length of each reversed chunk when Sync is on: 1/16 up to 2 bars. Short = rhythmic and stuttery, long = big swells.",
              "The note length of each reversed chunk when Sync is on. Short values (1/16, 1/8) give rhythmic, stuttery reverse hits. 1/4 and 1/2 are the classic reverse-into-the-beat sound. 1 bar and 2 bars give long, dramatic swells. T means triplet."),
 "time": ("Length of each reversed chunk in milliseconds when Sync is off (10 ms to 4 seconds).",
          "The length of each reversed chunk when Sync is off. Very short times (10-60 ms) turn into a gritty, pitched buzz; hundreds of milliseconds give classic reverse echoes."),
 "attack": ("How slowly each reversed chunk fades in. 0 = instant, higher = a smooth swell into the sound.",
            "How long each reversed chunk takes to fade in. 0 ms starts at full volume. 100-400 ms gives a smooth swell. It is this knob that decides how fast the reversed sound starts."),
 "cutoff": ("Where each reversed chunk ends. 100% plays the whole chunk; lower values stop it early, leaving gaps.",
            "Controls where each chunk stops. At 100% the whole chunk plays. At 50% only the first half of every chunk plays and then there is silence until the next chunk. Use it to make tight, gated reverse hits."),
 "release": ("How softly the sound fades out at the Cutoff point. Only matters when Cutoff is below 100%.",
             "The fade-out time at the Cutoff point. Short = a clean chop. Long = the chunk tails away gently. You only hear it when Cutoff is below 100%."),
 "smooth": ("Crossfade between one chunk and the next. Low = tight, punchy edges. High = chunks melt into each other.",
            "How much one reversed chunk overlaps and crossfades into the next. Low values keep every chunk sharp and punchy. High values blend the chunks into a continuous wash."),
 "feedback": ("Feeds the reversed sound back in so it repeats and trails away. High values make long echoing tails.",
              "Sends the reversed sound back into the buffer so it gets reversed again and echoes. 20-40% is a subtle trail, 60-80% builds long swirling repeats. The feedback is soft-limited, so it cannot run away."),
 "lowcut": ("High-pass filter on the reversed sound. 20 Hz = off. Raise it to remove low end from the reversed layer.",
            "A high-pass filter that removes low frequencies from the reversed sound. At its minimum (20 Hz) it is switched off. Use it to keep the reversed layer from muddying the bass."),
 "lowslope": ("How steep the Low Cut is: gentle 6 dB per octave up to a near-vertical brickwall.",
              "The steepness of the Low Cut. 6 dB per octave is a very gentle tilt. 12 and 24 dB are the usual choices. 36 to 96 dB are very steep, and Brickwall is an almost vertical wall (32nd order) that removes everything below the frequency. Watch the graph - it draws the exact curve."),
 "lowres": ("Adds a resonant peak right at the Low Cut frequency (0 to 12 dB).",
            "Boosts the frequencies right at the Low Cut corner for a more pronounced, vocal 'filter sweep' sound. 0 dB is neutral. It has no effect on the 6 dB shape."),
 "highcut": ("Low-pass filter on the reversed sound, down to 70 Hz. 20 kHz = off. Lower it to darken the reversed layer.",
             "A low-pass filter that removes high frequencies from the reversed sound. At its maximum (20 kHz) it is switched off. It goes all the way down to 70 Hz, so you can turn the reversed layer into a muffled low rumble."),
 "highslope": ("How steep the High Cut is: gentle 6 dB per octave up to a near-vertical brickwall.",
               "The steepness of the High Cut, from a gentle 6 dB per octave to a near-vertical Brickwall. The graph shows the exact curve."),
 "highres": ("Adds a resonant peak right at the High Cut frequency (0 to 12 dB).",
             "Boosts the frequencies right at the High Cut corner for a sharper, more 'synthy' filter sound. 0 dB is neutral. It has no effect on the 6 dB shape."),
 "diffuse": ("Smears the reversed sound so it sounds softer and more spread out.",
             "Runs the reversed sound through a chain of all-pass filters that smear transients without changing the tone. Higher values soften hard hits into a gentle haze."),
 "width": ("Stereo width of the reversed sound. 0% = mono, 100% = as recorded, 200% = extra wide.",
           "Narrows or widens the stereo image of the reversed sound only. 0% collapses it to mono, 100% leaves it as it is, 200% pushes it extra wide."),
 "chorus": ("Amount of chorus on the reversed sound.",
            "Thickens the reversed sound with a chorus. The Chorus Type knob picks the character; this knob sets how much."),
 "chorusmode": ("Chorus character: Classic, Ensemble, Dimension, Vibrato or Tape.",
                "Picks the kind of chorus. Classic = two voices, the familiar chorus. Ensemble = three slow voices, lush and wide. Dimension = four short voices with almost no obvious movement, just width. Vibrato = pitch wobble only, with no dry sound added. Tape = slow wow plus fast flutter, like an old tape machine."),
 "flangus": ("Amount of flange on the reversed sound.",
             "A sweeping comb-filter effect on the reversed sound, in the spirit of FL Studio's Fruity Flangus. The Flangus Type knob picks the character; this knob sets how much."),
 "flangusmode": ("Flangus character: Fruity, Jet, Metallic, Soft or Wide.",
                 "Picks the kind of flange. Fruity = the classic medium sweep. Jet = deep, slow and strong, like a passing plane. Metallic = negative feedback, a hollow ringing tone. Soft = gentle and subtle. Wide = the left and right sides sweep in opposite directions for a huge stereo sweep."),
 "output": ("Final output level in dB.",
            "The overall level leaving the plugin, from -24 dB to +12 dB. Use it to match your sample's loudness after adding effects."),
 "gpitch": ("Changes the pitch of the WHOLE signal (dry and reversed) by up to 12 semitones either way.",
            "Shifts the pitch of everything the plugin outputs - both your original sound and the reversed layer - without changing the speed or timing. Use it to retune a sample to your key (the Key Detector tells you what key it will land in). Adds a fixed 25 ms of latency that your DAW compensates automatically. Quality is best within about 5 semitones; the further you go, the more the sound takes on a granular, robotic character."),
 "pitch": ("Changes the pitch of the REVERSED sound only, up to 2 octaves either way. Raise it for risers, lower it for rumble.",
           "Shifts only the reversed layer, like speeding up or slowing down a tape (so the chunk is read faster or slower). Combine it with Overall Pitch: the reversed layer's total shift is the two added together. Very high values shorten the longest chunk the plugin can play, because it needs to read further back in time."),
 "refhz": ("Reference frequency (default 440 Hz) used by the pitch readout to show what a note becomes after shifting.",
           "A reference frequency for the pitch readout. Type a frequency (for example the pitch of the note you care about) and the panel shows what it becomes after the Overall and Reverse pitch changes, in Hz and as a note name, plus the difference in Hz. It does not change the sound."),
 "freeze": ("Stops recording and keeps looping the last reversed chunk until you turn it off.",
            "Freeze stops the plugin from recording new audio and keeps playing the last chunk backwards in a loop. Press it at a moment you like to hold a sound; press it again to carry on. Your original sound keeps playing through the Mix as normal."),
 "duckamt": ("How much the reversed sound is turned down by the ducking. 0% = off.",
             "Turns the reversed sound down so it gets out of the way. 0% = no ducking. 100% = it disappears completely when triggered. What triggers it is set by the Pump button."),
 "duckrel": ("How quickly the reversed sound comes back after being ducked.",
             "The recovery time after each duck. Short = snappy. Long = a slow swell back up. In Pump mode it is limited to one cycle of the chosen division."),
 "duckdiv": ("Pump mode only: how often the pumping happens (1/16, 1/8, 1/4, 1/2 or 1/1).",
             "In Pump mode, how often the ducking happens: every 1/16, 1/8, 1/4, 1/2 or 1/1 (a whole bar). 1/4 is a pulse on every beat, like sidechain compression from a kick. It follows the host tempo."),
 "duckmode": ("Off: the reversed sound ducks while YOU play (input ducking). On: it pumps on the beat (Pump).",
              "Off = Input ducking: the reversed sound gets quieter whenever your original sound is loud, so the reversed layer fills the gaps. On = Pump: the reversed sound dips on every beat of the chosen Division, whatever you play - the classic sidechain pump."),
 "rvtype": ("Reverb character: Room, Hall, Plate, Chamber or Cathedral.",
            "Picks the reverb's space. Room = small and quick. Hall = big and smooth. Plate = dense and bright, like a studio plate. Chamber = medium and warm. Cathedral = huge and slow. Size scales each one."),
 "rvmix": ("How much reverb is added to the reversed sound. 0% = no reverb.",
           "The amount of reverb. It is applied to the reversed sound, so the main Mix must be above 0% to hear it. The reverb keeps ringing after the reversed sound stops."),
 "rvsize": ("Size of the reverb space, from small to huge.",
            "Scales the size of the reverb space. Small = tight and close. Large = spacious and slow. Moving it while sound plays gives a gentle pitch glide in the tail."),
 "rvdecay": ("How long the reverb tail lasts, from 0.2 to 15 seconds.",
             "The length of the reverb tail (the time it takes to fade by 60 dB). Under 1 s is tight. 2-4 s is natural. 8 s and up is an ambient wash."),
 "rvpre": ("Delay before the reverb starts, up to 250 ms. Keeps the reversed sound clear before the reverb arrives.",
           "A pause before the reverb begins. 20-40 ms keeps the front of the reversed sound clear before the reverb blooms; long pre-delays make a distinct echo-then-wash effect."),
 "rvdamp": ("How quickly the reverb loses high frequencies. High = dark and warm, low = bright.",
            "How much the reverb tail darkens as it fades. 0% keeps it bright and glassy. High values make it warm and dull, like a soft room."),
 "rvwidth": ("Stereo width of the reverb. 0% = mono, 200% = extra wide.",
             "The stereo width of the reverb only."),
 "rvmod": ("Slowly moves the reverb's delay lines to remove metallic ringing and make the tail shimmer.",
           "Gently modulates the reverb to avoid a metallic ring and add shimmer. Low values are natural; high values make the tail wobble like a chorus."),
 "signature": ("@Reyaan. Signature Mode: auto-chops your sound on the beat with gates, stutters, half-time, tape-stops and reverse slices. The whole UI turns pink.",
               "Switches on auto-chop. On every step of the grid the plugin does one thing to your sound: plays it normally, silences it (gate), repeats a slice (stutter), plays it at half speed (half-time), slows it to a stop (tape-stop) or plays the last step backwards (reverse). It follows the host tempo (120 BPM if there is none). The window turns hot pink on black, with pink flashes on chop hits."),
 "chopmode": ("Which 16-step pattern is used: Auto (changes every bar), 1-8 built-in, or Custom (your own steps).",
              "Picks the pattern of actions. Auto = a different built-in pattern each bar (the Seed changes which). 1-8 = one fixed built-in pattern. Custom = the steps you drew in the grid. Clicking a step in the grid switches this to Custom automatically."),
 "choprate": ("How long each step lasts: 1/8, 1/16 or 1/32 note.",
              "The length of each grid step. 1/8 = slow, big chops. 1/16 = standard. 1/32 = fast glitchy chops. A pattern is always 16 steps long."),
 "chopintensity": ("How many steps get chopped. 100% = every step in the pattern, 0% = none.",
                   "The chance that each step actually does its action. At 100% the pattern plays exactly as shown. Lower values leave more steps untouched, for a looser, subtler chop."),
 "chopmix": ("Blend between the unchopped and the chopped sound. 100% = fully chopped.",
             "Mixes the chopped result with the normal sound. 100% = only the chopped sound. 50% = half and half, which makes the chops feel like accents on top of the groove."),
 "chopseed": ("Changes which patterns Auto picks and which steps Intensity skips. Try different numbers for new variations.",
              "A number from 0 to 99 that picks one 'version' of the randomness. The same seed always gives the same result, so you can find a variation you like and keep it. Roll seed picks a random one."),
 "chopstutter": ("How long the repeated slice is in a stutter step: 1/2, 1/4 or 1/8 of the step.",
                 "The size of the slice that repeats in a stutter step. 1/2 = two repeats per step, 1/4 = four, 1/8 = eight (fast, buzzy)."),
 "cgate": ("Allow gates (silent steps) in the chop.", "Turn off to stop the chop from ever silencing a step. Steps that are switched off play normally."),
 "cstut": ("Allow stutters in the chop.", "Turn off to stop the chop from ever stuttering."),
 "chalf": ("Allow half-time steps in the chop.", "Turn off to stop the chop from ever slowing to half speed."),
 "ctape": ("Allow tape-stops in the chop.", "Turn off to stop the chop from ever doing a tape-stop."),
 "crev": ("Allow reverse slices in the chop.", "Turn off to stop the chop from ever playing a step backwards."),
 "softclip": ("Rounds off loud peaks smoothly so they never go over 0 dB. Quiet sound is untouched.",
              "A gentle limiter-style safety. Anything below about -3 dB passes untouched; louder peaks are rounded off smoothly so the output can never go past 0 dBFS. Handy after adding feedback, chop or pitch effects."),
}
SECTIONS = [
 ("Time - the reverse engine", ["sync", "timealign", "division", "time", "attack", "cutoff", "release", "smooth"]),
 ("Pitch", ["gpitch", "pitch", "refhz"]),
 ("Space & Modulation", ["feedback", "diffuse", "width", "chorus", "chorusmode", "flangus", "flangusmode"]),
 ("Ducking", ["duckamt", "duckrel", "duckdiv", "duckmode"]),
 ("Reverb", ["rvtype", "rvmix", "rvsize", "rvdecay", "rvpre", "rvdamp", "rvwidth", "rvmod"]),
 ("Filter", ["lowcut", "lowslope", "lowres", "highcut", "highslope", "highres"]),
 ("Output", ["mix", "output", "softclip"]),
 ("Header", ["freeze", "signature"]),
 ("Signature Chop", ["chopintensity", "chopmix", "chopseed", "choprate", "chopmode", "chopstutter", "cgate", "cstut", "chalf", "ctape", "crev"]),
]
assert set(D) == set(P), set(P) ^ set(D)
assert sorted(sum([s[1] for s in SECTIONS], [])) == sorted(P)

# --------------------------------------------------------------------------- value formatting
def fnum(x, nd=None):
    if nd is None:
        return ("%g" % x)
    return ("%.*f" % (nd, x)).rstrip("0").rstrip(".")
def unit(p, x):
    f = p["fmt"]
    if f == 1: return fnum(x) + " %"
    if f == 2: return fnum(x) + " ms"
    if f == 3: return fnum(x) + " s"
    if f == 4: return fnum(x) + " Hz"
    if f == 5: return fnum(x) + " dB"
    if f == 6: return ("+" if x > 0 else "") + fnum(x) + " st"
    if f == 7: return fnum(x)
    if f == 8: return "Off" if x <= 20.5 else fnum(x) + " Hz"
    if f == 9: return "Off" if x >= 19999.5 else fnum(x) + " Hz"
    return fnum(x)
def rng(p):
    if p["kind"] == 1: return "On / Off"
    if p["kind"] == 2: return ", ".join(p["choices"])
    mn, mx = p["min"], p["max"]
    f = p["fmt"]
    base = {1: "%", 2: "ms", 3: "s", 4: "Hz", 5: "dB", 6: "st", 7: "", 8: "Hz", 9: "Hz"}.get(f, "")
    s = "%s to %s %s" % (fnum(mn), fnum(mx), base)
    if f == 8: s += " (20 = off)"
    if f == 9: s += " (20000 = off)"
    return s.strip()
def dflt(p):
    if p["kind"] == 1: return "On" if p["def"] >= 0.5 else "Off"
    if p["kind"] == 2: return p["choices"][int(p["def"])]
    return unit(p, p["def"])
def pretty_name(p):
    return {"Tempo Sync": "Sync", "Time (free)": "Time", "Reverse Pitch": "Reverse Pitch", "Duck Pump Mode": "Pump",
            "@Reyaan. Signature Mode": "@Reyaan. Signature Mode", "Chop Pattern": "Pattern", "Chop Rate": "Rate",
            "Chop Intensity": "Intensity", "Chop Mix": "Chop Mix", "Chop Seed": "Seed", "Stutter Size": "Stutter Size",
            "Chop: Gate": "Gate", "Chop: Stutter": "Stutter", "Chop: Half-time": "Half-time", "Chop: Tape-stop": "Tape-stop",
            "Chop: Reverse": "Reverse", "Duck Amount": "Amount", "Duck Release": "Release", "Duck Division": "Division",
            "Reverb Type": "Type", "Reverb Mix": "Mix", "Reverb Size": "Size", "Reverb Decay": "Decay", "Reverb Pre-Delay": "Pre-Delay",
            "Reverb Damping": "Damping", "Reverb Width": "Width", "Reverb Modulation": "Modulation", "Low Cut Slope": "Low Shape",
            "Low Cut Resonance": "Low Reso", "High Cut Slope": "High Shape", "High Cut Resonance": "High Reso",
            "Chorus Mode": "Chorus Type", "Flangus Mode": "Flangus Type", "Reference Hz": "Ref Hz"}.get(p["name"], p["name"])

# --------------------------------------------------------------------------- HelpText.h
def cstr(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'
extra = {
 "Preset": "Pick one of 20 ready-made sample-flipping presets, or Init to go back to the default sound. Use the arrows to step through them. '(edited)' appears once you change a knob.",
 "Theme": "Change the look. Only the default theme is built in - add your own as small .json files (see the manual).",
 "Manual": "Open the user manual in your web browser.",
 "Roll": "Picks a random Seed for new chop variations.",
 "Clear": "Sets every step in the Custom pattern to 'normal' so you can draw your own from scratch.",
 "Grid": "The 16 chop steps. Click a step to change what it does; right-click goes backwards. Editing switches Pattern to Custom.",
 "Graph": "Shows the exact filter curve made by Low Cut and High Cut, with their shapes and resonance.",
}
out = ['// GENERATED by tools/gen_docs.py - tooltip text for every control. Do not edit by hand.', '#pragma once', '#include "params.h"', '',
       'inline const char* const kTips[P_COUNT] = {']
for pid in [p["id"] for p in specs["params"]]:
    out.append("    %s," % cstr(D[pid][0]))
out.append("};")
for k, v in extra.items():
    out.append("inline const char* const kTip%s = %s;" % (k, cstr(v)))
(root / "src" / "HelpText.h").write_text("\n".join(out) + "\n")

# --------------------------------------------------------------------------- manual.html
e = html.escape
def table(ids):
    rows = []
    for pid in ids:
        p = P[pid]
        rows.append("<tr><td class=n>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>" % (e(pretty_name(p)), e(rng(p)), e(dflt(p)), e(D[pid][1])))
    return "<table><tr><th>Control</th><th>Range / options</th><th>Default</th><th>What it does</th></tr>%s</table>" % "".join(rows)

def pv(pid, v):
    p = P[pid]
    if p["kind"] == 1: return "On" if v >= 0.5 else "Off"
    if p["kind"] == 2: return p["choices"][int(v)]
    return unit(p, v)

preset_rows = []
for i, ps in enumerate(specs["presets"], 1):
    key = ", ".join("%s %s" % (pretty_name(P[k]), pv(k, v)) for k, v in ps["values"].items() if k in ("mix", "division", "gpitch", "pitch", "signature", "chopmode", "rvtype", "rvmix"))
    preset_rows.append("<tr><td>%02d</td><td class=n>%s</td><td>%s</td><td class=s>%s</td></tr>" % (i, e(ps["name"]), e(ps["desc"]), e(key)))

body = f"""
<h1>Apprehension</h1>
<p class=by>User Manual &nbsp;&middot;&nbsp; Designed by Reyaan. &nbsp;&middot;&nbsp; version 1.3.1 &nbsp;&middot;&nbsp; Windows and macOS</p>
<div class=toc><b>Contents</b>
<a href="#start">1. Quick start</a><a href="#how">2. How it works</a><a href="#tour">3. The window, control by control</a>
<a href="#sig">4. Signature Mode (auto-chop)</a><a href="#presets">5. The 20 presets</a><a href="#themes">6. Themes</a>
<a href="#recipes">7. Recipes for flipping samples</a><a href="#faq">8. Questions and troubleshooting</a></div>

<h2 id=start>1. Quick start</h2>
<ol>
<li><b>Put it on a sample.</b> In FL Studio, load Apprehension in an effect slot on the mixer track that holds your sample (open an empty slot's menu and pick Apprehension from the plugin list; run <i>Options &gt; Manage plugins &gt; Find installed plugins</i> first if you do not see it). Works the same in FL Studio for Windows and for Mac.</li>
<li><b>Pick a preset.</b> Click the preset name at the top (or the arrows) and choose one of the 20 sample-flipping presets. Press play. That is all you need to get a sound.</li>
<li><b>Adjust to taste.</b> Hover over any control for a one-line explanation. Turn <b>Mix</b> to set how much reversed sound you hear, and <b>Overall Pitch</b> to retune the whole sample.</li>
</ol>
<p class=tip><b>Handy things to know:</b> double-click a knob to reset it &middot; click the number under a knob to type a value &middot; drag a corner of the window to resize it &middot; hover over anything to see what it does.</p>

<h2 id=how>2. How it works</h2>
<p>Apprehension constantly records the sound that goes into it. Every so often it plays the last stretch of that recording <b>backwards</b>. Each stretch is called a <b>chunk</b>, and the <b>Division</b> (or <b>Time</b>) knob sets how long it is. Your original sound still passes straight through, and the <b>Mix</b> knob blends the two.</p>
<p>The reversed sound then travels through this chain, in order:</p>
<ol>
<li><b>Pitch (Reverse Pitch), Attack, Cutoff/Release, Smooth</b> - shape each chunk.</li>
<li><b>Diffusion</b> - softens the sound.</li>
<li><b>Low Cut and High Cut filters</b> - the point where <b>Feedback</b> is taken from, so repeats are filtered too.</li>
<li><b>Chorus</b>, then <b>Flangus</b>, then <b>Reverb</b>.</li>
<li><b>Ducking</b>, then <b>Width</b>.</li>
</ol>
<p>After that it is mixed with your original sound (<b>Mix</b>), set to the <b>Output</b> level, and optionally run through <b>Signature Mode</b>'s auto-chop, then <b>Overall Pitch</b>, then <b>Soft Clip</b>. Everything in this last group affects <i>everything</i> you hear, not only the reversed layer.</p>
<p class=tip><b>Latency:</b> Overall Pitch adds a fixed 25 ms delay (always, so nothing shifts when you move the knob). The plugin tells your DAW, and FL Studio compensates automatically, so everything stays in time.</p>

<h2 id=tour>3. The window, control by control</h2>
<h3>The header</h3>
<ul>
<li><b>Logo and name</b> - Apprehension, designed by Reyaan.</li>
<li><b>Preset bar</b> - the arrows step through presets; click the name for the full list (Init sets everything to the default sound). After you move a knob the name shows "(edited)".</li>
<li><b>@Reyaan. Signature Mode</b> and <b>Freeze</b> buttons (explained below).</li>
<li><b>Theme</b> - change the look (section 6). <b>Manual</b> - opens this manual in your browser.</li>
</ul>
"""
for title, ids in SECTIONS:
    body += "<h3>%s</h3>%s" % (e(title), table(ids))
    if title == "Pitch":
        body += """<p><b>Key detector (the panel under Pitch).</b> It listens to your incoming sound for a few seconds, works out which musical key it is in (for example "A minor") and shows a bar for each of the 12 notes. Underneath it shows what that key becomes after your <b>Overall Pitch</b> change (and, in small writing, after the Reverse Pitch change too). A fraction of a semitone is shown in cents. It works best on melodic samples with a few seconds of music; drums, noise and very quiet sounds will confuse it, and it shows a confidence percentage so you know how sure it is. It also shows the pitch readout in Hz: the reference frequency you typed, and what it turns into.</p>"""
    if title == "Time - the reverse engine":
        body += """<p><b>Attack, Cutoff and Release together</b> make the shape of every reversed chunk: Attack = how fast it starts, Cutoff = where it ends, Release = how softly it ends.</p>"""
    if title == "Space & Modulation":
        body += """<p><b>Chorus types:</b> Classic, Ensemble, Dimension, Vibrato, Tape. &nbsp; <b>Flangus types:</b> Fruity, Jet, Metallic, Soft, Wide. They are described in the table above.</p>"""
    if title == "Filter":
        body += """<p>The graph shows the real curve made by both filters, from 20 Hz to 20 kHz. <b>Low Cut</b> removes everything below its frequency; <b>High Cut</b> removes everything above. Shapes run 6, 12, 18, 24, 36, 48, 72, 96 dB per octave and Brickwall. The filters act on the reversed sound only (your original sound is not filtered).</p>"""
    if title == "Output":
        body += """<p>The level meter under the knobs shows the peak output level in dB.</p>"""

body += f"""
<h2 id=sig>4. Signature Mode (auto-chop)</h2>
<p>Press <b>@Reyaan. Signature Mode</b> and the plugin starts auto-chopping your sound on the beat. The window also turns black and hot pink, with pink flashes on the chops (never more than about three a second).</p>
<p>The row of 16 boxes at the bottom is the pattern. Each box is one step, and each step does one of these things:</p>
<table><tr><th>Step</th><th>What happens</th></tr>
<tr><td class=n>-</td><td>Plays normally.</td></tr>
<tr><td class=n>gate</td><td>Silence for that step.</td></tr>
<tr><td class=n>stutter</td><td>Repeats a short slice of the step (the size is set by Stutter Size).</td></tr>
<tr><td class=n>half</td><td>Plays the step at half speed - half-time, one octave down.</td></tr>
<tr><td class=n>tape</td><td>A tape-stop: it slows down to a stop within the step.</td></tr>
<tr><td class=n>reverse</td><td>Plays the previous step backwards.</td></tr></table>
<p><b>Make it your own:</b></p>
<ul>
<li><b>Click a step</b> to change what it does (click again to move to the next kind; <b>right-click</b> goes backwards). The first time you click, Pattern switches to <b>Custom</b> and keeps whatever was showing, so you are editing from the current pattern. Your Custom pattern is saved with your project.</li>
<li><b>Clear steps</b> sets all 16 to "normal" so you can build a pattern from nothing.</li>
<li><b>Intensity</b> decides how many of the steps actually do their thing. <b>Chop Mix</b> blends the chopped result with the normal sound.</li>
<li><b>Rate</b> sets the step length (1/8, 1/16 or 1/32). <b>Stutter Size</b> sets how long a stutter slice is.</li>
<li><b>Pattern</b> chooses Auto, 1-8 or Custom. In Auto a different built-in pattern is picked every bar. <b>Seed</b> changes which patterns Auto picks and which steps Intensity skips; <b>Roll seed</b> picks a random seed. Same seed, same result.</li>
<li><b>Gate, Stutter, Half-time, Tape-stop, Reverse</b> switches let you ban any kind of step from happening at all.</li>
</ul>
<p>The playing step lights up. The chop follows your project's tempo (it assumes 120 BPM if the host does not send one) and works on the final sound, so it chops both your original and the reversed layer together. Use <b>Time Align</b> too and everything stays locked to the beat.</p>

<h2 id=presets>5. The 20 presets</h2>
<p>These are starting points for flipping samples. They are all built from the same knobs you see, so after loading one you can tweak anything. Load a preset, put it on your sample, then adjust <b>Mix</b>, <b>Overall Pitch</b> and, for the chopping ones, the <b>Seed</b>. Most presets use Time Align, so press play for them to lock to the beat.</p>
<table><tr><th>#</th><th>Name</th><th>What it is for</th><th>Key settings</th></tr>{"".join(preset_rows)}</table>

<h2 id=themes>6. Themes</h2>
<p>Only the black-and-pink look is built in. You (and anyone you share with) add the others yourselves. Click <b>Theme</b>:</p>
<ul>
<li><b>Create a theme template</b> writes a file called <i>My Theme.json</i> into the themes folder and shows it to you. The folder is <code>%APPDATA%\\Reyaan\\Apprehension\\Themes</code> on Windows and <code>~/Library/Application Support/Reyaan/Apprehension/Themes</code> on a Mac.</li>
<li>Open the file in Notepad (or TextEdit on a Mac - choose Format > Make Plain Text), change the colours (written like <code>#ff3d9a</code>), save it, then choose <b>Reload themes</b>. Your theme is now in the Theme menu. Send the .json file to a friend and they can drop it into the same folder.</li>
<li><b>Open themes folder</b> opens the folder in Explorer (Windows) or Finder (Mac).</li>
</ul>
<p>A theme can set the background gradient, glow colours, knob colours, panel colour and opacity, text colours, and a font name (any font installed on the computer). It can also point at a <code>"image"</code> (a still picture that slowly pans and zooms behind the plugin) or a <code>"frames"</code> folder of numbered pictures (for example exported from your own video with <code>ffmpeg -i clip.mp4 -vf fps=24,scale=640:-1 frames/%04d.jpg</code>) that plays as a loop. Only use pictures and footage you own or have permission to use.</p>
<p>The theme you choose is saved with your project.</p>

<h2 id=recipes>7. Recipes for flipping samples</h2>
<ol>
<li><b>Reverse riser into the drop:</b> load <i>Reverse Swell</i>; raise Attack for a longer swell; put Mix at 100%. Automate Mix or Cutoff to build tension.</li>
<li><b>Instant new loop from a chord sample:</b> load <i>Soul Chop</i>, then try Seed values until the arrangement feels right. Use Chop Mix at 50% to keep the original groove under it.</li>
<li><b>Re-key a sample to your beat:</b> look at the Key Detector, decide which key you want, and turn <b>Overall Pitch</b> until the "After" key matches. It does not change the speed, so it stays in time.</li>
<li><b>Slowed and lo-fi:</b> load <i>Vaporwave Slow</i> or <i>Lo-Fi Flip</i> and lower Overall Pitch by 2-4 semitones. Use High Cut to darken it.</li>
<li><b>Keep the groove clear:</b> turn on Ducking in Input mode so the reversed layer drops while the original plays, or use Pump with 1/4 to make it breathe with the kick.</li>
<li><b>Hold a moment:</b> press <b>Freeze</b> on a nice moment; the last chunk loops backwards until you press it again. Add reverb and chorus for a pad.</li>
</ol>

<h2 id=faq>8. Questions and troubleshooting</h2>
<p><b>I hear nothing different.</b> Check <b>Mix</b> is above 0%. The reverb, chorus, flangus, filters and ducking only act on the reversed sound, so they need some Mix. Also check Signature Mode and Time Align need the project to be playing.</p>
<p><b>Time Align does nothing / sounds late.</b> It follows the host's beat position, so press play. If your DAW's delay compensation is switched off, the plugin's 25 ms of latency will not be corrected - turn it back on (FL Studio normally does this automatically unless you switched it off in its settings).</p>
<p><b>The sound is too quiet or too loud after a preset.</b> Presets are level-matched approximately; use <b>Output</b> and, if needed, <b>Soft Clip</b>.</p>
<p><b>The reverb tail keeps going.</b> That is the tail of the reverb (up to 15 s). Lower Decay or Mix.</p>
<p><b>The Key Detector says "listening...".</b> It needs a few seconds of audio that is not too quiet. It reads your original (dry) sound, before any effect.</p>
<p><b>The text looks different on my computer.</b> The plugin uses the first cursive font it finds on your computer (Apple Chancery or Snell Roundhand on a Mac; Monotype Corsiva, Palace Script, Edwardian Script, Brush Script, Segoe Script or Lucida Handwriting on a PC). A theme can name a different installed font.</p>
<p><b>Mac: macOS says the installer or plugin 'cannot be opened' or is 'from an unidentified developer'.</b> This happens with any app or plugin that has not been signed with a paid Apple developer certificate. Right-click (or Control-click) the installer, choose <b>Open</b>, then <b>Open</b> again. If it was blocked already, open <i>System Settings &gt; Privacy &amp; Security</i>, scroll down and click <b>Open Anyway</b>. If your host still refuses to load the plugin, open Terminal and run <code>xattr -cr /Library/Audio/Plug-Ins/VST3/Apprehension.vst3</code> (and the same for <code>/Library/Audio/Plug-Ins/Components/Apprehension.component</code>), then rescan plugins.</p>
<p><b>Mac: where are the plugins installed?</b> The VST3 goes to <code>/Library/Audio/Plug-Ins/VST3</code> and the Audio Unit (for Logic and GarageBand) to <code>/Library/Audio/Plug-Ins/Components</code>. The manual is also placed in <code>/Library/Application Support/Reyaan/Apprehension</code>. In FL Studio for Mac use <i>Options &gt; Manage plugins &gt; Find installed plugins</i>.</p>
<p><b>Pink flashing.</b> Signature Mode flashes the background on some chop hits, at most about three times a second. If flashing bothers you, do not use Signature Mode, or ask for a calmer theme.</p>
<p><b>Where are my settings saved?</b> Everything, including your Custom chop pattern, the preset name and the theme, is saved inside your project file.</p>
<p class=by>Apprehension &middot; Designed by Reyaan.</p>
"""

css = """
body{margin:0;background:#07030a;color:#f4e3ec;font:16px/1.55 Georgia,'Times New Roman',serif}
main{max-width:1000px;margin:0 auto;padding:48px 28px 80px}
h1{font:400 64px 'Monotype Corsiva','Palace Script MT','Edwardian Script ITC','Segoe Script','Brush Script MT',cursive;margin:0;background:linear-gradient(90deg,#fff,#ff6ec0);-webkit-background-clip:text;background-clip:text;color:transparent}
h2{font:400 38px 'Monotype Corsiva','Palace Script MT','Segoe Script',cursive;color:#ff86bd;border-bottom:1px solid #4a1636;padding-bottom:6px;margin-top:52px}
h3{font:400 27px 'Monotype Corsiva','Palace Script MT','Segoe Script',cursive;color:#ffa6d2;margin:34px 0 8px}
.by{color:#ff6ec0;font-style:italic}
table{border-collapse:collapse;width:100%;margin:10px 0 18px;font-size:14.5px}
th{background:#1a0612;color:#ff86bd;text-align:left;padding:8px 10px;border-bottom:1px solid #5a1a40}
td{padding:8px 10px;border-bottom:1px solid #2a0f1f;vertical-align:top}
td.n{color:#ffa6d2;white-space:nowrap;font-weight:bold}
td.s{color:#c98ab0;font-size:12.5px}
.toc{background:#12050d;border:1px solid #4a1636;border-radius:12px;padding:14px 20px;margin:22px 0;display:grid;grid-template-columns:1fr 1fr;gap:2px 20px}
.toc b{grid-column:1/3;color:#ff86bd;margin-bottom:4px}
.toc a{display:block}
a{color:#ff6ec0;text-decoration:none}a:hover{text-decoration:underline}
.tip{background:#1a0612;border-left:4px solid #ff3d9a;padding:10px 16px;border-radius:0 8px 8px 0}
code{background:#1a0612;padding:1px 6px;border-radius:4px;color:#ffa6d2}
li{margin:5px 0}
@media print{body{background:#fff;color:#111}h1,h2,h3{color:#a0125f;background:none;-webkit-text-fill-color:#a0125f}td,th{border-color:#ccc}.toc,.tip,code,th{background:#f6eef2;color:#111}}
"""
doc = "<!doctype html><html lang=en><head><meta charset=utf-8><title>Apprehension - User Manual</title><style>%s</style></head><body><main>%s</main></body></html>" % (css, body)
(root / "assets").mkdir(exist_ok=True)
(root / "assets" / "manual.html").write_text(doc, encoding="utf-8")
(root / "docs").mkdir(exist_ok=True)
(root / "docs" / "Apprehension Manual.html").write_text(doc, encoding="utf-8")
print("wrote HelpText.h and manual.html (%d bytes)" % len(doc))
