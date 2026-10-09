# AudioTrack

Le canal audio du séquenceur d'enregistrement : une ligne de la Timeline qui joue ses clips au rythme du transport et enregistre des prises de tout ce qui y est câblé.

Il s'insère en série dans le chemin du signal, entrée stéréo et sortie stéréo : mets-le donc en aval de la source que tu veux capturer. Ce qui en sort, ce sont ses clips, son entrée ou les deux, selon Monitor. Une prise atterrit sous forme de clip sur la ligne de la piste, et un deuxième passage sur les mêmes mesures se pose par-dessus le premier, donc rien n'est perdu. Un fichier audio déposé sur la Timeline atterrit aussi sur un AudioTrack, avec une nouvelle ligne créée et câblée au master s'il n'y en a aucune sous le curseur. Relie-le avant un Mixer, ou directement à un SoundOut.

## Paramètres

**Record** Arme la piste. Le bouton d'enregistrement du transport le pilote, donc on l'arme normalement depuis le transport plutôt qu'à la main.

**Monitor** Ce qui arrive aux sorties. In laisse passer l'entrée en permanence et ajoute les clips pendant la lecture. Auto laisse passer l'entrée quand la piste est armée ou à l'arrêt, et les clips pendant la lecture sans armement, ce qui convient à presque tous les enregistrements. Off ne joue que les clips.

**Gain** Niveau de la sortie de la piste.

**Mute** Coupe la sortie. Comme la piste est en série, cela coupe aussi tout ce qui passe par elle.

## Recette

**Capturer un synthé** Relie un Rhizome à l'AudioTrack et l'AudioTrack au master. Laisse Monitor sur Auto, arme depuis le transport, lance la lecture et joue la partie. Arrête, et la prise devient un clip sur la ligne, que tu peux déplacer et rogner comme n'importe quel autre.

## Organisms associés

Deck, Sampler, PianoRoll, Sequence
