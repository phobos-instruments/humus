# Math

Une formule d'une ligne qui s'exécute une fois par échantillon, comme oscillateur, comme processeur ou comme modulateur.

Baisse d'abord le volume : Math envoie tout ce que tu tapes directement à la sortie, et une formule peut être à pleine échelle, une composante continue pure ou un son bien en dehors de la plage audible. Les deux entrées arrivent comme a et b, les quatre boutons rotatifs comme x, y, z et w, l'entrée MIDI comme note, freq, gate et vel, et l'horloge fournit t, beat, bpm et sr. Un brouillon valide sonne tout de suite, et une modification à moitié tapée laisse tourner la dernière expression valide. Il y a deux sorties : une ligne qui ne mentionne jamais ch est envoyée aux deux, et une ligne qui le mentionne est calculée pour chaque côté. La valeur de contrôle out suit le dernier échantillon de chaque bloc, donc Contrôler avec assigne la formule à n'importe quel bouton rotatif. Câble un PianoRoll dans l'entrée MIDI pour une voix, ou un signal audio dans a pour un processeur.

## Le vocabulaire

Nombres, parenthèses, + - * / %, comparaisons qui répondent 0 ou 1, if(c, then, else) ; des termes côte à côte se multiplient, donc 2a marche. Fonctions : sin cos tan tanh asin exp log log2 sqrt abs floor ceil fract pow min max clamp mix step smoothstep, les formes de phase saw tri sqr pulse, ph(hz), harm(hz, n, tilt), env(gate, attack, release) et noise(). prev est l'échantillon de sortie précédent, dt vaut un échantillon en secondes, phase est le cycle de freq qui va de 0 à 1, et pi, tau et e sont définis. Les instructions se séparent par des points-virgules, et name := value garde une valeur d'un échantillon à l'autre.

rate est l'allure de ce qui joue la formule : 1 dans un Math, et le Rate du LFO quand la formule joue dans un LFO, donc un mouvement qui avance de rate*dt accélère avec lui. Les presets Nature sont des mouvements écrits ainsi, à lire et à changer : Gravity fait rebondir une balle, Pendulum se balance, Heart bat son double coup, Breath inspire et expire, Swarm clignote comme trois lucioles qui se mettent au pas, Wind souffle en rafales et Wander erre comme une main sur un bouton, chacun de régulier avec X à 0 à sauvage à 1. noise() suit sa propre suite dans chaque boîte, donc deux Math errent différemment tandis qu'un patch joue toujours pareil. Une formule qui avance de dt montre huit secondes dans le tracé.

## Paramètres

**Expression** La formule. a et b sont les entrées, x, y, z et w les boutons rotatifs, ch le côté de la sortie, et tout autre nom est le Var de ce nom câblé dans Vars (en minuscules, les espaces devenant des tirets bas). Elle est enregistrée telle qu'écrite, valide ou non.

**X** Variable de bouton x, de 0 à 1. Automatise-la, assigne-la ou passe-la dans un morphing. Un Var nommé x câblé dans Vars pilote ce bouton : le bouton bouge avec le Var et le son suit.

**Y** Variable de bouton y, de 0 à 1.

**Z** Variable de bouton z, de 0 à 1.

**W** Variable de bouton w, de 0 à 1.

**Auto** Activé, la formule joue toute seule, avec freq à la valeur du bouton Freq et gate à 1. Désactivé, elle ne sonne que pendant qu'une note MIDI est tenue, à la hauteur de cette note.

**Freq** La hauteur que porte freq tant que Auto est activé et qu'aucune touche n'est enfoncée, de 20 à 2000 Hz.

**Vars** Une entrée pour chaque Var que tu veux montrer sur le patch. Câble un Var et son nom fait le reste : x, y, z et w pilotent leurs boutons, tout autre nom est lu par la formule. Seuls les Var câblés comptent : retire le câble et le lien disparaît.


## Recette

**Voix jouable** Auto désactivé, Expression `sin(tau*ph(freq)) * vel * env(gate, 0.01, 0.3)`, et câble un PianoRoll dans l'entrée MIDI. Remplace `sin(tau*ph(freq))` par `harm(freq, 1 + x*15, y*2)` : x compte les harmoniques et y les façonne.

**Contrôles nommés** Crée un Var appelé x et un autre appelé depth, câble les deux dans Vars, et écris `sin(tau*ph(freq)) * depth * x`. Tire un câble d'un Slider ou d'un LFO vers la Value de chaque Var : le bouton X bouge avec le premier, la formule lit le second, jusqu'à huit noms.

**Mouvement vivant** Charge le preset Nature Gravity et utilise Contrôler avec sur la coupure d'un filtre : la balle rebondit de plus en plus tôt et de plus en plus bas jusqu'à s'arrêter, puis elle est relancée. Monte X pour une balle plus vive, ou charge Heart, Wind ou Breath.

**Une forme pour un LFO** Écris un cycle en fonction de phase, comme `pow(sin(tau*phase), 3)` ou `tri(phase)*x`, et relie le Math à la prise Function d'un LFO. Le LFO passe sur FN et joue la forme à son propre Rate, en suivant chaque modification et chaque bouton tourné ici.

## Organisms associés

Number, LFO, Follower, Var, Wave
