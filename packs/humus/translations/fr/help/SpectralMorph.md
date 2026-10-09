# SpectralMorph

Fond les spectres de deux entrées, pour qu'un timbre passe à travers l'autre au lieu de jouer par-dessus.

Pour chaque bande, le niveau est interpolé entre la source A et la source B, la phase est prise à celle qui contribue le plus à cet endroit, puis le résultat est resynthétisé. En mode Stereo, les entrées 1 et 2 sont A et les entrées 3 et 4 sont B ; en mode Mono, l'entrée 1 est A et l'entrée 2 est B, alors revérifie les câbles après avoir changé Mode. Si B n'est pas connecté, Morph devient un fondu vers le silence. Les sources tenues et riches en harmoniques se fondent le mieux : câble un Substrate face à une voix dans un Sampler et balaie Morph depuis un LFO ou un Metapad.

## Paramètres

**Morph** 0, c'est A pur, 1, c'est B pur, et tout ce qui est entre les deux est un mélange spectral des deux.

**A** Maintiens-le pour passer directement sur A, où que soit Morph ; lâche et le mélange revient. Assigne-le à un pad pour une coupure spectrale.

**B** Maintiens-le pour passer directement sur B. Maintenir les deux te place à mi-chemin.

## Recette

**Voix vers nappe** Câble une voyelle chantée depuis SoundIn sur les entrées 1 et 2 et un accord de Substrate sur les entrées 3 et 4. Câble un LFO sur Morph avec une période de huit mesures, pour que la voix se change en nappe puis revienne. Garde les deux sources en train de jouer pendant tout le balayage.

## Organisms associés

SpectralFilter, SpectralFreeze, SoundSpace, Mixer
