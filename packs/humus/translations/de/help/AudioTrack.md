# AudioTrack

Der Audiokanal des Aufnahme-Sequencers: eine Zeile auf der Timeline, die ihre Clips zum Transport abspielt und Takes von allem aufnimmt, was hineingekabelt ist.

Es sitzt inline im Signalweg, Stereo rein und Stereo raus, also kommt es hinter die Quelle, die du aufnehmen willst. Was es verlässt, sind seine Clips, sein Eingang oder beides, je nach Monitor. Ein Take landet als Clip in der Zeile der Spur, und ein zweiter Durchgang über dieselben Takte landet über dem ersten, sodass nichts verloren geht. Eine Audiodatei, die du auf die Timeline ziehst, landet ebenfalls auf einem AudioTrack; liegt unter dem Cursor keine Zeile, wird eine neue angelegt und mit dem Master verkabelt. Setz es vor einen Mixer oder direkt vor ein SoundOut.

## Parameter

**Record** Schaltet die Spur scharf. Aufnehmen im Transport steuert das, deshalb wird sie normalerweise über den Transport scharf geschaltet und nicht von Hand.

**Monitor** Was an den Ausgängen ankommt. In gibt den Eingang immer durch und fügt beim Abspielen die Clips hinzu. Auto gibt den Eingang durch, solange die Spur scharf ist oder der Transport steht, und die Clips, wenn die Spur nicht scharf ist und abspielt; das ist für fast jede Aufnahme richtig. Off spielt nur die Clips.

**Gain** Pegel des Spurausgangs.

**Mute** Schaltet den Ausgang stumm. Da die Spur inline sitzt, wird damit auch alles stumm, was durch sie hindurchläuft.

## Rezept

**Einen Synth aufnehmen** Verbinde ein Rhizome mit dem AudioTrack und den AudioTrack mit dem Master. Lass Monitor auf Auto, schalte über den Transport scharf, drück Wiedergabe und spiel den Part. Stoppe, und der Take ist ein Clip in der Zeile, den du wie jeden anderen verschieben und trimmen kannst.

## Verwandte Organisms

Deck, Sampler, PianoRoll, Sequence
