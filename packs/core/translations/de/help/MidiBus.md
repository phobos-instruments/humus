# MidiBus

Ein MIDI-Sammelpunkt ohne Bedienelemente: mehrere MIDI-Kabel hinein, ein zeitlich sortierter Strom heraus.

Zum Verteilen brauchst du nie ein Organism, denn ein MIDI-Ausgang lässt sich mit beliebig vielen Instrumenten verbinden; der MidiBus ist zum Zusammenführen da. Verbinde Controller und Sequencer mit ihm und führe pro Instrument ein Kabel aus ihm heraus, dann musst du zum Umleiten nur noch ein Kabel umstecken. Events aus allen Eingängen werden innerhalb jedes Blocks in zeitlicher Reihenfolge zusammengeführt. Das Auswahlmenü Inputs in den Eigenschaften legt fest, wie viele Eingänge er hat, 2 bis 8, und ändert die Größe des Busses an Ort und Stelle, wobei Name, Kabel und Automation erhalten bleiben. Ein Tuning, das mit einem MidiBus verbunden ist, gilt für jedes Instrument dahinter, und das macht ihn zum richtigen Ort für eine einzige Tonleiter für eine ganze Sektion.

## Rezept

**Keyboard plus Sequencer** Verbinde ein MidiIn und ein Microdot mit einem MidiBus und den Bus mit einem Rhizome. Gespielte und sequenzierte Noten erreichen dieselbe Stimme, und ein Tuning an einem dritten Eingang stimmt beide auf einmal um.

## Verwandte Organisms

Bus, MidiIn, MidiOut, Tuning
