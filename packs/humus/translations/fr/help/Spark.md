# Spark

Une montée qui n'arrive jamais : douze lignes de retard courtes balaient en filtres en peigne les mêmes octaves, sans jamais en sortir.

Six lignes par canal, chacune longue de quelques millisecondes seulement et remélangée au signal direct pour sonner comme un peigne. Chaque ligne balaie son peigne sur la même étendue d'octaves, mais chacune part d'un point différent, apparaît en fondu en bas et disparaît en fondu en haut sous sa propre fenêtre de gain ; six fenêtres réparties régulièrement ont une somme constante, donc aucune jointure ne s'entend et la hauteur monte sans fin. Time et Range fixent où vit le peigne, d'un sifflement aux temps courts à un corps qui palpite aux temps longs. Aléatoire tire Rate, Time, Range, Feedback, Damp, Spark et Spread ; Mode, Mix et Level ne bougent pas. Fais-y passer un pad, un organism Drums ou tout un bus de Mixer.

## Paramètres

**Mode** Rise monte, Fall descend, et Free va et vient sur l'étendue au lieu de repartir de l'autre bout.

**Rate** La vitesse à laquelle une ligne traverse toute l'étendue, en Hz. La valeur par défaut met environ huit secondes par balayage.

**Time** Le retard le plus court qu'atteint une ligne, en millisecondes : le sommet de sa montée.

**Range** À combien d'octaves sous Time la ligne commence.

**Feedback** La durée pendant laquelle chaque peigne résonne. Bas, il colore le son et passe son chemin ; haut, il fait de chaque ligne une dent étroite et résonante avec sa propre hauteur.

**Damp** Atténue les aigus dans la boucle de réinjection, pour que les longues queues deviennent chaudes plutôt que stridentes.

**Spark** Resserre l'espacement des lignes. À zéro, les lignes sont réparties régulièrement et la montée est lisse ; plus haut, des lignes se superposent et leurs dents se renforcent. Les décalages sont fixes, donc un réglage donne toujours le même résultat.

**Spread** Décale le canal droit par rapport au gauche, pour que les deux côtés montent en décalage et que l'image s'élargisse.

**Mix** Signal direct face au signal en peigne. Le peigne a besoin des deux moitiés, donc c'est au milieu qu'il vit.

**Level** Niveau de sortie.

## Recette

**Montée sans fin** Mode Rise, Rate 0.06, Time 2, Range 4, Feedback 0.8, Damp 0.4, Spark 0.2, Spread 0.6, Mix 0.5. Câble un pad tenu en entrée et laisse-le tourner sous une section ; passe Mode sur Fall pour la retombée après le drop.

## Organisms associés

Fern, Flanger, Chorus
