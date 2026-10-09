# Mixer

Summiert mehrere Signale zu einem, mit Gain, Stummschaltung und Solo pro Eingang sowie Master-Gain und Master-Stummschaltung.

Verbinde deine Quellen mit den nummerierten Eingängen und den Ausgang weiter mit einem SoundOut oder einem Bus. Das Dropdown Inputs oben legt fest, wie viele Eingänge er hat, 2 bis 8, und Mode wählt Stereo-Paare, Mono-Kanäle oder Pan-Eingänge; ändert sich eins davon, wird der Mixer an Ort und Stelle umgebaut und behält seinen Namen und seine Kabel. Im Modus Stereo dient der linke Eingang eines Paars auch als Mono-Buchse: Ein Kabel in die linke Seite speist beide Seiten des Mixes, sodass eine Mono-Quelle in der Mitte sitzt, und ein zusätzliches Kabel in die rechte Seite stellt echtes Stereo wieder her. Der Modus Pan gibt jedem Mono-Eingang seinen eigenen Pan-Regler, mit einer Balance-Kennlinie, die die Mitte auf vollem Pegel lässt und nur die Gegenseite absenkt.

## Parameter

**MasterGain** Ausgangspegel nach den Eingangs-Gains, bis zum Doppelten von Unity.

**MasterMute** Schaltet den ganzen Mix stumm.

**Gain_1-2** Pegel des Eingangspaars 1-2, und so weiter für 3-4 bis 7-8. Ganz oben ist Unity; die Pegelanzeige des Kanalzugs zeigt, was davor ankommt.

**Mute_1-2** Schaltet Eingang 1-2 ohne Klicken stumm, und so weiter für die anderen Paare. Ein stummgeschalteter Eingang bleibt still, auch wenn er auf Solo steht.

**Solo_1-2** Lässt Eingang 1-2 allein hören, und so weiter für die anderen Paare. Solange irgendein Eingang auf Solo steht, sind nur die Solo-Eingänge zu hören.

## Rezept

**Mix aus vier Quellen** Stell Inputs auf 4 und Mode auf Stereo, verbinde ein Drum-Organism, einen Bass, einen Synth und einen Fern-Return mit den Paaren 1-2 bis 7-8 und den Ausgang mit dem SoundOut. Zieh jeden Gain herunter, bis die Pegelanzeige des SoundOuts im lautesten Takt etwas Headroom lässt, schalte nacheinander jeden Eingang auf Solo, um jeden Part zu prüfen, und stell den Gesamtpegel mit MasterGain ein.

## Verwandte Organisms

Bus, Gain, Console
