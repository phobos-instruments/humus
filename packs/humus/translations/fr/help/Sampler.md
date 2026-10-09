# Sampler

Un lecteur d'échantillons polyphonique à zones, avec huit emplacements, le chargement de banques de sons et l'enregistrement depuis le patch.

Charge jusqu'à huit fichiers son, donne à chacun une fondamentale et joue-les en MIDI depuis un MidiIn, un PianoRoll ou le clavier à l'écran. Ou charge un fichier de banque de sons entier (.sf2, ou les bibliothèques d'échantillons .nsmp3 et .nsmp4 d'un clavier de scène) et joue ses instruments ; une banque chargée devient l'instrument, et les emplacements ne jouent que quand aucune banque n'est chargée. Branche un signal sur l'entrée audio et Record capture une prise directement dans un emplacement, sous forme de fichier son enregistré avec le patch. Il démarre grossier, douze bits à 32 kHz, dans la tradition du sampler en rack de la fin des années quatre-vingt ; le rail de presets va d'Ancient à Pristine, en ne bougeant que Bits et Rate. La vélocité règle le niveau, et au-delà de Polyphony la voix la plus ancienne est volée.

## Emplacements et banques

En mode Pitched, les zones se partagent le clavier aux points médians entre leurs fondamentales, et les notes sont transposées par rééchantillonnage depuis la fondamentale la plus proche ; une seule zone couvre tout le clavier. En mode Kit, chaque zone ne joue que sur la touche de sa fondamentale, à vitesse native, pour des one-shots sur des touches séparées. Une banque apporte ses propres répartitions de touches, points de boucle et enveloppes, et les boutons rotatifs d'enveloppe les mettent alors à l'échelle au lieu de les remplacer. Les prises arrivent dans un dossier samples sous Documents.

## Paramètres

**Mode** Pitched répartit les zones sur le clavier selon leur fondamentale. Kit joue chaque zone uniquement sur la touche de sa fondamentale.

**Gain** Niveau de sortie.

**BendRange** Jusqu'où va la molette de pitch bend en fin de course, en demi-tons. Deux, c'est la valeur courante par défaut ; zéro ignore la molette.

**Attack** Temps de montée de chaque note en millisecondes. Avec une banque chargée, il met à l'échelle l'attaque propre de la zone, donc la valeur par défaut joue la banque telle qu'elle est écrite.

**Decay** Temps de descente jusqu'au niveau Sustain en millisecondes, mis à l'échelle de la même façon avec une banque.

**Sustain** Le niveau tenu, de 0 à 1.

**Release** Temps de fondu après le relâchement de la touche, en millisecondes.

**Polyphony** Le nombre de voix qui sonnent en même temps. Au-delà, la voix la plus ancienne est volée.

**Loop** Activé, il tient une note sur les points de boucle qu'apporte le fichier, tant que tu gardes la touche enfoncée ; un fichier qui n'en a pas est lu jusqu'au bout dans tous les cas. Désactivé, il ignore tout point de boucle et laisse chaque note aller jusqu'à la fin de son échantillon : c'est le réglage à choisir quand les boucles d'une bibliothèque sont mal placées. Le raccord se fait en fondu enchaîné, donc une boucle dont les deux extrémités ne s'alignent pas tient au lieu de cliquer à chaque cycle.

**File1** Le fichier son de l'emplacement 1, et ainsi de suite pour File2 à File8.

**Root1** La note MIDI que l'emplacement 1 joue à sa hauteur native, 60 par défaut, et ainsi de suite pour Root2 à Root8. Un fichier qui indique sa propre fondamentale la règle au chargement ; pour un kit, règle chaque fondamentale sur la touche où tu veux l'entendre.

**FileBank** Un fichier de banque de sons. Tant qu'il est chargé, c'est lui qui joue.

**Preset** Quel instrument de la banque répond, listé par nom.

**RecSlot** L'emplacement où arrive une prise.

**RecordDuration** Arrête une prise toute seule après ce nombre de millisecondes. À 0, une prise continue jusqu'à ce que tu l'arrêtes ou jusqu'au plafond de trente secondes.

**Record** Lance une prise depuis l'entrée audio. Appuie à nouveau pour l'arrêter.

**Bits** Longueur de mot de l'échantillon, de 8 à 24. En dessous de 24, le fichier est stocké grossièrement, comme le faisaient les premiers samplers matériels, avec du dither pour que la grossièreté devienne un plancher stable au lieu d'un bourdonnement qui suit la note vers le bas. Le changer relit le fichier. Les flèches parcourent les résolutions standard, 8, 12, 16 et 24 ; tape n'importe quel autre nombre dans la case.

**Rate** Fréquence d'échantillonnage en kHz, de 4 à 48. Le fichier est filtré pour la nouvelle fréquence avant d'être décimé, comme le faisait le convertisseur à l'entrée d'un sampler en rack, donc ce qui se trouvait au-dessus du nouveau plafond se tait au lieu de se replier en résonance métallique. À 48, le fichier reste intact. Les flèches parcourent les fréquences standard, 4, 8, 11, 16, 22, 32, 44 et 48 ; tape n'importe quel autre nombre dans la case.

**Monitor** Fait passer l'entrée vers les sorties pour que tu entendes ce que tu vas enregistrer. Désactivé par défaut, pour qu'un Sampler déjà branché sur une table de mixage ne double pas le signal.

**Threshold** Retient une prise armée jusqu'à ce que l'entrée atteigne ce niveau, dessiné comme la marque au-dessus du vumètre. Tout à gauche, il enregistre dès que tu armes.

## Recette

**Kit depuis le patch** Mode Kit, Root1 36, Root2 38, Root3 42. Branche un Kick sur l'entrée audio, RecSlot 1, Threshold 0.05, Record activé, et déclenche le Kick une fois ; recommence pour les autres emplacements avec d'autres sources. Branche un Riff ou un PianoRoll sur l'entrée MIDI et joue le kit.

## Organisms associés

PianoRoll, MidiIn
