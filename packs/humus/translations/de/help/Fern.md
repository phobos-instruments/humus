# Fern

Ein tempo-synchronisierbares Stereo-Echo, dessen Wiederholungen beim Ausklingen dünner, dunkler und wärmer werden.

Body, Damp und Drive sitzen in der Feedback-Schleife, also wirken sie bei jedem Durchlauf erneut: Die erste Wiederholung wird einmal geformt, die fünfte fünfmal. Deshalb klingt eine Fahne, als würde sie sich entfernen, statt kopiert zu werden. Oberhalb von drei Vierteln Feedback hält es eine Note beliebig lange, und der Lesepunkt bewegt sich mit jedem Sample, also verbiegt das Drehen an Time von Hand die Tonhöhe, statt zu klicken.

## Parameter

**Mode** Stereo hält jede Seite für sich. Ping-pong kreuzt das Feedback, sodass jede Wiederholung auf der jeweils anderen Seite landet.

**Time** Delay-Zeit in Millisekunden. Wird ignoriert, solange Sync an ist.

**Sync** Koppelt die Zeit an den Transport statt an Millisekunden.

**SyncMultiplier** Wie viele SyncUnit-Einheiten eine Wiederholung ergeben, solange Sync an ist.

**SyncUnit** Der Notenwert, in dem die synchronisierte Zeit zählt, etwa 1/8 oder 1/16.

**Feedback** Wie viel von jeder Wiederholung zur nächsten wird. Niedrig ist ein Slap, hoch ist eine Fahne, die die Note überdauert.

**Spread** Zieht die Zeit der rechten Seite von der linken weg und öffnet das Echo über das Stereobild. Null hält beide Seiten zusammen.

**Body** Dünnt jede Wiederholung von unten aus, damit eine Fahne dem Bass aus dem Weg geht, statt sich unter ihm aufzutürmen.

**Damp** Dunkelt jede Wiederholung von oben ab. Das ist der Regler, der aus einem sauberen digitalen Echo einen Raum oder ein Band macht.

**Drive** Wärmt jede Wiederholung an. Bei null ist die Schleife sauber; höher wird die Fahne beim Ausklingen dichter.

**Glide** Was passiert, wenn sich Time ändert. Tape verschiebt den Lesepunkt gleitend, sodass sich die Wiederholungen unterwegs in der Tonhöhe verbiegen. Fade blendet die alten Wiederholungen in einen neuen Satz bei der neuen Zeit über, ohne Tonhöhenänderung; nimm es, wenn Time dem Transport folgt.

**Mix** Wie viel Echo gegenüber dem Direktsignal zu hören ist.

## Rezept

**Dub-Fahne** Sync an, SyncUnit 1/8 mit einem Multiplikator von 3 für ein punktiertes Feeling, Feedback um 0.7, Damp 0.6, Drive 0.3, Mode Ping-pong. Schick einen kurzen Stab hinein und lass die Fahne den Rhythmus tragen; fahr Feedback für eine Steigerung Richtung Maximum und wieder runter, um aufzuräumen.

## Verwandte Organisms

Repeater, SoundSpace, Trellis
