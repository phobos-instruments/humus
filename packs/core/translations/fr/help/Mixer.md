# Mixer

Additionne plusieurs signaux en un seul, avec Gain, Mute et Solo par entrée, plus un gain et un mute généraux.

Relie tes sources aux entrées numérotées et la sortie à un SoundOut ou à un Bus. Le menu déroulant Inputs, en haut, fixe le nombre d'entrées, de 2 à 8, et Mode choisit entre Stereo pairs, Mono channels ou Pan inputs ; changer l'un ou l'autre redimensionne le mixer sur place, en gardant son nom et ses câbles. En mode Stereo, l'entrée gauche d'une paire sert aussi de jack mono : un seul câble à gauche alimente les deux côtés du mix, si bien qu'une source mono se place au centre, et relier aussi le côté droit rétablit une vraie stéréo. Le mode Pan donne à chaque entrée mono son propre bouton rotatif Pan, avec une loi de balance qui laisse le centre à plein niveau et ne fait que baisser le côté opposé.

## Paramètres

**MasterGain** Niveau de sortie après les gains d'entrée, jusqu'à deux fois le gain unitaire.

**MasterMute** Coupe tout le mix.

**Gain_1-2** Niveau de la paire d'entrées 1-2, et ainsi de suite pour 3-4 jusqu'à 7-8. Gain unitaire tout en haut ; le vumètre de la tranche montre ce qui arrive avant lui.

**Mute_1-2** Coupe l'entrée 1-2 sans clic, et ainsi de suite pour les autres paires. Une entrée coupée reste silencieuse même en solo.

**Solo_1-2** Fait entendre l'entrée 1-2 seule, et ainsi de suite pour les autres paires. Tant qu'une entrée est en solo, on n'entend que les entrées en solo.

## Recette

**Mix à quatre sources** Règle Inputs sur 4 et Mode sur Stereo, relie un organism de batterie, une basse, un synthé et un retour de Fern aux paires 1-2 à 7-8, et relie la sortie au SoundOut. Baisse chaque Gain jusqu'à ce que le vumètre du SoundOut garde un peu de marge sur la mesure la plus forte, mets une entrée à la fois en solo pour vérifier chaque partie, et règle le niveau général avec MasterGain.

## Organisms associés

Bus, Gain, Console
