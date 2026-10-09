# MidiBus

Un punto di raccolta MIDI senza controlli: tanti cavi MIDI in ingresso, un solo flusso ordinato nel tempo in uscita.

Per distribuire non serve mai un organism, perché un'uscita MIDI si può collegare a quanti strumenti vuoi; il MidiBus serve per raccogliere. Collegaci controller e sequencer e fanne uscire un cavo per ogni strumento: per reindirizzare tutto basta poi spostare un cavo. Gli eventi di tutti gli ingressi vengono uniti in ordine di tempo dentro ogni blocco. Il menu a tendina Inputs nell'editor delle proprietà decide quanti ingressi ha, da 2 a 8, e ridimensiona il bus sul posto mantenendo nome, cavi e automazione. Un Tuning collegato a un MidiBus vale per ogni strumento a valle, ed è quindi il posto giusto per dare una sola scala a un'intera sezione.

## Ricetta

**Tastiera più sequencer** Collega un MidiIn e un Microdot a un MidiBus e il bus a un Rhizome. Le note suonate e quelle in sequenza arrivano alla stessa voce, e un Tuning su un terzo ingresso le riaccorda entrambe insieme.

## Organism correlati

Bus, MidiIn, MidiOut, Tuning
