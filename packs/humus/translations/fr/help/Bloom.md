# Bloom

La réverb shimmer : un réservoir de réverb modulé avec un transpositeur dans sa boucle de réinjection.

Chaque passage dans le réservoir est transposé à nouveau, si bien qu'une queue monte d'une octave, puis de deux, jusqu'à ce que l'amortissement l'emporte. L'équilibre entre la réinjection simple et la réinjection transposée, c'est Shimmer : à zéro, c'est une réverb ordinaire, à fond, la queue n'est que montée, sans corps. Feedback règle combien de temps le tout résonne. Passe Shift en négatif et la queue descend au lieu de monter. Branche-le après un Rhizome, un Wave ou un Sampler, et mets un Fern avant lui pour une queue qui fait aussi écho.

## Paramètres

**Mix** Signal direct contre signal traité.

**Shift** De combien le transpositeur transpose chaque passage, en demi-tons. 12 est l'octave classique, 7 empile des quintes, -12 descend.

**Shimmer** La part du signal transposé qui est réinjectée dans le réservoir, par rapport à la queue non transposée.

**Feedback** La part du réservoir qui revient à sa propre entrée. C'est la longueur de la queue.

**Size** Les dimensions de la pièce, de petite à immense.

**Diffusion** La vitesse à laquelle les premiers échos se fondent ensemble. Bas, il laisse entendre des échos distincts dans la queue.

**LowCut** Retire le grave à l'entrée du réservoir, pour que la montée reste hors des basses.

**HighCut** Assombrit la queue à chaque passage. Plus bas donne une queue plus courte et plus chaude.

**ModRate** La vitesse du mouvement dans le réservoir, en Hz.

**ModDepth** L'ampleur de ce mouvement. Un peu garde la queue vivante, beaucoup la transforme en eau.

**Mode** Le type de pièce. Bloom est petite et proche, Hall est naturelle, Cathedral est immense, Cloud se situe entre les deux avec plus de diffusion.

**Voice** L'agencement du transpositeur. Single suit Shift, Dual ajoute une deuxième voix qui va dans le sens opposé, Stacked ajoute une deuxième voix à deux fois la transposition, et Off est une réverb simple, quoi que dise Shift.

**Color** L'inclinaison de la queue. Bright garde les copies transposées ouvertes, Dark les atténue, Neutral se situe entre les deux.

## Recette

**Halo de pad** Mode Cathedral, Voice Single, Shift 12, Shimmer 0.6, Feedback 0.6, HighCut 6000, Color Dark, Mix 0.4. Branche un pad lent de Rhizome à l'entrée et tiens un accord une mesure ou deux pendant que les octaves s'empilent.

## Organisms associés

Fern, Prism, SpectralFreeze
