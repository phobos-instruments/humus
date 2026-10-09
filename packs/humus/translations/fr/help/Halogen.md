# Halogen

Un lecteur spectral qui transforme une image en son : l'horizontale est le temps, la verticale la hauteur, la luminosité le volume.

Une tête de lecture lit une colonne de l'image à la fois et une transformée inverse la reconvertit en son, donc ce qui est sous la tête est ce que tu entends. Dépose une image sur le panneau ou clique dessus pour parcourir tes fichiers ; un fichier qui n'est pas une image est lu comme des octets bruts disposés en rangées, donc une police ou une archive a aussi un spectre. Un câble vidéo venant d'un VideoPlayer, d'un CameraIn ou d'une VideoTrack remplace l'image fixe par l'image animée, image par image, et le fichier revient quand le câble est retiré. Le panneau montre l'image telle que la tête l'entend, après Gate, Blur et Tilt. Câble la sortie vers un Fern ou un Verbatim pour de l'espace.

## Paramètres

**File** L'image en cours de lecture. N'importe quel fichier : les images sont décodées, tout le reste est lu comme des octets.

**Scan** Sweep fait balayer la fenêtre par la tête à la vitesse Rate. Free l'immobilise sur X, donc un bouton rotatif, une enveloppe ou un glisser sur le panneau la déplace.

**Reverse** Balaie dans l'autre sens.

**Rate** Balayages par seconde, de 0.01 à 100. Tout en haut, le balayage lui-même devient une note.

**X** Où se trouve la tête en Free, ou où commence la plage balayée.

**Y** Le bas de la bande lue.

**Width** La part de la largeur de l'image qui est balayée.

**Height** La part de la hauteur de l'image qui est lue.

**Lowest** Le son du bas de la bande, en hertz.

**Highest** Le son du haut de la bande, en hertz. Les rangées sont réparties de façon logarithmique entre les deux.

**Gate** Un seuil de luminosité. Les pixels en dessous sont muets, ce qui empêche un fond sombre de bourdonner.

**Blur** Adoucit l'image pour qu'une tête en mouvement glisse sur les contours au lieu d'avancer par marches.

**Tilt** Penche l'équilibre vers les graves ou vers les aigus.

**Level** Niveau de sortie.

## Recette

**Dérive lente** Charge une photo à fond sombre. Scan Sweep, Rate 0.1, Lowest 55, Highest 4000, Gate 0.15, Blur 0.4. Câble la sortie vers un Verbatim et laisse un balayage durer dix secondes ; règle ensuite Scan sur Free et câble un LFO sur X pour déplacer la tête.

## Organisms associés

Prism, Wave, SoundSpace
