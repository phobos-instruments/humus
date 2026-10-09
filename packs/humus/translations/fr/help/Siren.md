# Siren

Une sirène de sound system : un oscillateur râpeux balayé entre deux hauteurs par une rampe lente, tenu aussi longtemps que tu veux.

Notre propre conception, d'après les boîtes classiques à deux transistors, avec les quatre mêmes réglages en façade, les deux extrémités de la hauteur et les deux moitiés du balayage, et les interrupteurs que ces boîtes portent sur le côté. Tiens le bouton Hold, tiens un pad qui lui est assigné, ou tiens n'importe quelle note sur un câble MIDI ; chaque pression relance le balayage depuis son début, et au relâchement le son s'éteint sur la durée de Release. La sortie est mono. Le vumètre suit le balayage, et le balayage et le gate sont aussi des valeurs de contrôle, donc une Siren peut piloter une lumière ou un Filter pendant qu'elle hurle.

## Paramètres

**PitchLow** Le bas du balayage en Hz. Règle-le au-dessus de PitchHigh et le balayage part dans l'autre sens.

**PitchHigh** Le haut du balayage en Hz.

**LfoUp** Le temps que met la rampe à monter, en secondes. Des Up et Down inégaux donnent la plainte bancale.

**LfoDown** Le temps que met la rampe à descendre, en secondes.

**Mode** Sweep monte et descend. Rise monte puis revient d'un coup, et se repose pendant le temps Down. Fall descend puis remonte d'un coup, en se reposant pendant le temps Up. Step saute entre les deux hauteurs : le deux-tons.

**Wave** Square, Pulse ou Saw pour l'oscillateur. Square, c'est le son de la boîte.

**Tone** Un passe-bas en sortie. Baisse-le pour une corne plus sourde.

**Auto** Activé, la rampe tourne toute seule. Désactivé, Manual règle la hauteur à la main.

**Manual** Où se situe la hauteur entre PitchLow et PitchHigh tant que Auto est désactivé, pour qu'un bouton rotatif assigné hurle à la main.

**Hold** Fait sonner la sirène tant qu'il est enfoncé.

**Release** La vitesse à laquelle le son meurt quand tu lâches, en millisecondes.

**Level** Gain de sortie.

## Recette

**Sirène deux-tons** Mode Step, Wave Square, PitchLow 400, PitchHigh 530, LfoUp 0.4, LfoDown 0.4, Tone 3000. Assigne Hold à un pad, fais passer la sortie par un Fern avec une répétition courte et synchronisée, et appuie pendant deux mesures au début d'un drop.

## Organisms associés

Kick, LFO, Button
