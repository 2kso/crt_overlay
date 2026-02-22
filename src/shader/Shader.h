#ifndef SHADER_H
#define SHADER_H

#define GL_GLEXT_PROTOTYPES 1

#include <GL/gl.h>
#include <GL/glext.h>
#include <string>

/**
 * @class Shader
 * @brief A simple wrapper around an OpenGL Shader Program.
 *
 * This class handles compiling the vertex and fragment shader source code,
 * linking them into an OpenGL program, and provides utility functions for
 * setting uniforms and using the program.
 */
class Shader {
public:
  // Program ID assigned by OpenGL
  GLuint ID;

  /**
   * @brief Constructs and compiles a shader program from source strings.
   * @param vertexCode The raw GLSL source for the Vertex Shader.
   * @param fragmentCode The raw GLSL source for the Fragment Shader.
   */
  Shader(const char *vertexCode, const char *fragmentCode);

  /**
   * @brief Activates the shader program for rendering.
   */
  void use() const;

  /**
   * @brief Utility function to pass a 2D float vector to a uniform variable.
   */
  void setVec2(const std::string &name, float x, float y) const;

  /**
   * @brief Utility function to pass a single float to a uniform variable.
   */
  void setFloat(const std::string &name, float value) const;

private:
  /**
   * @brief Helper function to compile an individual shader.
   */
  GLuint compileShader(GLenum type, const char *source);
};

#endif // SHADER_H
