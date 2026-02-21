#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glx.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/shape.h>
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
    
    void main() {
        vec2 uv = gl_FragCoord.xy / resolution.xy;
        
        // CRT curvature
        vec2 p = uv * 2.0 - 1.0;
        p *= vec2(1.0 + (p.y * p.y) * 0.05, 1.0 + (p.x * p.x) * 0.05);
        
        // Vignette
        float dist = length(p);
        float vignette = smoothstep(1.3, 0.8, dist); // dark edges

        // If outside the curved coords, it's pure black
        if (abs(p.x) > 1.0 || abs(p.y) > 1.0) {
            FragColor = vec4(0.0, 0.0, 0.0, 1.0);
            return;
        }
        
        // Scanlines based on curved uv coordinates
        vec2 curved_uv = p * 0.5 + 0.5;
        // Frequency of scanlines
        float scanline = sin(curved_uv.y * resolution.y * 1.5) * 0.04 + 0.04;
        
        float edgeDarkening = (1.0 - vignette) * 0.6;
        float alpha = scanline + edgeDarkening;
        
        FragColor = vec4(0.0, 0.0, 0.0, clamp(alpha, 0.0, 1.0));
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
  glUniform2f(resLoc, (float)width, (float)height);

  bool running = true;
  XEvent xev;

  while (running) {
    while (XPending(display)) {
      XNextEvent(display, &xev);
      // No need to catch events basically, it's click-through and
      // override-redirect
    }

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    // Important: Use ONE instead of SRC_ALPHA to prevent the background desktop
    // from being rendered dark by the overlay's black pixels.
    // We want premultiplied alpha-like blending for the shader lines.
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(shaderProgram);
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
