# PodOut

A pod's way out: one outlet pin on the pod's box, fed from an inlet inside.

Whatever is corded into a PodOut inside the pod appears on the pod box's matching outlet pin outside, one PodOut per pin in the order they were made. Add one from the pod canvas's right-click menu or from the Pod category. The port's editor is a single dropdown that re-classes it in place between Mono, Stereo, MIDI, Video and Control; the port keeps its name so every cord survives, and the pod's box grows a matching pin. A Control PodOut is the mirror of the Control PodIn: draw any control outlet inside the pod onto its socket and the box grows a control outlet that carries the value outside, so a pod can be a control source of its own. A Video port carries frames rather than sound and passes them on unchanged.

## Recipe

**Pod as a control source** Inside a pod, cord a Stereo PodIn to a Follower and draw the Follower's env outlet onto a Control PodOut. Outside, cord the kick into the pod and draw the pod's new control outlet onto a Gain on the bass: the pod ducks the bass from whatever it is listening to.

## Related Organisms

PodIn, Follower, Gain
