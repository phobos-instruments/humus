# VideoOut

La finestra o lo schermo da cui l'immagine esce dalla patch.

Il gemello video di SoundOut. Collega al suo ingresso un composito di Lumen, un VideoMix, un VideoFX o un semplice VideoPlayer, e apri la sua finestra con il pulsante schermo accanto al menu Screen, dal sottomenu Uscite video o con un clic destro sull'organism. Come finestra resta sospesa sopra il patcher mentre lavori; mandata a uno schermo, lì diventa senza bordi e a tutto schermo. Più organism VideoOut sono più uscite, così lo spettacolo può andare a un proiettore mentre un mix di anteprima resta sul tuo schermo.

## Parametri

**Screen** Dove va l'immagine. Window apre una finestra ridimensionabile che puoi spostare; Display 1, Display 2 o Display 3 occupa a tutto schermo quello schermo collegato, ed è l'impostazione per il proiettore.

**Fade** Il fader generale di tutta l'uscita. 0 è nero; pilotalo da un'automazione o da un controller per chiudere lo spettacolo in dissolvenza.

**Preview** Tiene viva la piccola anteprima nell'editor dell'organism mentre la finestra d'uscita è chiusa. Disattivalo per risparmiare lavoro grafico quando non ti serve.

Un'uscita impostata su uno schermo si riapre da sola quando carichi la patch, su quello schermo, riempiendolo. Una lasciata su Window no: nella patch non c'è niente che dica se la sua finestra era aperta, solo dove andrebbe.

**OnTop** Tiene la finestra d'uscita sopra le altre finestre, così se torni a cliccare nella patch l'immagine non finisce sotto. Attivo di default; un'uscita che riempie uno schermo resta in primo piano qualunque cosa dica questo parametro.

## Ricetta

**Proiettore più anteprima** Collega l'uscita di Lumen a due organism VideoOut. Imposta uno su Display 2 per il proiettore e lascia l'altro su Window sul tuo schermo, con Preview attivo. Mappa il Fade dell'uscita del proiettore su un fader, così puoi portare la sala al nero senza toccare il mix.

## Organism correlati

Lumen, VideoMix, VideoPlayer
