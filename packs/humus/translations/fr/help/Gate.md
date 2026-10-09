# Gate

Un gate audio ouvert par le niveau d'un signal, par des notes MIDI tenues ou par un interrupteur.

Le signal de la première entrée passe quand le gate est ouvert et tombe à Floor quand il est fermé. Source décide de ce qui l'ouvre : LEVEL écoute l'entrée key, ou le signal lui-même quand rien n'y est câblé, et ouvre au-dessus de Threshold ; MIDI ouvre tant qu'une note est tenue sur l'entrée MIDI ; SWITCH confie le gate à l'interrupteur Open. L'état d'ouverture lissé est publié comme valeur de contrôle "open", si bien que le même gate peut déclencher ou piloter n'importe quoi d'autre dans le patch. Câble un Kick dans l'entrée key et une nappe dans l'entrée du signal pour la hacher en sidechain, ou un PianoRoll dans l'entrée MIDI pour jouer du gate comme d'un instrument.

## Paramètres

**Source** Ce qui ouvre le gate : LEVEL, MIDI ou SWITCH.

**Detector** Comment LEVEL écoute. PEAK réagit aux variations instantanées, RMS au volume moyen.

**Threshold** Le niveau qui ouvre le gate en mode LEVEL.

**AttackTime** La vitesse à laquelle le gate s'ouvre, en millisecondes.

**HoldTime** Combien de temps le gate reste ouvert après la baisse de la source, en millisecondes.

**ReleaseTime** La lenteur avec laquelle le gate se ferme, en millisecondes.

**Floor** Combien de signal passe encore quand il est fermé. Zéro donne un gate franc ; monte-le pour un effet d'expandeur.

**Duck** Inverse ouvert et fermé, ce qui fait de n'importe laquelle des trois sources un ducking.

**Open** L'interrupteur manuel, utilisé quand Source vaut SWITCH. Câble dessus un Slider, un LFO ou un Follower pour basculer le gate par un contrôle.

## Recette

**Nappe hachée** Câble une nappe dans la première entrée et un Kick dans l'entrée key. Source LEVEL, Detector PEAK, Threshold 0.2, AttackTime 1, HoldTime 60, ReleaseTime 80, Floor 0. La nappe ne sonne plus que sur chaque coup de grosse caisse ; active Duck pour qu'elle disparaisse plutôt à chaque coup.

## Organisms associés

NoiseGate, SideChain, Gain, Follower
