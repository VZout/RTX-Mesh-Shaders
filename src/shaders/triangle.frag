#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 frag_color;

layout(location = 0) out vec4 out_color;

const vec2 res = vec2(1280, 1280);
const float zoom = 20;

float Hash21(vec2 p)
{
    p = fract(p * vec2(234.24, 435.346));
    p += dot(p, p + 34.23);
    return fract(p.x * p.y);
}

void main()
{
    float time = 0;
    vec3 color = vec3(0);
    const vec2 screen_uv = gl_FragCoord.xy / res;
    const vec2 uv = ((gl_FragCoord.xy - 0.5 * res) / res) * zoom;

    const float line_width = 0.15;

    vec2 id = floor(uv); // the id of the tile
    vec2 gv = fract(uv) - 0.5; // pixel coordinate

    float n = Hash21(id); // rand between 0 and 1

    if (n < 0.5) gv.x *= -1; // Flip tile at random
    //float dist = abs(abs(gv.x + gv.y) - 0.5);
    vec2 c_uv = gv - sign(gv.x + gv.y + 0.001) * 0.5; // circle_uv
    float dist = length(c_uv);
    float mask = smoothstep(0.01, -0.01, abs(dist - 0.5) - line_width);
    float angle = atan(c_uv.x, c_uv.y); // -pi to pi

    // Animation
    float checker  = mod(id.x + id.y, 2.0) * 2.0 - 1.0;
    float flow = sin(time + checker * angle * 10);

    // proper uv the truchet
    float x = fract(angle / 1.57);
    float y = (dist - (0.5 - line_width)) / (line_width * 2.0);
    y = abs(y - 0.5) * 2.0;
    vec2 t_uv = vec2(x, y); // truchet_uv

    color.rg += t_uv * mask;

    // Red borders
    //if (gv.x > 0.48 || gv.y > 0.48) color = vec3(1, 0, 0);

    out_color = vec4(color, 1.0);
}
