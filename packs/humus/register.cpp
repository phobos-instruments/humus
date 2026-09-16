// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "hum/Registry.h"

#include "DNA/DNA.h"
#include "Mineral/Mineral.h"
#include "Rhizome/Rhizome.h"
#include "Substrate/Substrate.h"
#include "ValveFilter/ValveFilter.h"
#include "MidiMonitor/MidiMonitor.h"
#include "OscMonitor/OscMonitor.h"
#include "Phono/Phono.h"
#include "Prism/Prism.h"
#include "Fern/Fern.h"
#include "Verbatim/Verbatim.h"
#include "Decomposer/Decomposer.h"
#include "Trellis/Trellis.h"
#include "Wave/Wave.h"
#include "Tuning/TuningNode.h"
#include "Deck/Deck.h"
#include "Follower/Follower.h"
#include "LFO/LFO.h"
#include "DigiRust/DigiRust.h"
#include "Spark/Spark.h"
#include "Halogen/Halogen.h"
#include "Drops/Drops.h"
#include "FilePlayer/FilePlayer.h"
#include "MidiPlayer/MidiPlayer.h"
#include "Graft/Graft.h"
#include "Leafcutter/Leafcutter.h"
#include "Cluster/Cluster.h"
#include "Gate/Gate.h"
#include "Harmonizer/Harmonizer.h"
#include "Math/Math.h"
#include "Morse/Morse.h"
#include "Button/Button.h"
#include "Number/Number.h"
#include "RNG/RNG.h"
#include "Paulstretch/Paulstretch.h"
#include "PinkTrombone/PinkTrombone.h"
#include "Slider/Slider.h"
#include "VuMeter/VuMeter.h"
#include "Ph/Ph.h"
#include "PianoRoll/PianoRoll.h"
#include "Sampler/Sampler.h"
#include "Board/Board.h"
#include "SerialIn/SerialIn.h"
#include "Sig/Sig.h"
#include "SerialOut/SerialOut.h"
#include "Notes/Notes.h"
#include "Var/Var.h"
#include "Sequence/Sequence.h"
#include "SideChain/SideChain.h"
#include "AudioTrack/AudioTrack.h"
#include "SoundSpace/SoundSpace.h"
#include "SpectralFilter/SpectralFilter.h"
#include "SpectralFreeze/SpectralFreeze.h"
#include "SpectralMorph/SpectralMorph.h"
#include "StereoTool/StereoTool.h"
#include "TransientShaper/TransientShaper.h"
#include "Console/Console.h"
#include "Acid/Acid.h"
#include "Grit/Grit.h"
#include "Silt/Silt.h"
#include "Bloom/Bloom.h"
#include "Spectrum/Spectrum.h"
#include "Cicada/Cicada.h"
#include "Kick/Kick.h"
#include "Microdot/Microdot.h"
#include "Crossover/Crossover.h"
#include "Helix/Helix.h"
#include "Repeater/Repeater.h"
#include "Riff/Riff.h"
#include "SideKick/SideKick.h"
#include "Siren/Siren.h"
#include "Steps/Steps.h"
#include "Dynamics/Dynamics.h"
#include "ParaEQ/ParaEQ.h"
#include "Flanger/Flanger.h"
#include "Phaser/Phaser.h"
#include "SChorus/SChorus.h"
#include "FrequencyShifter/FrequencyShifter.h"
#include "TestGen/TestGen.h"

namespace hum {

void hum_register_layouts_humus(Registry&);

void hum_register_pack_humus(Registry& r) {
    hum_register_layouts_humus(r);
    r.registerClass("MidiPlayer", [] { return std::make_unique<MidiPlayer>(); });
    r.registerClass("Deck", [] { return std::make_unique<Deck>(); });
    r.registerClass("Substrate", [] { return std::make_unique<Substrate>(); });
    r.registerClass("Mineral",   [] { return std::make_unique<Mineral>(); });
    r.registerClass("Rhizome",   [] { return std::make_unique<Rhizome>(); });
    r.registerClass("Morse",     [] { return std::make_unique<Morse>(); });
    r.registerClass("DigiRust",  [] { return std::make_unique<DigiRust>(); });
    r.registerClass("Spark",     [] { return std::make_unique<Spark>(); });
    r.registerClass("Halogen",   [] { return std::make_unique<Halogen>(); });
    r.registerClass("Drops",     [] { return std::make_unique<Drops>(); });
    r.registerClass("Tuning",    [] { return std::make_unique<TuningNode>(); });
    r.registerClass("DNA",       [] { return std::make_unique<DNA>(); });
    r.registerClass("PianoRoll", [] { return std::make_unique<PianoRoll>(); });
    r.registerClass("Sequence",  [] { return std::make_unique<Sequence>(); });
    r.registerClass("AudioTrack", [] { return std::make_unique<AudioTrack>(); });
    r.registerClass("Sampler", [] { return std::make_unique<Sampler>(); });
    r.registerClass("FilePlayer", [] { return std::make_unique<FilePlayer>(); });
    r.registerClass("Graft", [] { return std::make_unique<Graft>(); });
    r.registerClass("Leafcutter", [] { return std::make_unique<Leafcutter>(); });
    r.registerClass("SoundSpace", [] { return std::make_unique<SoundSpace>(); });
    r.registerClass("MidiMonitor", [] { return std::make_unique<MidiMonitor>(); });
    r.registerClass("OscMonitor", [] { return std::make_unique<OscMonitor>(); });
    r.registerClass("Decomposer", [] { return std::make_unique<Decomposer>(); });
    r.registerClass("Phono", [] { return std::make_unique<Phono>(); });
    r.registerClass("Trellis", [] { return std::make_unique<Trellis>(); });
    r.registerClass("Wave", [] { return std::make_unique<Wave>(); });
    r.registerClass("Prism", [] { return std::make_unique<Prism>(); });
    r.registerClass("TransientShaper", [] { return std::make_unique<TransientShaper>(2); });
    r.registerClass("STransientShaper", [] { return std::make_unique<TransientShaper>(2); });
    r.registerClass("MTransientShaper", [] { return std::make_unique<TransientShaper>(1); });
    r.registerClass("Helix", [] { return std::make_unique<Helix>(); });
    r.registerClass("Crossover", [] { return std::make_unique<Crossover>(3); });
    for (int n = 2; n <= 5; ++n)
        r.registerClass("Crossover" + std::to_string(n),
                        [n] { return std::make_unique<Crossover>(n); });
    r.registerClass("Fern", [] { return std::make_unique<Fern>(2); });
    r.registerClass("Verbatim", [] { return std::make_unique<Verbatim>(); });
    r.registerClass("SFern", [] { return std::make_unique<Fern>(2); });
    r.registerClass("MFern", [] { return std::make_unique<Fern>(1); });

    r.registerClass("SideChain", [] { return std::make_unique<SideChain>(2); });
    r.registerClass("SSideChain", [] { return std::make_unique<SideChain>(2); });
    r.registerClass("MSideChain", [] { return std::make_unique<SideChain>(1); });
    r.registerClass("SpectralFreeze", [] { return std::make_unique<SpectralFreeze>(2); });
    r.registerClass("SSpectralFreeze", [] { return std::make_unique<SpectralFreeze>(2); });
    r.registerClass("MSpectralFreeze", [] { return std::make_unique<SpectralFreeze>(1); });
    r.registerClass("SpectralMorph", [] { return std::make_unique<SpectralMorph>(2); });
    r.registerClass("SSpectralMorph", [] { return std::make_unique<SpectralMorph>(2); });
    r.registerClass("MSpectralMorph", [] { return std::make_unique<SpectralMorph>(1); });
    r.registerClass("SpectralFilter", [] { return std::make_unique<SpectralFilter>(2); });
    r.registerClass("SSpectralFilter", [] { return std::make_unique<SpectralFilter>(2); });
    r.registerClass("MSpectralFilter", [] { return std::make_unique<SpectralFilter>(1); });
    r.registerClass("StereoTool", [] { return std::make_unique<StereoTool>(); });
    r.registerClass("VuMeter", [] { return std::make_unique<VuMeter>(); });
    r.registerClass("Console", [] { return std::make_unique<Console>(6, 2); });
    for (int n : {2, 3, 4, 5, 6, 7, 8})
        r.registerClass("S" + std::to_string(n) + "Console",
                        [n] { return std::make_unique<Console>(n, 2); });
    r.registerClass("Button", [] { return std::make_unique<Button>(); });
    r.registerClass("Number", [] { return std::make_unique<Number>(); });
    r.registerClass("RNG", [] { return std::make_unique<RNG>(); });
    r.registerClass("Slider", [] { return std::make_unique<Slider>(); });
    r.registerClass("Gate", [] { return std::make_unique<Gate>(2); });
    r.registerClass("SGate", [] { return std::make_unique<Gate>(2); });
    r.registerClass("MGate", [] { return std::make_unique<Gate>(1); });
    r.registerClass("Cluster", [] { return std::make_unique<Cluster>(); });
    r.registerClass("Harmonizer", [] { return std::make_unique<Harmonizer>(); });
    r.registerClass("Paulstretch", [] { return std::make_unique<Paulstretch>(); });
    r.registerClass("PinkTrombone", [] { return std::make_unique<PinkTrombone>(); });
    r.registerClass("Math", [] { return std::make_unique<MathNode>(); });
    r.registerClass("pH", [] { return std::make_unique<Ph>(); });
    r.registerClass("Board", [] { return std::make_unique<Board>(); });
    r.registerClass("SerialIn", [] { return std::make_unique<SerialIn>(); });
    r.registerClass("SerialOut", [] { return std::make_unique<SerialOut>(); });
    r.registerClass("LFO", [] { return std::make_unique<LfoGen>(); });
    r.registerClass("Follower", [] { return std::make_unique<Follower>(); });
    r.registerClass("Filter",  [] { return std::make_unique<ValveFilter>(); });
    r.registerClass("SFilter", [] { return std::make_unique<ValveFilter>(); });
    r.registerClass("MFilter", [] { return std::make_unique<ValveFilter>(); });
    r.registerClass("Sig", [] { return std::make_unique<Sig>(); });
    r.registerClass("Notes", [] { return std::make_unique<Notes>(); });
    r.registerClass("Var", [] { return std::make_unique<Var>(); });

    r.registerClass("Kick",    [] { return std::make_unique<Kick>(); });
    r.registerClass("Cicada",  [] { return std::make_unique<Cicada>(); });
    r.registerClass("Bloom",   [] { return std::make_unique<Bloom>(); });
    r.registerClass("Spectrum", [] { return std::make_unique<Spectrum>(); });
    r.registerClass("Acid",    [] { return std::make_unique<Acid>(); });
    r.registerClass("Silt", [] { return std::make_unique<Silt>(); });
    r.registerClass("Grit", [] { return std::make_unique<Grit>(); });
    r.registerClass("Microdot", [] { return std::make_unique<Microdot>(); });
    r.registerClass("Riff",    [] { return std::make_unique<Riff>(); });
    r.registerClass("Steps",   [] { return std::make_unique<Steps>(); });
    r.registerClass("SideKick", [] { return std::make_unique<SideKick>(); });
    r.registerClass("Siren",   [] { return std::make_unique<Siren>(); });
    r.registerClass("Repeater", [] { return std::make_unique<Repeater>(); });

    r.registerClass("Compressor",  [] { return std::make_unique<Compressor>(2); });
    r.registerClass("SCompressor", [] { return std::make_unique<Compressor>(2); });
    r.registerClass("MCompressor", [] { return std::make_unique<Compressor>(1); });
    r.registerClass("Limiter",     [] { return std::make_unique<Limiter>(2); });
    r.registerClass("SLimiter",    [] { return std::make_unique<Limiter>(2); });
    r.registerClass("MLimiter",    [] { return std::make_unique<Limiter>(1); });
    r.registerClass("NoiseGate",   [] { return std::make_unique<NoiseGate>(2); });
    r.registerClass("SNoiseGate",  [] { return std::make_unique<NoiseGate>(2); });
    r.registerClass("MNoiseGate",  [] { return std::make_unique<NoiseGate>(1); });
    r.registerClass("ParaEQ",      [] { return std::make_unique<ParaEQ>(2); });
    r.registerClass("SParaEQ",     [] { return std::make_unique<ParaEQ>(2); });
    r.registerClass("MParaEQ",     [] { return std::make_unique<ParaEQ>(1); });
    r.registerClass("Chorus",      [] { return std::make_unique<SChorus>(); });
    r.registerClass("SChorus",     [] { return std::make_unique<SChorus>(); });
    r.registerClass("Flanger",     [] { return std::make_unique<Flanger>(); });
    r.registerClass("Phaser",      [] { return std::make_unique<Phaser>(); });
    r.registerClass("FrequencyShifter", [] { return std::make_unique<FrequencyShifter>(); });
    r.registerClass("TestGen",     [] { return std::make_unique<TestGen>(); });
}

}
