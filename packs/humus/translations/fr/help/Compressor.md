# Compressor

Un compresseur feed-forward avec un coude doux, un étage de tenue et un relâchement synchronisé au tempo.

Il mesure l'entrée, décide d'un gain et l'applique : les moments forts sont baissés et l'écart entre le plus fort et le plus faible se réduit. La détection est liée entre les canaux : un seul gain est calculé et appliqué à chaque côté, donc un moment fort à gauche ne peut pas tirer l'image vers la droite. Règle d'abord Threshold, puis CompressionRatio, puis AttackTime et ReleaseTime, et MakeupGain en dernier. Sur une partie qui a une pulsation, active Sync pour que la récupération tombe sur une division du temps et y reste quand le tempo change. Il n'y a pas d'entrée de clé ; pour compresser un son avec un autre, utilise SideChain.

## Paramètres

**InputGain** Ajuste le son entrant avant tout le reste. Le monter pousse une plus grande part du son au-dessus du seuil, sans déplacer le seuil.

**MakeupGain** Gain ajouté après la réduction, pour remonter le son nivelé.

**AutoMakeupGain** Calcule le gain de compensation à partir de Threshold et CompressionRatio, pour que changer l'un ou l'autre ne change pas aussi le niveau. Pratique pendant le réglage.

**AttackTime** La vitesse de réaction une fois le seuil franchi, en millisecondes. Rapide, il attrape le coup ; lent, il laisse passer le coup et nivelle ce qui suit.

**HoldTime** Combien de temps la réduction complète est gardée avant que le relâchement commence, en millisecondes. Empêche le compresseur de lâcher puis de reprendre sur un matériau qui baisse brièvement.

**ReleaseTime** La vitesse à laquelle il s'arrête une fois que le son repasse sous le seuil, en millisecondes. Ignoré quand Sync est activé.

**CompressionRatio** La fermeté avec laquelle le son est tenu. 2 signifie qu'un son à 2 dB au-dessus du seuil finit à 1 dB au-dessus. Les ratios faibles nivellent, les ratios élevés écrasent.

**Threshold** Le niveau où la compression commence. Au-dessus, le son est retenu ; en dessous, rien ne se passe.

**KneeWidth** La progressivité avec laquelle le ratio arrive, en dB de part et d'autre du seuil. Un coude large entre en douceur au lieu de s'enclencher d'un coup.

**Sync** Cale le relâchement sur le transport au lieu de ReleaseTime. L'attaque reste où tu l'as mise.

**SyncUnit** La valeur de note dans laquelle compte le relâchement synchronisé, comme 1/8.

**SyncMultiplier** Combien de ces unités font un relâchement.

## Recette

**Cohésion du bus batterie** Threshold 0.4, CompressionRatio 3, KneeWidth 8, AttackTime 20, HoldTime 0, Sync activé avec SyncUnit 1/8. Branche la somme batterie d'un Mixer, active AutoMakeupGain pendant que tu règles le seuil, puis désactive-le une fois le niveau là où tu le veux.

## Organisms associés

SideChain, Limiter, NoiseGate, TransientShaper
