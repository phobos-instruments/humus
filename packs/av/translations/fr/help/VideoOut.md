# VideoOut

La fenêtre ou l'écran par où l'image quitte le patch.

Le jumeau vidéo de SoundOut. Branche un composite de Lumen, un VideoMix, un VideoFX ou un simple VideoPlayer sur son entrée, et ouvre sa fenêtre avec le bouton écran à côté du menu Screen, depuis le sous-menu Sorties vidéo, ou par un clic droit sur l'organism. En fenêtre, elle flotte au-dessus du patcher pendant que tu travailles ; envoyée vers un écran, elle s'y affiche sans bordure et en plein écran. Plusieurs organisms VideoOut font plusieurs sorties : le spectacle peut partir vers un projecteur pendant qu'un mix de prévisualisation reste sur ton propre écran.

## Paramètres

**Screen** Où va l'image. Window fait flotter une fenêtre redimensionnable que tu peux déplacer ; Display 1, Display 2 ou Display 3 occupe tout l'écran branché correspondant, et c'est le réglage pour un projecteur.

**Fade** Le fader maître de toute la sortie. 0, c'est le noir ; pilote-le par automation ou depuis un contrôleur pour faire disparaître le spectacle en fondu.

**Preview** Garde la petite prévisualisation de l'éditeur de l'organism active pendant que la fenêtre de sortie est fermée. Désactive-le pour épargner le travail graphique quand tu n'en as pas besoin.

Une sortie réglée sur un écran se rouvre d'elle-même au chargement du patch, sur cet écran, en le remplissant. Une sortie laissée sur Window, non : rien dans le patch ne dit si sa fenêtre était ouverte, seulement où elle irait.

**OnTop** Garde la fenêtre de sortie au-dessus des autres fenêtres, pour qu'un clic dans le patch n'enterre pas l'image. Activé par défaut ; une sortie qui remplit un écran reste au premier plan quel que soit ce réglage.

## Recette

**Projecteur plus prévisualisation** Branche la sortie de Lumen sur deux organisms VideoOut. Règle l'un sur Display 2 pour le projecteur et laisse l'autre sur Window, sur ton propre écran, avec Preview activé. Assigne Fade de la sortie projecteur à un fader pour pouvoir plonger la salle dans le noir sans toucher au mix.

## Organisms associés

Lumen, VideoMix, VideoPlayer
