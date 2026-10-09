# FileRecorder

Un passe-plat qui écrit ce qui lui parvient dans un fichier audio pendant que le patch joue.

L'entrée va telle quelle à la sortie, sans changement : il se glisse donc sur n'importe quel câble du patch et y enregistre un stem, plutôt que seulement au master. Le menu déroulant Tracks fixe le nombre de fichiers stéréo qu'il écrit, de 1 à 32, avec une ligne de fichier par piste ; les canaux sont attribués aux fichiers dans l'ordre, le sélecteur Ch de chaque ligne indique combien il en prend, et un fichier qui dépasse la dernière entrée enregistre du silence. Les fichiers sont écrits hors du chemin audio, pour qu'un disque lent ne puisse pas interrompre le son, et chaque ligne mesure ce qui arrive, qu'une prise soit en cours ou non. Le bouton d'enregistrement du transport, lui, capture sur la timeline les organisms AudioTrack armés ; utilise cet organism quand tu veux de simples fichiers sur le disque.

## Paramètres

**File_1** L'emplacement du fichier 1, et ainsi de suite pour les lignes suivantes. Laisse-le vide et chaque armement écrit une prise horodatée dans le dossier des enregistrements et remplit la ligne ; un fichier que tu as nommé toi-même est conservé et réutilisé.

**RequestedChannelCounts_1** Le nombre de canaux que prend le fichier 1, et ainsi de suite pour les lignes suivantes. Deux par défaut ; zéro désactive la ligne.

**FileMode** Overwrite remplace le fichier nommé. Append enregistre à la suite du fichier, pour construire une session en plusieurs passages.

**PunchMode** Manual et SoundIn Sync s'arrêtent avec le bouton. Timed s'arrête tout seul après RecordDuration et finalise le fichier.

**RecordDuration** La durée d'un passage en mode Timed, en millisecondes.

**Record** Arme et désarme tous les fichiers à la fois. Il se désactive tout seul quand aucune ligne n'a de canaux à écrire, et une prise doit être arrêtée ici plutôt qu'abandonnée, car c'est l'arrêt qui écrit l'en-tête qui rend le fichier lisible.

## Recette

**Stem de batterie** Place le FileRecorder entre l'organism de batterie et le Mixer, laisse File_1 vide, Ch à 2, PunchMode sur Manual et FileMode sur Overwrite. Arme Record pour le passage et désarme-le à la fin ; la ligne montre où la prise a atterri, et le mix a continué sans changement pendant l'écriture.

## Organisms associés

AudioTrack, SoundOut, Mixer, Bus
