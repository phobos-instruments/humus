# Graft

Ein Sample-Stitcher, der bis zu acht Audiodateien in Fragmente schneidet und pro Slot ein Fragment zu einem neuen Sample aneinanderreiht.

Lade Audiodateien in die Slots, wähl, wie sie geschnitten werden und wie die Slots ihre Fragmente aussuchen, und das zusammengenähte Ergebnis ist das Instrument: Es spielt vom MIDI-Eingang, mit C4 in seiner eigenen Geschwindigkeit, oder dröhnt mit Play für sich allein. Jeder Schnitt rückt zum nächsten Nulldurchgang, und benachbarte Fragmente überlappen mit einer Equal-Power-Überblendung, sodass eine Naht zwischen fremden Aufnahmen ohne Klick durchgeht. Derselbe Seed baut immer dasselbe Ergebnis, sodass ein gespeichertes Patch so wieder aufgeht, wie es war, und Zufall würfelt ein neues, ohne die anderen Einstellungen anzufassen. Verbinde eine PianoRoll oder ein DNA mit dem MIDI-Eingang, oder schalte Play ein und verbinde den Ausgang mit einem Fern oder einem Filter.

## Die Karte

Jedes Band ist nach der Datei gefärbt, aus der es stammt, passend zum Slot darüber. Klick ein Band an, um nur dieses Fragment zu hören. Zieh nach oben oder unten, um es umzustimmen, zieh seitwärts, um durch die anderen Fragmente derselben Datei zu blättern, und halte beim seitlichen Ziehen Shift, um stattdessen durch die Dateien zu blättern. Klick mit rechts, um den Slot rückwärts abzuspielen oder stummzuschalten, ihn allein neu zu würfeln oder ihn zu behalten. Ein behaltener Slot hält sein Fragment, während alles um ihn herum neu gewürfelt wird.

## Parameter

**File1** Eine Quelldatei zum Zerschneiden, und so weiter für File2 bis File8. Jede Audiodatei geht.

**Slices** Wie viele Slots das Ergebnis hat, 4 bis 64.

**Order** Wie die Slots wählen. Random nimmt ein beliebiges Fragment aus einer beliebigen Datei; In order und Round robin gehen Slot für Slot durch die geladenen Dateien; Blocks gibt jeder Datei einen zusammenhängenden Abschnitt; Mirror baut die erste Hälfte zufällig und faltet sie rückwärts zurück; One source ordnet nur die Datei neu, auf die Only zeigt.

**Cut** Woher die Fragmente kommen. Free gibt jeder Datei so viele Stücke, wie es Slots gibt, Grid schneidet auf einen Notenwert des Transport-Tempos, Onsets schneidet an Transienten.

**Div** Die Fragmentlänge im Grid-Modus, als Notenwert.

**Length** Im Free-Modus, wie viel von jedem natürlichen Fragment genommen wird, 10 bis 200 Prozent. Kurz stottert, lang überlappt.

**Sense** Im Onsets-Modus, wie viele Transienten als Schnitt zählen.

**Xfade** Die Überlappung an jeder Naht, in Millisekunden.

**Seed** Angezeigt als Creature. Dieselbe Zahl baut immer dasselbe Ergebnis.

**Bars** 0 lässt das Ergebnis so lang, wie es herauskommt. Jede andere Zahl resampelt es so, dass es beim aktuellen Tempo genau so viele Takte füllt, was die Tonhöhe um bis zu eine Oktave verschiebt; die Statuszeile sagt, um wie viel.

**Normalize** Hebt das fertige Ergebnis bis knapp unter Vollaussteuerung an.

**Source** Angezeigt als Only: welche Datei One source nimmt.

**Play** Lässt das Ergebnis ohne Note dauerhaft dröhnen. Änderungen werden darunter ohne Unterbrechung neu gebaut.

**Attack** Attack der Hüllkurve in Millisekunden, für jede Note.

**Decay** Decay der Hüllkurve in Millisekunden.

**Sustain** Sustain-Pegel der Hüllkurve.

**Release** Release der Hüllkurve in Millisekunden.

**Loop** Eine gehaltene Note läuft im Kreis, statt am Ende aufzuhören.

**Reverse** Spielt vom Ende her.

**Speed** Abspielrate, vom Viertel bis zum Vierfachen, ohne die Tastatur zu verschieben.

**Start** Wo im Ergebnis eine Note beginnt. Zieh am Streifen unter der Karte, um den Punkt zu setzen.

**Level** Ausgangspegel.

**BendRange** Wie weit das Pitch-Rad bei vollem Ausschlag reicht, in Halbtönen. Zwei ist der übliche Standard; null ignoriert das Rad.

**Voices** Wie viele Noten gleichzeitig klingen können, bis zu 16.

**SliceEdits** Die Umstimmungen, Umkehrungen und Stummschaltungen pro Slot, die du auf der Karte gemacht hast, mit dem Patch gespeichert.

**Pins** Die behaltenen Slots, mit dem Patch gespeichert.

## Rezept

**Genähter Break** Lade einen Drumloop, eine Gesangsaufnahme und eine Feldaufnahme in drei Slots. Cut Onsets, Sense 0.5, Slices 16, Order Random, Bars 2, Play an. Drück Zufall, bis ein Slot gut sitzt, klick ihn mit rechts an und behalte ihn, dann würfel neu, bis der Rest passt. Schick den Ausgang durch ein Fern.

## Verwandte Organisms

Leafcutter, Sampler, Paulstretch, PianoRoll
