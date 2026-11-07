#version 330 core

in vec2 BeamCoords;
out vec4 color;

uniform vec3 beamColor;
uniform float beamAlpha;

void main()
{
    // Centro luminoso y bordes suaves para evitar que el haz parezca una pared.
    float center = 1.0 - abs(BeamCoords.x * 2.0 - 1.0);
    float edgeFade = smoothstep(0.0, 0.42, center);
    float startFade = smoothstep(0.0, 0.08, BeamCoords.y);
    float endFade = 1.0 - 0.18 * smoothstep(0.82, 1.0, BeamCoords.y);
    float alpha = beamAlpha * edgeFade * startFade * endFade;
    if (alpha < 0.01) discard;
    color = vec4(beamColor, alpha);
}
