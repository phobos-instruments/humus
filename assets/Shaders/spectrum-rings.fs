/*{
    "ISFVSN": "2",
    "DESCRIPTION": "Concentric rings driven by the live spectrum (Humus Lumen example)",
    "CREDIT": "Humus",
    "CATEGORIES": ["Generator"],
    "INPUTS": [
        {"NAME": "rings", "TYPE": "float", "MIN": 4.0, "MAX": 40.0, "DEFAULT": 16.0},
        {"NAME": "spin", "TYPE": "float", "MIN": -2.0, "MAX": 2.0, "DEFAULT": 0.4},
        {"NAME": "tint", "TYPE": "color", "DEFAULT": [0.3, 0.8, 1.0, 1.0]},
        {"NAME": "spectrumImage", "TYPE": "audioFFT"}
    ]
}*/
// In Humus, `rings` and `spin` ride Knob1/Knob2 (rescaled onto MIN..MAX);
// the FFT arrives as a one-row texture, magnitude in .r.
void main() {
    vec2 p = isf_FragNormCoord * 2.0 - 1.0;
    p.x *= RENDERSIZE.x / max(RENDERSIZE.y, 1.0);
    float r = length(p);
    float a = atan(p.y, p.x) + TIME * spin;

    // Radius reads the spectrum with a curve, not straight: linear puts the
    // outer four fifths of the screen on the top octaves, which carry almost
    // no energy, and the rings there never light.
    float bin = pow(clamp(r, 0.0, 1.0), 2.5);
    float mag = IMG_NORM_PIXEL(spectrumImage, vec2(bin, 0.5)).r;
    mag = sqrt(clamp(mag, 0.0, 1.0));

    float ring = abs(fract(r * rings - TIME * 0.5) - 0.5) * 2.0;
    float shape = 1.0 / (0.08 + ring * ring * 6.0);
    float spokes = 0.5 + 0.5 * sin(a * 6.0);

    // The rings stand on their own and the spectrum lights them. Multiplying
    // by magnitude alone left the whole scene black whenever nothing played.
    float glow = shape * (0.12 + 1.6 * mag) * exp(-r * 0.8);

    vec3 col = tint.rgb * glow * (0.6 + 0.4 * spokes);
    gl_FragColor = vec4(col, 1.0);
}
