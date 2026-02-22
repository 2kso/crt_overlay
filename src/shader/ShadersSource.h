#ifndef SHADERS_SOURCE_H
#define SHADERS_SOURCE_H

/**
 * @file ShadersSource.h
 * @brief Contains the raw GLSL shader strings used to render the CRT overlay.
 */

static const char *vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec2 aTexCoords;
    out vec2 TexCoords;
    
    void main() {
        gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
        TexCoords = aTexCoords;
    }
)";

static const char *fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    in vec2 TexCoords;
    
    uniform vec2 resolution; // Screen resolution
    uniform float time;      // Time in seconds for dynamic glitches
    
    // Pseudo-random noise function for static snow
    float rand(vec2 co) {
        return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
    }

    void main() {
        // Normalize coordinates
        vec2 uv = gl_FragCoord.xy / resolution.xy;
        
        // --- 1. CRT Curvature ---
        vec2 p = uv * 2.0 - 1.0;
        p *= vec2(1.0 + (p.y * p.y) * 0.04, 1.0 + (p.x * p.x) * 0.05);

        // Discard pixels completely outside the curved monitor area (pure black)
        if (abs(p.x) > 1.0 || abs(p.y) > 1.0) {
            FragColor = vec4(0.0, 0.0, 0.0, 1.0);
            return;
        }

        vec2 curved_uv = p * 0.5 + 0.5;

        // --- 2. Vignette (Darkened Edges) ---
        float dist = length(p);
        float vignette = smoothstep(1.5, 0.5, dist); 

        // --- 3. Scanlines ---
        // A combination of static fine horizontal lines and slowly rolling wide bands
        float fine_lines = sin(curved_uv.y * resolution.y * 2.5) * 0.08;
        float roll_bands = sin(curved_uv.y * 15.0 - time * 6.0) * 0.08;
        float scanline = fine_lines + roll_bands + 0.08;

        // --- 4. Pixelization Grid (Subpixel gap mask) ---
        // Emulates physical gaps between RGB pixels on a low-res TV
        vec2 pixel_size = vec2(3.0, 3.0); 
        vec2 grid = mod(gl_FragCoord.xy, pixel_size);
        float grid_alpha = 0.0;
        
        // Darken the top/left edges of each 3x3 pixel block
        if (grid.x < 1.0 || grid.y < 1.0) {
            grid_alpha = 0.35; 
        }

        // --- 5. Organic Glitches & Artifacts ---

        // A. Static Noise (TV Snow)
        // High-frequency dark speckles
        float noise = rand(gl_FragCoord.xy * time) * 0.15;

        // B. Luminance Flickering
        // Occasional rapid flashes of brightness/darkness over the whole screen
        float flicker = sin(time * 30.0) * sin(time * 45.0) * 0.03;

        // C. VHS Tracking Line (Glitch Band)
        // A thick band of distorted noise that rolls down the screen occasionally
        float track_pos = mod(time * 0.2, 2.0) - 0.5; 
        float distance_to_track = abs(curved_uv.y - track_pos);
        
        float tracking_distort = 0.0;
        // When the Y coordinate is near the tracking band, increase the noise heavily
        if (distance_to_track < 0.05) {
            tracking_distort = rand(vec2(gl_FragCoord.y, time)) * 0.4;
        }

        // --- 6. Compositing ---
        // Alpha controls how much the desktop is darkened.
        float edgeDarkening = (1.0 - vignette) * 0.8; 
        float final_alpha = scanline + grid_alpha + edgeDarkening + noise + flicker + tracking_distort;
        
        // Output pure black, using alpha to draw the dark CRT artifacts over the desktop
        FragColor = vec4(0.0, 0.0, 0.0, clamp(final_alpha, 0.0, 1.0));
    }
)";

#endif // SHADERS_SOURCE_H
