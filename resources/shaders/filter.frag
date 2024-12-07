#version 330 core

in vec2 uv;

uniform sampler2D myTexture; // Renamed to avoid conflict with GLSL function names

uniform bool enableInvert;
uniform bool enableBoxBlur;
uniform int screenWidth;
uniform int screenHeight;

out vec4 fragColor;

void main() {
    fragColor = vec4(1);
    // Set fragColor using the sampler2D at the UV coordinate
    fragColor = texture(myTexture, uv); // Use modern GLSL function `texture`

    // Box blur
    if (enableBoxBlur) {
        vec2 deltaUV = vec2(1.0 / float(screenWidth), 1.0 / float(screenHeight));
        // 5x5 box
        vec3 sum = vec3(0);
        for (int x = -2; x <= 2; x++) {
            for (int y = -2; y <= 2; y++) {
                vec2 offset = vec2(x, y);
                sum += vec3(texture(myTexture, uv + offset * deltaUV)); // Use `texture`
            }
        }
        sum = sum / 25.0;
        fragColor = vec4(sum, 1.0);
    }

    // Invert fragColor's r, g, and b color channels if your bool is true
    if (enableInvert) {
        fragColor.rgb = 1.0 - fragColor.rgb; // Shorter and cleaner inversion
    }
}
