// Humus native scene: an oscilloscope + spectrum floor, straight from the
// hum_Wave / hum_Spectrum textures (one row, value in .r).
// Knob1 = trace glow, Knob2 = hue, Knob3 = spectrum height, Knob4 = fade.
void main() {
    vec2 p = uv;
    vec3 col = vec3(0.0);

    // Waveform trace across the middle: distance to the sampled amplitude.
    float smp = texture2D(hum_Wave, vec2(p.x, 0.5)).r;        // 0..1, 0.5 = silence
    float trace = abs(p.y - smp);
    col += (0.004 + 0.02 * hum_Knob1) / (trace * trace + 0.001)
           * 0.02 * (0.5 + 0.5 * sin(6.28318 * hum_Knob2 + vec3(0.0, 2.1, 4.2)));

    // Spectrum floor rising from the bottom.
    float mag = texture2D(hum_Spectrum, vec2(p.x * 0.5, 0.5)).r;   // musical half
    float bar = step(p.y, mag * (0.2 + 0.6 * hum_Knob3));
    col += bar * vec3(0.1, 0.5, 0.6) * (0.3 + 0.7 * mag);

    // Beat flash washes the whole frame, fading per Knob4.
    col += vec3(0.5, 0.35, 0.15) * hum_OnBeat * 0.15 * (1.0 - hum_Knob4);
    col *= hum_Brightness;
    gl_FragColor = vec4(col, 1.0);
}
