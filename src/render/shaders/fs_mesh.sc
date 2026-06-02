$input v_normal, v_wpos

#include <bgfx_shader.sh>

void main()
{
    vec3 n = normalize(v_normal);
    // Simple headlight-ish directional light from upper-right-front.
    vec3 lightDir = normalize(vec3(0.4, 0.7, 0.6));
    float diff = max(dot(n, lightDir), 0.0);
    float ambient = 0.25;

    vec3 baseColor = vec3(0.70, 0.74, 0.80);
    vec3 color = baseColor * (ambient + diff * 0.85);

    gl_FragColor = vec4(color, 1.0);
}
