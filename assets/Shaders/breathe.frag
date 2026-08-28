// Humus native scene: a breathing aurora that follows the mix.
// Point a Lumen's Scene param at this file; edits hot-reload on save.
// Knob1 = band spread, Knob2 = hue, Knob3 = grain, Knob4 = drift.
void main() {
    vec2 p = uv * 2.0 - 1.0;
    p.x *= hum_Resolution.x / max(hum_Resolution.y, 1.0);
    float t = hum_Time * (0.3 + 0.7 * hum_Knob4);

    // Eight ribbons, one per spectral band, stacked bottom (sub) to top (air).
    vec3 col = vec3(0.0);
    for (int i = 0; i < 8; ++i) {
        float fi = float(i);
        float y = -0.8 + fi * 0.23 * (0.5 + hum_Knob1);
        float band = hum_Bands[i];
        float wave = sin(p.x * (2.0 + fi) + t * (1.0 + 0.2 * fi)) * 0.15 * (0.2 + band);
        float d = abs(p.y - y - wave);
        float glow = band / (1.0 + 40.0 * d * d);
        col += glow * (0.5 + 0.5 * sin(6.28318 * (fi / 8.0 + hum_Knob2)
                                       + vec3(0.0, 2.1, 4.2)));
    }
    col += 0.08 * sin(p.y * (40.0 + 80.0 * hum_Knob3) + t * 3.0) * hum_Level;
    col *= hum_Brightness * (0.6 + 0.4 * hum_OnBeat);
    gl_FragColor = vec4(col, 1.0);
}
