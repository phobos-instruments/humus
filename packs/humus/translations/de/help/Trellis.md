# Trellis

Ein monophoner Tonhöhenkorrektor, der eine Gesangsstimme oder Lead-Linie auf die Noten einer Tonart und Tonleiter zieht.

Es verfolgt die eingehende Grundfrequenz, rastet sie auf die nächste erlaubte Note von Key und Scale ein und verschiebt das Audio dorthin, mit einem überblendenden Pitchshifter auf Delay-Leitungen, eine Stimme nach der anderen. Speed bestimmt, wie schnell das Gleiten zum Ziel ist: Langsam bewahrt die natürliche Drift, schnell ist der Hard-Tune-Effekt. Das Notenraster kommt aus der Stimmung des Patches, sodass ein Tuning-Organism auf 19-EDO die Tonleitermasken über 19 Stufen statt 12 legt. Der Effektweg hat eine kurze Verzögerung. Schließ eine Mono-Gesangsstimme von einem SoundIn oder einem Decomposer an.

## Parameter

**Strength** Wie weit die Tonhöhe zum Ziel gezogen wird. 0 lässt die Performance in Ruhe; 1 rastet sie ganz aufs Raster ein.

**Speed** Wie schnell die Tonhöhe zum Ziel gleitet. Niedrig ist eine unauffällige Korrektur; nahe am Maximum ist es der Roboter-Sprung.

**Key** Der Grundton, C bis B. Klick auf die Box und drück eine Taste, auf einem MIDI-Keyboard oder auf der Klaviatur, die sich öffnet; jede Oktave setzt denselben Grundton.

**Scale** Die Noten, auf denen die Tonhöhe landen darf: Chromatic, Major, Minor, die Modi, Harmonic und Melodic Minor, beide Pentatoniken, Blues, Whole Tone und Hirajoshi.

**Mix** Wie viel korrigiertes Signal gegenüber dem Direktsignal zu hören ist.

## Rezept

**Natürliche Korrektur** Key auf die Tonart des Songs, Scale Major oder Minor, Strength 0.7, Speed 0.4, Mix 1. Für den Effekt stattdessen: Scale Chromatic, Strength 1, Speed 1. Führe den Ausgang in ein Verbatim oder ein Fern, damit die Korrektur in einem Raum sitzt.

## Verwandte Organisms

Decomposer, Tuning
