#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glx.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/shape.h>
#include <chrono>
#include <iostream>
#include <string.h>
#include <unistd.h>

const char *vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec2 aTexCoords;
    out vec2 TexCoords;
    void main() {
        gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
        TexCoords = aTexCoords;
    }
)";

const char *fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    in vec2 TexCoords;
    uniform vec2 resolution;
    uniform float time; // Passing time for dynamic effects
    
    // Pseudo-random noise function
    float rand(vec2 co) {
        return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
    }

    void main() {
        vec2 uv = gl_FragCoord.xy / resolution.xy;
        
        // --- 1. CRT Curvature ---
        vec2 p = uv * 2.0 - 1.0;
        p *= vec2(1.0 + (p.y * p.y) * 0.04, 1.0 + (p.x * p.x) * 0.05);

        // Outside curved area is pure black
        if (abs(p.x) > 1.0 || abs(p.y) > 1.0) {
            FragColor = vec4(0.0, 0.0, 0.0, 1.0);
            return;
        }

        vec2 curved_uv = p * 0.5 + 0.5;

        // --- 2. Vignette ---
        float dist = length(p);
        float vignette = smoothstep(1.5, 0.5, dist); 

        // --- 3. Scanlines ---
        float fine_lines = sin(curved_uv.y * resolution.y * 2.5) * 0.08;
        float roll_bands = sin(curved_uv.y * 15.0 - time * 6.0) * 0.08;
        float scanline = fine_lines + roll_bands + 0.08;

        // --- 4. Pixelization Grid (Subpixel mask gap) ---
        vec2 pixel_size = vec2(3.0, 3.0); 
        vec2 grid = mod(gl_FragCoord.xy, pixel_size);
        float grid_alpha = 0.0;
        if (grid.x < 1.0 || grid.y < 1.0) {
            grid_alpha = 0.35; 
        }

        // --- 5. Glitches & Artifacts ---

        // A. Static Noise (TV Snow)
        // Add random high-frequency dark speckles
        float noise = rand(gl_FragCoord.xy * time) * 0.15;

        // B. Luminance Flickering
        // Occasional rapid flashes of brightness/darkness over the whole screen
        float flicker = sin(time * 30.0) * sin(time * 45.0) * 0.03;

        // C. VHS Tracking Line (Glitch Band)
        // A thick band of distorted noise that rolls down the screen occasionally
        // `track_pos` loops from 1.5 to -0.5, ensuring it sweeps across the screen and then disappears for a bit.
        float track_pos = mod(time * 0.2, 2.0) - 0.5; 
        float distance_to_track = abs(curved_uv.y - track_pos);
        
        // When the Y coordinate is near the tracking band, increase the noise heavily
        float tracking_distort = 0.0;
        if (distance_to_track < 0.05) {
            // Intense noise inside the tracking band
            tracking_distort = rand(vec2(gl_FragCoord.y, time)) * 0.4;
        }

        // --- 6. Compositing ---
        float edgeDarkening = (1.0 - vignette) * 0.8; 
        float final_alpha = scanline + grid_alpha + edgeDarkening + noise + flicker + tracking_distort;
        
        // Output pure black, using alpha to draw the dark artifacts on the screen
        FragColor = vec4(0.0, 0.0, 0.0, clamp(final_alpha, 0.0, 1.0));
    }
)";

GLuint compileShader(GLenum type, const char *source) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);

  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(shader, 512, nullptr, infoLog);
    std::cerr << "Shader Compilation Error:\n" << infoLog << std::endl;
  }
  return shader;
}

// Function to find an ARGB visual
XVisualInfo *findARGBVisual(Display *display, int screen) {
  XVisualInfo *vinfo = new XVisualInfo;
  if (XMatchVisualInfo(display, screen, 32, TrueColor, vinfo)) {
    return vinfo;
  }
  delete vinfo;
  return nullptr;
}

int main() {
  Display *display = XOpenDisplay(NULL);
  if (!display) {
    std::cerr << "Failed to open display" << std::endl;
    return -1;
  }

  int screen = DefaultScreen(display);
  Window root = RootWindow(display, screen);

  XVisualInfo *vinfo = findARGBVisual(display, screen);
  if (!vinfo) {
    std::cerr << "No appropriate ARGB visual found. A composite manager is "
                 "likely required."
              << std::endl;
    return -1;
  }

  Colormap colormap = XCreateColormap(display, root, vinfo->visual, AllocNone);

  XSetWindowAttributes attr;
  memset(&attr, 0, sizeof(attr));
  attr.colormap = colormap;
  attr.border_pixel = 0;
  attr.background_pixel = 0; // transparent background
  attr.override_redirect =
      True; // bypass window manager (stay on top, no borders)

  int width = DisplayWidth(display, screen);
  int height = DisplayHeight(display, screen);

  Window window = XCreateWindow(
      display, root, 0, 0, width, height, 0, vinfo->depth, InputOutput,
      vinfo->visual,
      CWColormap | CWBorderPixel | CWBackPixel | CWOverrideRedirect, &attr);

  // XFixes click-through
  XserverRegion region = XFixesCreateRegion(display, NULL, 0);
  XFixesSetWindowShapeRegion(display, window, ShapeInput, 0, 0, region);
  XFixesDestroyRegion(display, region);

  // Create GLX Context
  GLXContext glc = glXCreateContext(display, vinfo, NULL, GL_TRUE);
  if (!glc) {
    std::cerr << "Failed to create GLX context" << std::endl;
    return -1;
  }

  glXMakeCurrent(display, window, glc);
  XMapWindow(display, window);

  // Setup Quad
  float vertices[] = {-1.0f, 1.0f, 0.0f, 1.0f,  -1.0f, -1.0f,
                      0.0f,  0.0f, 1.0f, -1.0f, 1.0f,  0.0f,

                      -1.0f, 1.0f, 0.0f, 1.0f,  1.0f,  -1.0f,
                      1.0f,  0.0f, 1.0f, 1.0f,  1.0f,  1.0f};

  GLuint VBO, VAO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        (void *)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);

  GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
  GLuint fragmentShader =
      compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  glUseProgram(shaderProgram);
  GLint resLoc = glGetUniformLocation(shaderProgram, "resolution");
  GLint timeLoc = glGetUniformLocation(shaderProgram, "time");
  glUniform2f(resLoc, (float)width, (float)height);

  bool running = true;
  XEvent xev;

  // Need higher precision timer for the "time" uniform
  auto startTime = std::chrono::high_resolution_clock::now();

  while (running) {
    while (XPending(display)) {
      XNextEvent(display, &xev);
      // No need to catch events basically, it's click-through and
      // override-redirect
    }

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    // Standard alpha blending to composite over the desktop correctly
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(shaderProgram);

    auto currentTime = std::chrono::high_resolution_clock::now();
    float timeSecs =
        std::chrono::duration<float>(currentTime - startTime).count();
    glUniform1f(timeLoc, timeSecs);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glXSwapBuffers(display, window);
    usleep(16000); // ~60 FPS
  }

  glXMakeCurrent(display, None, NULL);
  glXDestroyContext(display, glc);
  XDestroyWindow(display, window);
  XCloseDisplay(display);

  return 0;
}
