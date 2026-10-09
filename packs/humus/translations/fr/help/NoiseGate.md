# NoiseGate

Un gate qui laisse passer le son tant qu'il est assez fort et se ferme quand il ne l'est pas, ou qui fait l'inverse en l'atténuant.

Il supprime le souffle, le ronflement et la repisse qui traînent dans les silences entre les phrases, resserre une batterie en coupant sa queue, et en mode Duck maintient le son bas tant qu'il dépasse le seuil. Le réglage Threshold a deux poignées parce que le gate a deux niveaux : la haute l'ouvre, la basse le ferme, et les garder écartées empêche le gate de bégayer sur un signal qui hésite autour de la ligne. La détection est liée entre les canaux, il ne retire donc jamais un seul côté de l'image. Il n'y a pas d'entrée key : pour hacher un son au rythme d'un autre, utilise SideChain. Câble-le après un SoundIn ou avant un Compressor.

## Paramètres

**Mode** Off laisse passer les sons forts et atténue les sons faibles. Duck fait l'inverse : il maintient le son bas tant qu'il dépasse le seuil.

**InputGain** Ajuste le son entrant avant la détection, pour pousser une matière au-dessus du seuil sans déplacer celui-ci.

**Threshold** Le niveau d'ouverture sur la poignée haute et le niveau de fermeture sur la basse. Garde-les écartés pour que le gate ne bégaie pas.

**Range** De combien le son est atténué quand le gate est fermé. Tout en bas, rien ne passe ; un peu plus haut, les silences restent calmes au lieu d'être vides.

**AttackTime** La vitesse à laquelle le gate s'ouvre, en millisecondes. Assez rapide pour ne pas émousser une note, assez lente pour ne pas faire de clic.

**HoldTime** Combien de temps le gate reste ouvert après que le signal est passé sous le niveau de fermeture, en millisecondes. Ça permet à une batterie de garder son corps avant que le gate se ferme.

**ReleaseTime** La vitesse à laquelle le gate se ferme une fois HoldTime écoulé, en millisecondes. Court, c'est abrupt et rythmique ; long, ça estompe la queue.

## Recette

**Caisse claire serrée** Câble la caisse claire dedans. Threshold à 0.3 pour l'ouverture et 0.15 pour la fermeture, Range 0, AttackTime 0.5, HoldTime 40, ReleaseTime 60. Baisse HoldTime jusqu'à ce que la queue soit coupée court, et monte un peu Range si les silences sonnent vides.

## Organisms associés

SideChain, Compressor, SideKick, TransientShaper
