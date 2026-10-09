# Decomposer

Audio en entrée, notes MIDI en sortie : un détecteur de hauteur qui transforme ce qui est joué en notes pendant que tu joues, une ligne à la fois ou un accord entier.

Règle Voices sur 1 et il suit une seule voix : c'est le meilleur réglage pour une ligne de basse, une mélodie chantée, un lead ou une guitare jouée note par note. La recherche de hauteur est limitée à la plage de LowNote à HighNote, ce qui est sa meilleure défense contre les erreurs d'octave ; les accrocs d'une seule trame sont lissés, et une note répétée se réarticule depuis son attaque au lieu de se fondre en une seule note tenue. Il rapporte ce qu'il entend sans aucune correction de tonalité ni de gamme ; pour ramener le résultat dans le ton, fais passer les notes par un Trellis. L'affichage de l'éditeur montre la note envoyée et de combien la source est trop haute ou trop basse. Branche un SoundIn dessus et sa sortie MIDI sur n'importe quel instrument, ou sur un MidiMonitor pour la regarder. Compte quelques dizaines de millisecondes de latence de suivi.

## Paramètres

**Voices** Combien de notes à la fois. À 1, il suit une seule ligne, avec la réponse la plus rapide et le suivi le plus stable. Au-dessus de 1, il écoute un accord et envoie chaque note qu'il trouve, jusqu'à six.

À partir de 2, il fonctionne autrement : au lieu de suivre une seule hauteur, il décompose le son. Il évalue chaque note qu'il pourrait entendre d'après les partiels que cette note produirait, garde la plus forte, retire ce que cette note explique, puis cherche à nouveau dans ce qui reste. C'est ce qui lui permet de distinguer un accord d'une seule note au timbre riche. Cela prend du temps : un accord demande environ un tiers de seconde de son pour qu'il en soit sûr, contre quelques dizaines de millisecondes pour une seule ligne, donc Voices 1 reste le bon réglage pour tout ce qui se joue note par note.

Demande le nombre de notes que tu comptes jouer, pas plus. En demander six pour un accord de trois notes ne trouve pas trois notes de plus, mais donne trois occasions de plus de se tromper. Il est le plus sûr de lui sur les sons à la série harmonique claire et au niveau stable : un piano, une guitare claire, un orgue, une nappe de synthé. Il l'est le moins sur l'octave la plus grave, où les notes sont trop proches pour être séparées rapidement, et sur tout ce qui est percussif ou soufflé, où il n'y a pas de série harmonique à décomposer. Sensitivity règle à quel point une note peut être faible, par rapport à la plus forte de l'accord, avant d'être écartée.

**Sensitivity** Abaisse les seuils de niveau et de netteté. Monte-le pour des sources faibles ou soufflées, baisse-le si le bruit déclenche des notes parasites.

**Response** Combien de trames une hauteur doit tenir avant d'être validée. Fast est plus vif, Accurate est plus stable sur les sources bruitées, Balanced se situe entre les deux.

**LowNote** La note la plus grave qu'il rapportera. Monte-la jusqu'à la vraie tessiture de ta source pour que le grondement en dessous ne puisse pas être choisi.

**HighNote** La note la plus aiguë qu'il rapportera. Baisse-la jusqu'à la vraie tessiture de ta source pour que les erreurs d'octave au-dessus ne puissent pas être choisies.

**Channel** Le canal MIDI sur lequel sortent les notes.

## Recette

**Un accord vers une nappe** Branche un SoundIn venant d'une guitare ou d'un clavier sur le Decomposer, Voices 4, LowNote 40, HighNote 84, et sa sortie sur un Rhizome ou un Sampler. Joue un accord et toute sa forme ressort de l'autre côté. L'affichage montre les notes qu'il envoie, de la plus grave à la plus aiguë, et la barre de timbre à côté montre à quel point ce qu'il entend est harmonique : une barre pleine, c'est un accord net ; une barre vide, c'est du bruit qu'il ne nommera pas.

**Voix vers synthé** Branche un SoundIn venant d'un micro sur le Decomposer et sa sortie sur un Rhizome. Response Balanced, Sensitivity 0.5, LowNote 48, HighNote 84 pour une tessiture de chant typique. Fredonne une ligne et le synthé la double ; ajoute un Trellis entre les deux pour rester dans le ton.

## Organisms associés

Trellis, OscMonitor, MidiMonitor
