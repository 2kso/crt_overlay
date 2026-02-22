#include "shader/Shader.h"
#include "shader/ShadersSource.h"
#include "window/OverlayWindow.h"
#include <chrono>
#include <iostream>
#include <unistd.h>

/**
 * @brief Sets up a full screen quad for the fragment shader to render onto.
 * @param VAO Reference to the Vertex Array Object ID to generate.
 * @param VBO Reference to the Vertex Buffer Object ID to generate.
 */
void setupQuad(GLuint &VAO, GLuint &VBO) {
  float vertices[] = {// positions   // texCoords
                      -1.0f, 1.0f, 0.0f, 1.0f,  -1.0f, -1.0f,
                      0.0f,  0.0f, 1.0f, -1.0f, 1.0f,  0.0f,

                      -1.0f, 1.0f, 0.0f, 1.0f,  1.0f,  -1.0f,
                      1.0f,  0.0f, 1.0f, 1.0f,  1.0f,  1.0f};

  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // Position attribute
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  // Texture Coordinate attribute
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        (void *)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);
}

int main() {
  try {
    // 1. Initialize the transparent, click-through X11 window
    OverlayWindow overlay;
    std::cout << "Successfully created transparent CRT overlay resolution: "
              << overlay.width << "x" << overlay.height << std::endl;

    // 2. Setup the OpenGL shader program and screen quad
    Shader crtShader(vertexShaderSource, fragmentShaderSource);
    GLuint VAO, VBO;
    setupQuad(VAO, VBO);

    // Configure static shader uniforms
    crtShader.use();
    crtShader.setVec2("resolution", (float)overlay.width,
                      (float)overlay.height);

    // 3. Main Application Loop
    bool running = true;
    auto startTime = std::chrono::high_resolution_clock::now();

    while (running) {
      // Check for OS events (essentially a no-op since window is click-through)
      running = overlay.handleEvents();

      // Clear screen physically to transparent (alpha = 0)
      glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
      glClear(GL_COLOR_BUFFER_BIT);

      // Setup alpha blending so the dark CRT lines actually darken the desktop
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      // Update dynamic uniforms (time for glitches/scrolling)
      crtShader.use();
      auto currentTime = std::chrono::high_resolution_clock::now();
      float timeSecs =
          std::chrono::duration<float>(currentTime - startTime).count();
      crtShader.setFloat("time", timeSecs);

      // Render the full screen quad
      glBindVertexArray(VAO);
      glDrawArrays(GL_TRIANGLES, 0, 6);

      // Complete the frame
      overlay.swapBuffers();
      usleep(16000); // Sleep roughly equivalent to ~60 FPS
    }

    // Cleanup OpenGL
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

  } catch (const std::exception &e) {
    std::cerr << "Fatal Error: " << e.what() << std::endl;
    return -1;
  }

  return 0;
}
