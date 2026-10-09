# DigiRust

Ein Lo-Fi-Kanal, der einen billigen Wandler nachbildet, der kaputtgeht: weniger Bits, eine langsamere Clock, eine eiernde Clock und Rauschen.

Bits vergröbert das Signal auf weniger Stufen, und Rate hält jedes Sample länger, sodass die Höhen per Aliasing nach unten in Dreck umschlagen. Jitter lässt die Clock eiern, sodass die Haltezeiten von Sample zu Sample schwanken, und Noise fügt ein Rauschen hinzu, das auf dem Signal mitläuft und in der Stille verschwindet, damit ein ruhendes Patch still bleibt. Tone kippt das Ergebnis ins Dunkle oder Präsente, Mix mischt es für parallelen Dreck mit dem Direktsignal, und ein DC-Blocker hält den Ausgang mittig. Zufall würfelt Bits, Rate, Jitter, Noise und Tone neu, lässt aber Mix und Level in Ruhe, sodass ein Wurf die Farbe ändert, nie die Balance des Patches. Setz es hinter einen Sampler oder ein Drums-Organism, oder vor ein Fern für ein verschlissenes Echo.

## Parameter

**Bits** Wortbreite, 1 bis 16, in ganzen Schritten. Niedriger ist gröber.

**Rate** Abtastrate in Hz. Ganz oben ist es aus dem Signalweg; niedrig klingt es knusprig.

**Jitter** Eiern der Clock. Null ist eine stabile Clock; höher bringt Leiern und Verschmieren.

**Noise** Rauschen, dessen Pegel dem Signal folgt. Null ist still.

**Tone** Neigung von dunkel bis präsent, um die Mitte herum.

**Mix** Mischung vom Direktsignal bis zum verrosteten Signal.

**Level** Ausgangspegel.

## Rezept

**Alte Sampler-Drums** Bits 10, Rate 22000, Jitter 0.1, Noise 0.15, Tone 0.4, Mix 1. Schließ einen Sampler an, der einen Break spielt, und die Höhen stumpfen zum klassischen Crunch früher Sampler ab; zieh Rate für den Breakdown auf 8000 herunter.

## Verwandte Organisms

Sampler, Fern, Filter
