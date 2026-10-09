# Drops

Des gouttes d'eau comme instrument : une pluie aléatoire de petits événements physiques pendant que le transport joue.

Chaque goutte est une sinusoïde amortie à la résonance d'une bulle, dont la hauteur monte pendant qu'elle s'éteint, et c'est ce qui donne à un goutte-à-goutte son côté mouillé. Les gouttes tombent au hasard tant que le transport tourne et s'arrêtent quand il s'arrête, et un même patch rend la même pluie à chaque fois, donc un bounce ne réserve aucune surprise. Size règle la bulle, Spread à quel point chaque goutte s'en écarte, et Pool ajoute une petite queue résonante sous la pluie. Branche-le dans un Fern pour des gouttes qui se répondent, ou dans un SoundSpace pour une grotte.

## Paramètres

**Density** Gouttes par seconde. Une fuite à 1, la pluie à 8, une averse à 20.

**Size** L'échelle de la bulle. À gauche, de courts tics brillants ; à droite, de grosses gouttes avec une vraie hauteur.

**Spread** À quel point chaque goutte s'écarte de Size. À zéro, toutes les gouttes sont des clones ; grand ouvert, il pleut des gouttes de toutes tailles à la fois.

**Chirp** Le pitch bend montant à l'intérieur de chaque goutte. À zéro, les gouttes sont de simples pings ; au-delà de la moitié, elles font bloop.

**Pool** Une petite queue résonante et humide sous la pluie. De la pierre sèche à zéro, une citerne à un.

**Width** Jusqu'où les gouttes tombent dans le champ stéréo.

**Level** Volume de sortie.

## Recette

**Fuite lente** Density 2, Size 0.8, Spread 0.3, Chirp 0.8, Pool 0.5, Width 0.7. Fais-le passer par un Fern avec Feedback autour de 0.5 et chaque goutte revient un temps plus tard. Pousse Density à 15 et baisse Size à 0.2 pour de la pluie sur une vitre.

## Organisms associés

SoundSpace, Fern, Morse
