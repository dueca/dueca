/* ------------------------------------------------------------------   */
/*      item            : TextRenderer.hxx
        made by         : Rene van Paassen
        date            : 250125
        category        : header file
        description     : from https://learnopengl.com Joey de Vries
                          CC BY-NC 4.0
        changes         : 250125 first version
        language        : C++
        copyright       : (c) 25 TUDelft-AE-C&S
*/

#pragma once

#include "Shader.hxx"
#include <map>
#include <glm/glm.hpp>
#include <string>

/** Renders text from a certain font. Create the renderer with an opengl context
    active */
class TextRenderer
{
  /** Shader for this text */
  Shader s;

  /** Buffer object */
  unsigned vbo;

  /** Array object */
  unsigned vao;

  /** Updating the projection matrix */
  Shader::link<glm::mat4> projection_link;

  /** Updating the color */
  Shader::link<glm::vec3> color_link;

  /** character struct type */
  struct Character
  {
    unsigned int TextureID; // ID handle of the glyph texture
    glm::ivec2 Size; // Size of glyph
    glm::ivec2 Bearing; // Offset from baseline to left/top of glyph
    unsigned int Advance; // Offset to advance to next glyph
  };

  /** Character map */
  std::map<char, Character> characters;

public:
  /** Constructor. Create this in an active GL context, have GL_BLEND, and blend
      func enabled

      @param font   Font file.
  */
  TextRenderer(const char *font);

  /** Render some text, at location x, y (baseline left) */
  void renderText(const std::string &text, float x, float y, float scale,
                  glm::vec3 color);

  /** Set the projection matrix for text rendering. */
  inline void setProjection(const glm::mat4 &p)
  {
    s.use();
    projection_link.update(p);
  }

  /** Destructor.

      Make sure your gc is current when the destructor is called. */
  ~TextRenderer();
};
