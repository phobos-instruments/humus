# Gate

Ein Audio-Gate, das von einem Signalpegel, von gehaltenen MIDI-Noten oder von einem Schalter geöffnet wird.

Das Signal am ersten Eingang geht durch, wenn das Gate offen ist, und fällt auf Floor, wenn es zu ist. Source entscheidet, was es öffnet: LEVEL hört auf den Key-Eingang, oder auf das Signal selbst, wenn dort kein Kabel steckt, und öffnet über Threshold; MIDI öffnet, solange am MIDI-Eingang irgendeine Note gehalten wird; SWITCH übergibt das Gate an den Schalter Open. Der geglättete Offen-Zustand wird als Steuerwert "open" bereitgestellt, sodass dasselbe Gate alles andere im Patch auslösen oder steuern kann. Verkable einen Kick mit dem Key-Eingang und ein Pad mit dem Signaleingang, um per Sidechain zu zerhacken, oder eine PianoRoll mit dem MIDI-Eingang, um das Gate wie ein Instrument zu spielen.

## Parameter

**Source** Was das Gate öffnet: LEVEL, MIDI oder SWITCH.

**Detector** Wie LEVEL hinhört. PEAK reagiert auf momentane Ausschläge, RMS auf die durchschnittliche Lautheit.

**Threshold** Der Pegel, der das Gate im LEVEL-Modus öffnet.

**AttackTime** Wie schnell das Gate öffnet, in Millisekunden.

**HoldTime** Wie lange das Gate offen bleibt, nachdem die Quelle abfällt, in Millisekunden.

**ReleaseTime** Wie langsam das Gate schließt, in Millisekunden.

**Floor** Wie viel Signal noch durchsickert, wenn es zu ist. Null ist ein hartes Gate; dreh es auf für ein Expander-Gefühl.

**Duck** Tauscht offen und zu, was jede der drei Quellen in einen Ducker verwandelt.

**Open** Der manuelle Schalter, verwendet, wenn Source auf SWITCH steht. Verkable einen Slider, LFO oder Follower damit, um das Gate per Steuersignal umzuschalten.

## Rezept

**Zerhacktes Pad** Verkable ein Pad mit dem ersten Eingang und einen Kick mit dem Key-Eingang. Source LEVEL, Detector PEAK, Threshold 0.2, AttackTime 1, HoldTime 60, ReleaseTime 80, Floor 0. Das Pad erklingt jetzt nur bei jedem Kick; schalte Duck an, damit es stattdessen bei jedem Kick wegfällt.

## Verwandte Organisms

NoiseGate, SideChain, Gain, Follower
