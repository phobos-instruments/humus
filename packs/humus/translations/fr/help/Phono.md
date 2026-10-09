# Phono

Un préamplificateur phono : désaccentuation RIAA, gain de cellule et filtre anti-rumble pour une platine branchée sur une entrée ligne.

Les disques sont gravés avec les basses réduites et les aigus renforcés, et une cellule sort quelques millivolts, donc un disque joué directement dans une entrée ligne sonne maigre et très faible. Phono annule la courbe et rétablit le niveau. Le filtre de désaccentuation est calé sur la réponse RIAA analogique à la fréquence d'échantillonnage en cours, octave la plus haute comprise. Relie-le par un câble juste après un SoundIn et avant un Deck, une Console ou un Mixer. Si la voie de l'interface a un sélecteur phono/ligne, mets-le sur ligne, sinon la correction est appliquée deux fois. Le signal passe par la désaccentuation, puis le filtre anti-rumble, puis le gain.

## Paramètres

**Curve** RIAA est la courbe de lecture standard. RIAA + IEC ajoute une douce atténuation à 20 Hz que certains préamplis phono intègrent. Flat saute l'égalisation et ne garde que le gain, pour des numérisations que tu égaliseras plus tard ou pour des disques plus anciens que la norme.

**Gain** Gain de cellule en dB. Une cellule à aimant mobile, comme toutes les cellules DJ standard, demande environ 40 ; une cellule à bobine mobile à faible sortie demande environ 60. Trop haut, ça sature.

**Rumble** Un passe-haut à 18 Hz qui retire le flottement des disques voilés et le rumble de la platine avant qu'ils ne mangent la marge. Activé par défaut ; désactive-le pour les travaux de mesure.

## Recette

**Entrée platine** Relie SoundIn à Phono par un câble, puis Phono à un Deck ou à un Mixer. Curve RIAA, Rumble activé, Gain 40 pour une cellule à aimant mobile ; monte Gain par pas de 5 jusqu'à ce qu'un passage fort arrive juste sous le maximum sur les vumètres.

## Organisms associés

Console, Gain, Deck, SoundIn
