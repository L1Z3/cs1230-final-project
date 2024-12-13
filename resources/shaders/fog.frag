#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D scene;  // Rendered scene texture
uniform vec3 fogColor;    // Fog color
uniform float fogStart;   // Start distance of fog
uniform float fogEnd;     // End distance of fog

void main() {
    // Fetch scene color
    vec4 sceneColor = texture(scene, TexCoords);

    // Use TexCoords.y as a proxy for distance; modify this to adjust fog behavior
    float depth = length(TexCoords.y);

    // Calculate fog factor
    float fogFactor = clamp((fogEnd - depth) / (fogEnd - fogStart), 0.0, 1.0);

    // Apply a more aggressive fog effect by using an exponential curve
    fogFactor = pow(fogFactor, 2.0); // Makes the fog transition sharper

    // Blend the fog color and scene color
    vec3 finalColor = mix(fogColor, sceneColor.rgb, fogFactor);
    FragColor = vec4(finalColor, sceneColor.a);
}
