/* ------------------------------------------------------------------   */
/*      item            : VaoVbo.hxx
        made by         : Rene van Paassen
        date            : 250127
        category        : header file
        description     : In GL, most vao/vbo are paired, shorthand
                          struct for this
        changes         : 250127 first version
        language        : C++
        copyright       : (c) 2025 DUECA Authors
*/

#pragma once

#include <epoxy/egl.h>
#include <gtk/gtk.h>

/** Simple helper struct, to combine a VertexArrayObject with a single
    VertexBufferObject */
struct VaoVbo
{
  /// buffer object
  unsigned vbo;
  /// array object
  unsigned vao;

  /// make a set of these
  VaoVbo() :
    vao(0),
    vbo(0)
  {}

  // initialize, do this from within an active GL context
  inline void init()
  {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
  }

  /** remove again, ensure this is called with the right GC active.
   */
  ~VaoVbo()
  {
    if (vao) {
      glDeleteVertexArrays(1, &vao);
      glDeleteBuffers(1, &vbo);
    }
  }

  // explicitly reset the bindings
  inline void unbind()
  {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  }
};
