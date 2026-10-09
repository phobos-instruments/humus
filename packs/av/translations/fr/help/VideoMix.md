# VideoMix

Un crossfader A/B pour deux flux vidéo, ou un mixeur de canaux pour huit au plus.

Inputs règle le nombre de flux qu'il reçoit. À 2, c'est un crossfader : l'entrée 1 est A, l'entrée 2 est B, et la sortie porte le fondu enchaîné. Fade à 0 montre A, à 1 montre B, et n'importe où entre les deux mélange les deux selon la loi que fixe Curve. Fade est un paramètre ordinaire : assigne-lui un contrôleur pour avoir un crossfader matériel, mets dessus un LFO synchronisé pour que les deux flux échangent leur place tous les seize temps, ou dessine-le dans un couloir d'automation pour que la liste des coupes fasse partie du morceau. Fais un fondu entre deux compositions Lumen entières avant le VideoOut, ou choisis plus d'entrées pour un mixeur de canaux.

À partir de 3 entrées, il devient un mixeur de canaux avec un fader vertical par entrée. Les canaux s'empilent dans l'ordre, le canal 1 en bas, et chaque fader règle à quel point ce canal recouvre ceux du dessous : en haut il les masque, à mi-course il est à moitié transparent, en bas il s'efface. Le canal 1 démarre en haut et les autres en bas, donc monter un fader fait entrer son flux par-dessus l'image. Les câbles restent sur leurs entrées quand tu changes le nombre ; un câble sur une entrée que la nouvelle taille n'a pas passe sur une entrée libre ou est retiré.

## Paramètres

**Inputs** Le nombre de flux, de 2 à 8. Deux, c'est le crossfader décrit ci-dessous ; au-delà, c'est le mixeur de canaux.

**Level 1 to 8** Les faders du mixeur de canaux, un par entrée : à quel point ce canal recouvre les canaux du dessous.

**Fade** Le fondu enchaîné. 0 est entièrement A (entrée 1), 1 est entièrement B (entrée 2), et entre les deux, c'est un mélange façonné par Curve.

**Curve** La loi du fondu du crossfader, le même bouton rotatif que sur Crossfader. À 0, elle est linéaire, donc à mi-chemin les deux images sont à moitié de leur luminosité et l'image s'assombrit. Plus haut, chaque côté reste lumineux plus loin dans le fondu ; deux tiers donne la loi à puissance constante, et 1 la version dure.

## Recette

**Échange calé sur la mesure** Branche un VideoPlayer sur l'entrée 1, un CameraIn sur l'entrée 2 et la sortie sur un VideoOut. Règle Curve sur deux tiers et ajoute un LFO, puis fais un clic droit sur Fade, choisis Contrôler avec et sélectionne le LFO, et règle le LFO en synchro sur 8 temps avec une onde carrée pour que les flux s'échangent pile sur la mesure.

**Quatre flux, un écran** Règle Inputs sur 4 et branche un CameraIn, deux VideoPlayers et un VideoPad sur les entrées 1 à 4. Laisse le canal 1 en haut comme base, puis assigne les faders 2 à 4 à trois faders matériels et monte chacun d'eux pour faire entrer son flux par-dessus le reste.

## Organisms associés

Lumen, VideoPlayer, VideoOut
