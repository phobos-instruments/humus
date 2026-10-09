# Sig

Transforme une valeur de contrôle en signal audio, avec une rampe pour ne jamais claquer.

Les câbles transportent du son et les nombres arrivent sur des boutons rotatifs ; Sig est le seul organism qui renvoie un nombre sous forme de câble, pour les rares endroits qui en ont besoin. La plupart du temps, un Follower, un capteur ou un LFO va directement sur un bouton rotatif par un câble de contrôle, mais un Number qui doit entrer dans une formule Math comme signal, un Slider qui pilote une sortie en tension de contrôle via un AuxOut avec sa protection DC désactivée, ou une constante ajoutée à un câble ont tous besoin de Sig. Tout ce qui arrive sur sa prise Value ressort par la sortie en signal stable.

## Paramètres

**Value** Le nombre à transformer en signal. Câble n'importe quoi dessus, ou tape-le.

**Slew** Le temps que met un changement à arriver, en millisecondes. À 0, un changement arrive en un seul échantillon ; les 5 ms par défaut arrondissent les marches sans retard perceptible.

## Recette

**Décalage dans Math** Câble un Slider sur la Value de Sig et la sortie de Sig sur l'entrée b de Math, avec l'audio sur a. Écris a+b dans Math et le Slider ajoute un décalage DC au signal ; règle Slew sur 200 pour que le mouvement du fader glisse au lieu de faire des marches.

## Organisms associés

Number, Slider, Follower, Math
