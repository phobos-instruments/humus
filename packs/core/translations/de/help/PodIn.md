# PodIn

Der Weg in einen Pod: ein Eingangs-Pin an der Box des Pods, nach innen zu einem Ausgang durchgereicht.

Alles, was von außen per Kabel in die Box des Pods geht, fließt durch ein PodIn und kommt drinnen an dessen Ausgang heraus, ein PodIn pro Pin, in der Reihenfolge, in der sie angelegt wurden. Füg eines über das Rechtsklickmenü der Pod-Arbeitsfläche oder aus der Kategorie Pod hinzu und verbinde seinen Ausgang mit dem, was im Pod sein Signal hören soll. Der Editor des Ports ist ein einziges Dropdown, das ihn an Ort und Stelle zwischen Mono, Stereo, MIDI, Video und Steuerung umstellt; der Port behält seinen Namen, sodass jedes Kabel erhalten bleibt, und die Box des Pods bekommt einen passenden Pin. Ein Steuerungs-Port ist eine Buchse an der Box des Pods: Verbindest du von außen ein Number, einen LFO oder einen Follower damit, kommt das Signal drinnen am Steuerausgang des Ports heraus, bereit, auf eine beliebige Buchse im Pod gezogen zu werden. Ein Video-Port trägt Frames statt Klang und reicht sie unverändert weiter.

## Rezept

**Eine Effektkette einpacken** Erstell einen Pod, füg ein Stereo-PodIn und ein Stereo-PodOut hinzu und verbinde das PodIn mit einem Fern und das Fern mit dem PodOut. Füg ein PodIn vom Typ Steuerung hinzu und zieh seinen Ausgang auf Mix am Fern; außen zeigt die Box des Pods jetzt einen Stereo-Eingang, einen Stereo-Ausgang und eine Steuerbuchse, die ein LFO antreiben kann.

## Verwandte Organisms

PodOut, Number, LFO
