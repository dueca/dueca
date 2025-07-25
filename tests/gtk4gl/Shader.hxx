/* ------------------------------------------------------------------   */
/*      item            : Shader.cxx
        made by         : repa
        date            : Fri Jan 24 09:09:39 2025
        category        : header file
        description     : from https://learnopengl.com Joey de Vries
                          CC BY-NC 4.0
        changes         : Fri Jan 24 09:09:39 2025 first version
        language        : C++
        copyright       : (c) EUPL-1.2
*/

#pragma once
#include <epoxy/egl.h>

/** Shader class for GL, simplifies the creation of
    vertex and fragment shader, and binding these into a
    program. Create the shader whit a opengl GC context active.
    Delete the shader with the same context active.
 */
class Shader
{
public:
  unsigned int ID;

  /// constructor generates the shader
  Shader(const char *vertexPath, const char *fragmentPath);

  /** destructor, frees remaining resources. Make sure your
      GL context is active when the destructor runs. */
  ~Shader();

  /// activate the shader
  inline void use() { glUseProgram(ID); }

  /** Link objects, create a reference to a "Uniform" location */
  template <class T> struct link
  {
    GLint linkid;

    /// constructor, called by Shader
    link(const char *name, GLuint ID)
    {
      linkid = glGetUniformLocation(ID, name);
    }

    /// Update the data of a Uniform
    void update(const T &data, GLsizei count);

    /// Update the data of a Uniform, only one or no multiple possible
    void update(const T &data);
  };

  /** create a link for Uniform data

      @param name    Name of the uniform (must match shader program)
      @param data    First sample of the data type, used to create the
                     right type of link. */
  template <class T>
  inline const link<T> getLink(const char *name, const T &data) const
  {
    return link<T>(name, ID);
  }

private:
  /// Different error throws
  enum ErrorType { ProgramError, VertexError, FragmentError };

  /// utility function for checking shader compilation/linking errors.
  void checkCompileErrors(unsigned int shader, ErrorType type,
                          const char *fname);
};
