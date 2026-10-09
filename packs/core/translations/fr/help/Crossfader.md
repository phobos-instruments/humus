# Crossfader

Un fondu enchaîné stéréo entre deux sources, sur une seule commande.

Les entrées 1-2 sont le côté A et les entrées 3-4 le côté B. Fade tout en bas laisse passer A, tout en haut laisse passer B, et toute position intermédiaire mélange A et B selon la loi que choisit Curve. Câble deux organisms Deck, deux boucles, ou une copie directe et une copie traitée d'un même signal, et câble la sortie vers un Mixer ou un SoundOut. Équilibre les deux côtés avec TrimA et TrimB avant de toucher à Curve : un fondu qui fait un bond vient en général de deux sources à des niveaux différents, pas d'une mauvaise loi. Fade peut aussi piloter d'autres paramètres via Contrôle des paramètres, pour qu'un seul balayage ouvre un filtre au passage.

## Paramètres

**MasterGain** Niveau de sortie après le fondu et les trims.

**TrimA** Niveau du côté A avant le fondu, pour équilibrer d'abord les deux sources.

**TrimB** La même chose pour le côté B.

**Fade** La position du fondu. En bas c'est A, en haut c'est B.

**Curve** La loi du fondu. Zéro est une ligne droite, adaptée à une copie directe et une copie traitée d'un même son ; environ deux tiers donne une puissance constante, adaptée à deux sources sans rapport ; un donne une coupure rapide, où un petit mouvement près de l'une ou l'autre extrémité bascule complètement le son.

**CutA** Maintenu, le mix saute sur le côté A quoi que dise Fade, et revient à Fade au relâchement. Maintenir A et B ensemble cale le fondu au milieu.

**CutB** La même chose pour le côté B.

**CutTime** Le temps que met une coupure à arriver, en millisecondes. Zéro est instantané ; quelques millisecondes enlèvent le clic sur une basse tenue.

## Recette

**Mix à deux platines** Câble un Deck dans les entrées 1-2 et un autre dans les entrées 3-4, règle Curve sur environ 0.67, et ajuste TrimA et TrimB jusqu'à ce que chaque platine affiche la même valeur sur le vumètre du Mixer, avec Fade à l'une ou l'autre extrémité. Assigne CutA et CutB à deux pads et règle CutTime sur environ 5 pour hacher le morceau entrant par-dessus le sortant, sans clics.

## Organisms associés

Mixer, Gain, Console, Send
