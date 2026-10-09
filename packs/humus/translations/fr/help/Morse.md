# Morse

Un générateur de patterns qui émet un message tapé en morse, sous forme de son sinusoïdal et de notes MIDI, avec le vrai timing du morse.

Tape dans Text et le message est émis en points, traits, espaces entre lettres et espaces entre mots, calé sur le transport : muet à l'arrêt, il reprend au début du message quand la lecture démarre. Le son suit l'accordage du patch et les fronts de chaque impulsion sont adoucis sur 3 ms, donc il ne claque pas. La sortie MIDI émet le même message en paires note-on et note-off à la hauteur de Note ; branche-la dans un Kick et le message devient un rythme, ou dans un Acid et il se joue comme une ligne ; avec Level à 0, c'est une pure source de patterns MIDI. Envoie le son dans un Fern pour une ligne mélodique.

## Paramètres

**Text** Le message. Lettres, chiffres et ponctuation courante ; tout ce que le morse ne sait pas dire est ignoré. Le ruban en dessous dessine le message en points et en traits, et allume celui qui est en cours d'émission.

**WPM** Vitesse d'émission en mots par minute, de 5 à 40. Une unité dure 1.2 divisé par WPM secondes.

**Sync** Ignore WPM et fait durer une unité exactement une double croche, donc le message devient un rythme sur la grille et les changements de tempo le recalent. Le message commence sur le temps 1 et sa position suit le transport, donc sauter ou boucler dans la timeline retombe à chaque fois sur la même lettre.

**Note** La hauteur du son, sous forme de note, qui suit l'accordage du patch.

**Loop** Répète le message, avec entre deux passages l'espace standard de sept unités entre les mots. Désactivé, il joue un seul passage à chaque lecture.

**Level** Niveau de sortie du son.

## Recette

**Rythme télégraphique** Text CQ CQ, Sync activé, Loop activé, Note D5. Branche la sortie MIDI dans un Kick et fais passer la sortie audio par un Fern avec Sync activé sur 1/8 ; lance la lecture à 140 BPM et le message fait jouer le Kick.

## Organisms associés

Rhizome, Sequence, SideKick
