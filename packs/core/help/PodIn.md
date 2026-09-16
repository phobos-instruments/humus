# PodIn

A pod's way in: one inlet pin on the pod's box, carried to an outlet inside.

Everything corded into the pod's box from outside flows through a PodIn and appears on its outlet inside, one PodIn per pin in the order they were made. Add one from the pod canvas's right-click menu or from the Pod category, and cord its outlet to whatever inside the pod should hear it. The port's editor is a single dropdown that re-classes it in place between Mono, Stereo, MIDI, Video and Control; the port keeps its name so every cord survives, and the pod's box grows a matching pin. A Control port is a socket on the pod's box: a Number, an LFO or a Follower corded onto it from outside appears on the port's control outlet inside, ready to draw onto any socket in the pod. A Video port carries frames rather than sound and passes them on unchanged.

## Recipe

**Wrap an effect chain** Make a pod, add a Stereo PodIn and a Stereo PodOut, and cord the PodIn to a Fern and the Fern to the PodOut. Add a Control PodIn and draw its outlet onto the Fern's Mix; outside, the pod's box now shows a stereo inlet, a stereo outlet and a control socket that an LFO can drive.

## Related Organisms

PodOut, Number, LFO
