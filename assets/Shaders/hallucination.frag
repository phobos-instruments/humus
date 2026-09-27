// Humus native scene: the picture dreams. Each frame feeds back into the next,
// slowly swirling, growing its edges and bleeding its colours apart.
// Cord video into a Lumen inlet to dream over it; with none, the audio seeds it.
// Knob1 = dream (0 the live picture, 1 almost all feedback), Knob2 = swirl,
// Knob3 = growth, Knob4 = colour bleed.
void main() {
    float t = hum_Time;
    vec2 center = vec2(0.5);
    vec2 toCenter = uv - center;
    float dist = length(toCenter);

    // Turn a little around the centre, more near it, and pull the picture outward.
    float angle = (0.004 + 0.05 * hum_Knob2) * sin(t * 0.5) * (1.0 - dist);
    float c = cos(angle);
    float s = sin(angle);
    float pull = 0.997 - 0.003 * hum_Knob2 * hum_OnBeat;
    vec2 warped = center + mat2(c, -s, s, c) * toCenter * pull;

    // Read the last frame with the colour channels drifting apart.
    float split = 0.003 * sin(t) * (0.3 + hum_Knob4);
    vec3 dream = vec3(texture2D(hum_Feedback, warped + vec2(split, 0.0)).r,
                      texture2D(hum_Feedback, warped).g,
                      texture2D(hum_Feedback, warped - vec2(0.0, split)).b);

    // Push each pixel away from its neighbour so detail keeps growing.
    vec3 neighbour = texture2D(hum_Feedback, warped + vec2(0.002)).rgb;
    dream += (dream - neighbour) * (0.05 + 0.35 * hum_Knob3);
    dream = clamp(dream, 0.0, 1.0);
    dream.rg += vec2(0.004 * hum_Knob4 * sin(t + uv.x * 10.0));

    // Without video beneath, a soft ring that breathes with the bands seeds the dream.
    vec3 live = texture2D(hum_Input, uv).rgb;
    float bass = hum_Bands[1] + hum_Bands[2];
    vec2 square = toCenter * vec2(hum_Resolution.x / max(hum_Resolution.y, 1.0), 1.0);
    float ring = smoothstep(0.02, 0.0, abs(length(square) - 0.12 - 0.08 * bass));
    live += ring * (0.15 + 0.6 * hum_Level) * (0.5 + 0.5 * sin(6.28318 * (t * 0.05 + vec3(0.0, 0.33, 0.67))));

    float keep = 0.985 * (1.0 - pow(1.0 - clamp(hum_Knob1, 0.0, 1.0), 6.0));
    vec3 col = mix(live, dream, keep);
    gl_FragColor = vec4(col * hum_Brightness, 1.0);
}
