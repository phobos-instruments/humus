# Tuning

L'accordage du patch : chaque organism qui transforme une note en fréquence lui demande la réponse.

Il ne fait aucun son et n'envoie aucune note. Les tempéraments égaux, l'intonation juste à partir de rapports exacts, la série des harmoniques et n'importe quel fichier de gamme .scl sont tous dans un seul navigateur de gammes, et Map décide comment les numéros de note ordinaires trouvent leur hauteur dans une gamme qui n'a pas douze degrés. Les instruments natifs, Rhizome, Substrate, Wave, pH, Mineral, Acid, Microdot et Trellis, le suivent directement ; les plug-ins hébergés sont informés par MIDI, comme le décrit la rangée Plugins. Le piano roll dessine toujours douze lignes par octave, donc avec Map sur Every step dans une gamme qui n'a pas 12 notes, ses étiquettes ne correspondent plus aux hauteurs, même si les notes sont justes.

## Atteindre les organisms

Relie sa sortie MIDI à un instrument par un câble et cet instrument joue dans cet accordage. L'accordage descend le long des câbles MIDI, donc Tuning vers DNA vers Rhizome accorde les deux, et deux organisms Tuning mettent deux synthés dans deux accordages à la fois. Ne le relie à rien et il accorde tout le patch : c'est le cas habituel, et le seul moyen d'atteindre un organism sans entrée MIDI, comme Trellis.

## Paramètres

**Preset** Le mode, en bas de la boîte. Scale file, le réglage par défaut, joue la gamme choisie dans le navigateur en haut, et un nouveau Tuning y démarre sur 12-TET, l'accordage ordinaire, qui est un fichier de gamme comme les autres. Custom EDO divise l'octave en son propre nombre de Divisions égales et grise le navigateur, en gardant sa gamme pour quand tu reviens. Les gammes que cette liste contenait avant - 12-TET, 19, 24 et 31-EDO, Bohlen-Pierce, les gammes justes de limite 5, 7 et 17, Harmonics 8-16 et Pythagorean - sont maintenant dans le navigateur, identiques note pour note. Un patch enregistré sur l'une d'elles s'ouvre toujours dessus, affiche son nom ici et joue comme il l'a toujours fait.

**Divisions** Degrés égaux par octave pour Custom EDO, de 1 à 128. Un, ce sont seulement des octaves, deux ajoute le triton, et 128 est le maximum qu'une gamme peut contenir. Grisé en mode Scale file, où c'est le fichier qui décide.

**Root** La note MIDI fixée sur RootHz. 69 est A4.

**RootHz** La fréquence à laquelle sonne cette note. 440 est la norme.

**Map** Comment une touche trouve sa note quand la gamme n'en a pas douze. Every step joue la note suivante de la gamme sur chaque touche, donc chaque note est accessible, une octave de 19 notes prend 19 touches, et changer de gamme déplace tout. Fit to 12 keys répartit une période de la gamme sur une octave ordinaire de douze touches et fait sonner la note la plus proche de la place habituelle de chaque touche, donc une gamme juste de sept notes tombe sur les touches blanches et la touche d'octave reste une vraie octave ; avec plus de douze notes, certaines n'ont pas de touche. Avec une gamme de douze notes, les deux reviennent au même.

**File** La gamme, en haut de la boîte : le fichier .scl que joue le mode Scale file. Clique sur le nom pour ouvrir le navigateur de gammes : les gammes livrées avec l'app, les tiennes, celles que tu as importées d'ailleurs et celles que tu as utilisées récemment, chacune avec son nombre de notes et sa description, et une recherche qui lit tout ça. Les flèches passent à la gamme suivante et à la précédente. Choisir une gamme sélectionne aussi le preset. Un fichier manquant est marqué en ambre et joue en simple 12-TET.

**Plugins** Comment les plug-ins hébergés sont informés. MTS SysEx envoie des messages d'accordage note par note aux plug-ins qui comprennent la norme. Pitch bend met chaque note sur son propre canal avec un bend pour le reste de l'écart, ce qui marche presque partout mais limite la polyphonie et prend la main sur la molette de pitch bend. Auto (probe) teste chaque plug-in une fois et retient le verdict.

## Recette

**Quinte sans battements** Preset Just - 17-limit, Map Fit to 12 keys, ne le relie à rien pour que tout le patch suive. Tiens une quinte sur un Rhizome depuis un PianoRoll : elle se cale sans battements comme aucune quinte tempérée ne peut le faire. Puis passe à Harmonics 8-16 et joue une gamme sur le même Rhizome pour entendre la série des harmoniques elle-même.

## Organisms associés

Rhizome, Trellis, Substrate
