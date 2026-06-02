$input a_position, a_normal
$output v_normal, v_wpos

#include <bgfx_shader.sh>

void main()
{
    vec4 wpos = mul(u_model[0], vec4(a_position, 1.0));
    v_wpos = wpos.xyz;
    // Normal in world space (model assumed rigid/uniform-scale for now).
    v_normal = mul(u_model[0], vec4(a_normal, 0.0)).xyz;
    gl_Position = mul(u_viewProj, wpos);
}
