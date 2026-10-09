# Cluster

Huit emplacements d'accords qui partent comme des voicings entiers par une sortie MIDI.

Chaque emplacement contient quelques notes tapées par leur nom, comme C3 E3 G3 ; les dièses, les bémols et les simples numéros de note sont acceptés. Un emplacement se déclenche quand tu appuies sur son bouton, que tu joues sa note de déclenchement ou que tu pilotes son paramètre Fire depuis un routage de contrôle. En mode PADS, les huit emplacements sont posés sur huit notes entrantes à partir de TriggerNote et sonnent à la vélocité de ton jeu. En mode FOLLOW, un emplacement devient une forme qui suit ton jeu : chaque note entrante devient la fondamentale et l'accord se déplace avec elle. Les notes partagées entre deux emplacements qui sonnent sont comptées, donc relâcher un pad ne coupe jamais une note qu'un autre tient encore. Branche la sortie dans un Rhizome, un Wave ou n'importe quel autre instrument.

## Paramètres

**Mode** PADS joue les huit emplacements depuis huit touches. FOLLOW transpose un emplacement d'après ce que tu joues.

**Hold** Ce que veut dire un appui. GATE fait sonner l'accord tant que le déclencheur est enfoncé. LATCH fait de chaque déclenchement un interrupteur, pour que les pads puissent se superposer. PEDAL joue un seul accord à la fois : chaque appui remplace le précédent et les relâchements sont ignorés, donc un emplacement vide devient un bouton stop.

**TriggerNote** La note entrante assignée à l'emplacement 1. Les sept notes suivantes vers l'aigu sont assignées aux emplacements 2 à 8.

**Slot** L'emplacement que le mode FOLLOW déplace.

**Velocity** La vélocité utilisée quand un emplacement est déclenché depuis son bouton ou un routage de contrôle.

**Notes1** Les notes de l'emplacement 1, tapées en noms ou en numéros. Et ainsi de suite de 2 à 8.

**Fire1** Le déclencheur derrière le bouton de l'emplacement 1, accessible aux routages de contrôle et à l'automation. Et ainsi de suite de 2 à 8.

## Recette

**Changements à un doigt** Mode PADS, Hold PEDAL, TriggerNote 36. Tape une progression dans les emplacements 1 à 4 et laisse l'emplacement 5 vide. Branche un PianoRoll dans l'entrée et Cluster dans un Rhizome, puis dessine des notes seules de 36 à 39 là où chaque accord doit changer, et un 40 là où il doit s'arrêter.

## Organisms associés

PianoRoll, Steps, Riff, Harmonizer
