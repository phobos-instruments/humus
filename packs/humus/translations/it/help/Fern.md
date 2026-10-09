# Fern

Un'eco stereo sincronizzabile al tempo, con ripetizioni che si assottigliano, si scuriscono e si scaldano mentre decadono.

Body, Damp e Drive stanno dentro il loop di feedback, quindi ogni passaggio nel loop li applica di nuovo: la prima ripetizione viene modellata una volta, la quinta cinque volte. È questo che fa sembrare che la coda si allontani, invece di essere copiata. Oltre i tre quarti di Feedback tiene una nota quanto vuoi, e il punto di lettura si muove a ogni campione, così girare Time a mano piega il pitch invece di fare un clic.

## Parametri

**Mode** Stereo tiene ogni lato per sé. Ping-pong incrocia il feedback, così ogni ripetizione cade sul lato opposto rispetto alla precedente.

**Time** Tempo di ritardo in millisecondi. Ignorato mentre Sync è attivo.

**Sync** Aggancia il tempo di ritardo al transport invece che ai millisecondi.

**SyncMultiplier** Quante unità di SyncUnit fanno una ripetizione mentre Sync è attivo.

**SyncUnit** Il valore di nota in cui conta il tempo sincronizzato, per esempio 1/8 o 1/16.

**Feedback** Quanto di ogni ripetizione diventa la successiva. Basso è uno slap, alto è una coda che sopravvive alla nota.

**Spread** Allontana il tempo del lato destro da quello del sinistro, aprendo l'eco nell'immagine stereo. Zero tiene insieme i due lati.

**Body** Assottiglia ogni ripetizione togliendo le basse, così la coda lascia spazio al basso invece di accumularsi sotto di lui.

**Damp** Scurisce ogni ripetizione togliendo le alte. È il controllo che trasforma un'eco digitale pulita in una stanza o in un nastro.

**Drive** Scalda ogni ripetizione. A zero il loop è pulito; più su, la coda si ispessisce mentre decade.

**Glide** Cosa succede quando Time cambia. Tape fa scivolare il punto di lettura, così nel frattempo le ripetizioni si piegano di pitch. Fade fa un crossfade dalle vecchie ripetizioni a una nuova serie al nuovo tempo, senza cambi di pitch; usalo quando Time segue il transport.

**Mix** Quanta eco si sente rispetto al segnale secco.

## Ricetta

**Coda dub** Sync attivo, SyncUnit 1/8 con un moltiplicatore di 3 per un andamento puntato, Feedback intorno a 0.7, Damp 0.6, Drive 0.3, Mode Ping-pong. Manda dentro uno stab breve e lascia che la coda porti il ritmo; spingi Feedback verso l'alto per una salita e riportalo giù per ripulire.

## Organism correlati

Repeater, SoundSpace, Trellis
