# Button

Un bouton-poussoir sous forme d'organism, avec une sortie de contrôle et une note MIDI.

Appuie dessus et la sortie passe de Off à On : n'importe quel bouton rotatif du patch peut la suivre par un câble de contrôle ou un routage Contrôler avec, et une note part de la sortie MIDI. Relâche, et les deux reviennent. Latch transforme l'appui en bascule, ce qui en fait une coupure du son à peu de frais, un sélecteur de scène, ou un moyen de garder un Gate ouvert sans y laisser le doigt. Assigne un pad, une touche ou une pédale à Press par apprentissage MIDI et le bouton vit sur ton contrôleur ; câble le gate d'un Follower sur la prise Press et le patch appuie pour toi. Tout ce qui est câblé dans son entrée s'ajoute à la sortie.

## Paramètres

**Press** Le bouton lui-même. Momentané, sauf si Latch est activé.

**Latch** Chaque appui inverse l'état au lieu de suivre le doigt.

**Off** Le niveau de la sortie quand le bouton est éteint. Il peut être plus grand que On, ou négatif pour un interrupteur bipolaire.

**On** Le niveau de la sortie quand le bouton est allumé.

**Slew** Le temps que met la sortie pour atteindre le nouveau niveau, en millisecondes. Zéro bascule d'un coup, ce qui fait un clic à travers un gain, mais c'est ce qu'il faut pour un déclencheur.

**Note** La note MIDI envoyée tant que le bouton est allumé : note on quand il s'allume, note off quand il s'éteint.

**Velocity** La vélocité de cette note.

## Recette

**Coupe-circuit** Latch activé, Off 0, On 1, Slew 40. Câble la sortie sur le Mute d'un Gain placé sur un bus de batterie et assigne Press à un pad par apprentissage MIDI. Un appui coupe la batterie, le suivant la ramène, sans clic.

## Organisms associés

Slider, Number, Gate, Kick
