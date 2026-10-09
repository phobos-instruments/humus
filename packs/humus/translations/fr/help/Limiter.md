# Limiter

Un limiteur brickwall à attaque instantanée : rien ne dépasse le plafond.

Threshold est l'endroit où la limitation commence et Ceiling le niveau que la sortie peut atteindre, et la sortie est mise à l'échelle pour qu'un son qui arrive au seuil ressorte au plafond. Baisser Threshold rend donc le résultat à la fois plus fort et plus limité : c'est le bouton rotatif à manier ; Ceiling est la ligne de sécurité qu'on règle une fois. L'attaque est instantanée et un écrêtage franc au plafond lui sert de filet de sécurité, donc toute l'expressivité se joue dans le relâchement. La détection est liée entre les canaux, donc la limitation ne fait jamais pencher l'image stéréo. Branche-le en dernier, après une Console ou un Mixer et avant le SoundOut.

## Paramètres

**InputGain** Ajuste le son entrant avant la limitation.

**Threshold** L'endroit où la limitation commence. Baisse-le pour plus de limitation et un résultat plus fort.

**Ceiling** Le niveau le plus haut autorisé en sortie. Laisse une petite marge sous la pleine échelle pour ce qui vient après.

**HoldTime** Combien de temps la réduction maximale est tenue avant que le relâchement commence, en millisecondes. Un peu de maintien évite le crépitement sur des crêtes rapprochées.

**ReleaseTime** La vitesse à laquelle le limiteur récupère après une crête, en millisecondes. Court, il garde le volume mais peut pomper sur une matière dense ; long, il reste propre et cède un peu de niveau.

## Recette

**Plafond du master** Relie la sortie de la Console au Limiter par un câble, et le Limiter au SoundOut. Ceiling 0.95, ReleaseTime 150, HoldTime 10, puis baisse Threshold jusqu'à ce que la section la plus forte montre quelques dB de réduction, pas plus.

## Organisms associés

Compressor, Console, VuMeter
