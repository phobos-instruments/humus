# CameraIn

Una videocamera come sorgente video.

Attiva Enabled e collega l'uscita video a un layer di Lumen, a un VideoMix, a un VideoFX o a un VideoOut: l'immagine dal vivo diventa un layer come qualsiasi nastro. La videocamera si apre solo finché Enabled è attivo e si chiude quando lo disattivi; i fotogrammi vanno allo schermo e da nessun'altra parte. È un'immagine, non un controller: per suonare qualcosa con una videocamera, collega questo organism a Hands o a Skeleton, oppure lascia che siano loro ad aprire la videocamera.

## Parametri

**Enabled** Apre la videocamera. Disattivato chiude il dispositivo, non si limita a nascondere l'immagine.

**Camera** Quale dispositivo di acquisizione aprire, quando il computer ne ha più di uno.

**Mirror** Specchia l'immagine in orizzontale, così se ti sporgi a sinistra ti sposti a sinistra anche sullo schermo. Attivalo quando ti esibisci davanti alla videocamera.

**Opacity** Quanto questo layer si vede sopra quelli sottostanti in un Lumen.

**Blend** Come il layer si combina con quelli sottostanti: Normal, Add, Multiply o Screen.

## Ricetta

**Videocamera sotto una scena** Enabled attivo, Mirror attivo, poi collega l'uscita video al primo ingresso video di Lumen e Lumen a un VideoOut. Carica uno shader di scena in Lumen, imposta SceneBlend su Add e SceneOpacity a circa 0.6, così la scena brilla sopra l'immagine dal vivo.

## Organism correlati

Lumen, VideoMix, VideoOut, Hands
