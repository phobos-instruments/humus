#pragma once
#include <juce_core/juce_core.h>

#include "core/IsfParse.h"

namespace hum {
namespace shk {

inline bool looksLikeShk(const juce::String& src) {
    return (src.contains("v_tex_coord") || src.contains("SKDefaultShading"))
           && src.contains("void main(");
}

inline juce::String wrap(const juce::String& src) {
    juce::String pre;
    if (!src.contains("#version")) pre << "#version 120\n";
    pre <<
        "uniform vec2 a_size;\n"
        "#define v_tex_coord (gl_FragCoord.xy / a_size)\n"
        "#define v_color_mix vec4(1.0)\n"
        "uniform sampler2D u_texture;\n"
        "uniform float u_time;\n"
        "uniform float u_strength;\n"
        "uniform float u_speed;\n"
        "uniform float u_frequency;\n"
        "uniform float u_width;\n"
        "uniform float u_brightness;\n"
        "uniform float u_density;\n"
        "uniform float u_rows;\n"
        "uniform float u_cols;\n"
        "uniform float u_red;\n"
        "uniform float u_group_size;\n"
        "uniform vec2 u_center;\n"
        "uniform vec4 u_color;\n"
        "uniform vec4 u_first_color;\n"
        "uniform vec4 u_second_color;\n"
        "vec4 SKDefaultShading() { return texture2D(u_texture, gl_FragCoord.xy / a_size); }\n";
    return assembleGlsl(pre, src);
}

}
}
