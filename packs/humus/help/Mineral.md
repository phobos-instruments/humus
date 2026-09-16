# Mineral

A struck percussion synth whose partials sit on a geometric lattice, each one the previous one times Ratio.

That series is the inharmonic spectrum of struck glass, bells and gamelan metal, and the shape of the lattice is the timbre. Four voices play from the MIDI inlet, so sequence it from a PianoRoll or a DNA, or play it from the keyboard while its editor is focused. The sigil on the left of the editor draws the model live: one side per facet, stretched by Ratio, doubled by Shimmer, flashing on a strike. Cord the outlet into a Verbatim or a Fern for the space a bell wants.

## Parameters

**Facets** How many partials, 1 to 12. Few is a struck bar; many fills out into a bell.

**Ratio** The lattice constant. 2.0 is octaves, 1.62 the classic bell inharmonicity, lower is clustered and metallic.

**Geometry** Hands Ratio to the facet count: the diagonal of a regular polygon over its side, so five facets give 1.618 and more facets climb towards the octave. The Ratio knob dims while it is on.

**Shine** Level slope across the facets, dull to glassy.

**Decay** Ring time in milliseconds. Higher facets always die faster.

**Strike** The contact transient, a short burst of noise.

**Shimmer** Detunes each facet slightly differently left and right, so the sound beats across the stereo field.

**Level** Output level.

**BendRange** How far the pitch wheel reaches at full travel, in semitones. Two is the common default; zero ignores the wheel.

## Recipe

**Gamelan** Facets 6, Ratio 1.62, Shine 0.75, Decay 1200, Strike 0.5, Shimmer 0.4. Cord a DNA into the MIDI inlet for a pentatonic line and the outlet into a Verbatim with a long tail.

## Related Organisms

DNA, Rhizome, PianoRoll
