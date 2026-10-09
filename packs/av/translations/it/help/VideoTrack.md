# VideoTrack

Una traccia video sulla timeline che riproduce le sue clip a tempo con il transport e registra quello che arriva al suo ingresso.

Aggiungine una dal menu di aggiunta tracce della timeline, oppure trascina un video sull'arrangiamento e comparirà una traccia apposta. Le clip si tagliano, si accorciano, si spostano, vanno in loop e hanno i fade come le clip audio, e l'uscita mostra quello che sta sotto la testina: se fermi il transport resta su quel fotogramma, se sposti la posizione salta lì, se metti il brano in loop va in loop anche lei. Warp adatta una clip al tempo del brano rispetto al tempo originale, Inverti la riproduce al contrario e i fade sono fade al nero. Ogni clip mostra una striscia di fotogrammi presi dal suo nastro, e con un clic destro su una clip trovi Info media, per leggere codec, dimensioni e frequenza fotogrammi. Collega l'uscita a un VideoOut, a un VideoFX o a un livello di un Lumen; due tracce sono due livelli.

## Registrazione

Collega all'ingresso un CameraIn, una catena di VideoFX, un VideoPad o un'altra traccia, arma la traccia con Record e premi Registra sul transport: quello che arriva all'ingresso viene scritto come video nella cartella delle registrazioni mentre il transport scorre, e finisce sulla timeline come clip, sul beat in cui è partito, un giro per ogni passata di un loop. Il suono non fa parte dell'immagine; arma una traccia audio accanto. La dimensione della registrazione si imposta in Impostazioni, sotto Video.

## Parametri

**Record** Arma la traccia. Mentre è armata, la prossima registrazione del transport scrive un take dall'ingresso.

**Monitor** Cosa mostra l'uscita. In mostra sempre l'ingresso, Off mostra sempre le clip, e Auto mostra l'ingresso mentre la traccia è armata o il transport è fermo, e le clip mentre scorre. Ha qualcosa tra cui scegliere solo quando all'ingresso è collegato qualcosa: senza niente lì, si vedono le clip, qualunque cosa dica questa impostazione.

**Opacity** Quanto si vede l'immagine della traccia. Sopra i livelli sottostanti in un Lumen, o sopra un'altra sorgente che condivide lo stesso ingresso, sfuma tra le due; senza niente sotto, sfuma l'immagine verso il nero. In ogni caso viene applicata una volta sola, ovunque arrivi.

**Blend** Come si combina con i livelli sottostanti: Normal, Add, Multiply o Screen. A differenza di Opacity, ha bisogno di qualcosa sotto con cui combinarsi, quindi non fa niente su una traccia collegata direttamente a un VideoOut o a un lato di un VideoMix.

## Ricetta

**Cattura dal vivo su un loop** Collega un CameraIn all'ingresso della traccia e l'uscita della traccia a un VideoOut. Imposta Monitor su Auto, arma Record, metti in loop quattro battute e premi Registra: vedi la videocamera mentre registra, e quando ti fermi il take viene riprodotto nell'arrangiamento, tagliato sul loop.

## Organism correlati

VideoPad, VideoPlayer, VideoOut, AudioTrack
