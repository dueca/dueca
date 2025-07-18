/* ------------------------------------------------------------------   */
/*      item            : TextRenderer.cxx
        made by         : Rene' van Paassen
        date            : 250125
        category        : body file
        description     : from https://learnopengl.com Joey de Vries
                          CC BY-NC 4.0
        changes         : 250125 first version
        language        : C++
        copyright       : (c) 2025 DUECA Authors EUPL-1.2
*/

#include "TextRenderer.hxx"
#include <ft2build.h>
#include <iostream>
#include FT_FREETYPE_H

TextRenderer::TextRenderer(const char *font) :
  s("../text_vrt.glsl",
    "../text_frg.glsl"),
  vbo(0),
  vao(0),
  projection_link(s.getLink("projection", glm::mat4())),
  color_link(s.getLink("textColor", glm::vec3()))
{
  FT_Library ft;
  if (FT_Init_FreeType(&ft)) {
    std::cerr << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
    return;
  }

  FT_Face face;
  if (FT_New_Face(ft, font, 0, &face)) {
    std::cerr << "ERROR::FREETYPE: Failed to load font " << font << std::endl;
    return;
  }
  FT_Set_Pixel_Sizes(face, 0, 48);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  // map with characters
  for (unsigned char c = 32; c < 128; c++) {

    // load character glyph
    if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
      std::cerr << "ERROR::FREETYTPE: Failed to load Glyph" << c << std::endl;
      continue;
    }

    // generate texture
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width,
                 face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE,
                 face->glyph->bitmap.buffer);

    // set texture options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // now store character for later use
    Character character = {
      texture, glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
      glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
      unsigned(face->glyph->advance.x)
    };
    characters.insert(std::pair<char, Character>(c, character));
  }
  glBindTexture(GL_TEXTURE_2D, 0);

  // return the ft resources
  FT_Done_Face(face);
  FT_Done_FreeType(ft);

  // vertex arrays for characters
  // unsigned int VAO, VBO;

  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

TextRenderer::~TextRenderer()
{
  if (vao) {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
  }
}

void TextRenderer::renderText(const std::string &text, float x, float y,
                              float scale, glm::vec3 color)
{
  s.use();
  color_link.update(color);

  glActiveTexture(GL_TEXTURE0);
  glBindVertexArray(vao);

  // iterate through all characters
  std::string::const_iterator c;
  for (c = text.begin(); c != text.end(); c++) {
    Character ch = characters[*c];

    float xpos = x + ch.Bearing.x * scale;
    float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

    float w = ch.Size.x * scale;
    float h = ch.Size.y * scale;
    // update VBO for each character
    float vertices[6][4] = { { xpos, ypos + h, 0.0f, 0.0f },
                             { xpos, ypos, 0.0f, 1.0f },
                             { xpos + w, ypos, 1.0f, 1.0f },

                             { xpos, ypos + h, 0.0f, 0.0f },
                             { xpos + w, ypos, 1.0f, 1.0f },
                             { xpos + w, ypos + h, 1.0f, 0.0f } };

    // render glyph texture over quad
    glBindTexture(GL_TEXTURE_2D, ch.TextureID);

    // update content of VBO memory
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // render quad
    glDrawArrays(GL_TRIANGLES, 0, 6);
    // now advance cursors for next glyph (note that advance is number of 1/64
    // pixels)
    x += (ch.Advance >> 6) *
         scale; // bitshift by 6 to get value in pixels (2^6 = 64)
  }

  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);
}