# FilePlayer

Lit des fichiers son dans le patch : un fichier stéréo, ou jusqu'à 32 à la fois, un par sortie.

Utilise-le pour des pistes d'accompagnement, des enregistrements de terrain et des sons ponctuels à déclencher qui n'ont pas besoin de suivre le transport. Il tourne librement : Active le lance et l'arrête, et la barre de position sur la boîte permet d'aller à n'importe quel point pendant qu'il joue ou reste en pause. Chaque bouton de la bande de transport est un paramètre, alors fais un clic droit sur n'importe lequel pour Apprentissage MIDI, Apprentissage OSC, Contrôler avec ou Suivre. Le Stop du transport le laisse jouer ; appuie une seconde fois sur Stop, quand plus rien ne tourne, et il s'arrête avec tout le reste. Loop répète le fichier avec un silence optionnel entre les passages, et AutoRewind décide si chaque démarrage reprend là où il s'était arrêté ou depuis le début. Pour des boucles qui restent calées sur le temps, utilise plutôt LoopPlayer. Branche la sortie sur un Mixer ou directement sur SoundOut.

Outputs, en haut de la boîte, en fait un lecteur multi-fichiers à 4, 6, 8, 16, 24 ou 32 sorties, avec une ligne de fichier par sortie. Un fichier dans la ligne 1 joue à partir de la sortie 1, une sortie par canal : un fichier mono prend une sortie, un fichier stéréo dans la ligne 1 prend les sorties 1 et 2, et un fichier à six canaux en prend six, tandis que les lignes qu'il couvre restent muettes. Tous les fichiers démarrent, s'arrêtent, bouclent et se déplacent ensemble, le plus long fixe la durée, et les plus courts se taisent à leur fin jusqu'au passage suivant. Changer Outputs garde les fichiers et tous les câbles qui tiennent encore ; à 2, c'est de nouveau le lecteur stéréo.

## Paramètres

**File** Le fichier son à lire. Choisis-le avec le bouton de fichier ou dépose un fichier sur l'emplacement.

**File_1-File_32** Le fichier de chaque ligne d'un lecteur multi-fichiers, affiché quand Outputs dépasse 2.

**Loop** Relit le fichier depuis le début chaque fois qu'il arrive à la fin. Désactivé, il le lit une fois puis se tait.

**LoopDelay** Secondes de silence insérées entre la fin d'un passage et le début du suivant. Utilisé seulement quand Loop est activé.

**AutoRewind** Revient au début chaque fois qu'Active est activé. Désactivé, il reprend là où la lecture s'est arrêtée la dernière fois.

**Active** Lance et arrête la lecture. Automatise-le pour faire entrer un fichier à un moment précis du patch.

## Recette

**Accompagnement à la demande** Charge le fichier, AutoRewind activé, Loop désactivé. Branche la sortie sur un Gain, puis sur SoundOut, et assigne Active à un Button pour qu'un appui lance la piste depuis le début à chaque fois ; un deuxième appui l'arrête et le suivant la relance proprement.

## Organisms associés

SoundIn, LoopPlayer, Drums
