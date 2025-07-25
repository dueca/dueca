/* ------------------------------------------------------------------   */
/*      item            : Shader.cxx
        made by         : repa
        date            : Fri Jan 24 09:09:39 2025
        category        : body file
        description     : from https://learnopengl.com Joey de Vries
                          CC BY-NC 4.0
        changes         : Fri Jan 24 09:09:39 2025 first version
        language        : C++
        copyright       : (c) EUPL-1.2
*/

#include "Shader.hxx"

#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>
#include <string>


Shader::Shader(const char *vertexPath, const char *fragmentPath)
{
  // 1. retrieve the vertex/fragment source code from filePath
  std::string vertexCode;
  std::string fragmentCode;
  std::ifstream vShaderFile;
  std::ifstream fShaderFile;

  // ensure ifstream objects can throw exceptions:
  vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

  try {
    // open files
    vShaderFile.open(vertexPath);
    fShaderFile.open(fragmentPath);
    std::stringstream vShaderStream, fShaderStream;
    // read file's buffer contents into streams
    vShaderStream << vShaderFile.rdbuf();
    fShaderStream << fShaderFile.rdbuf();
    // close file handlers
    vShaderFile.close();
    fShaderFile.close();
    // convert stream into string
    vertexCode = vShaderStream.str();
    fragmentCode = fShaderStream.str();
  }
  catch (std::ifstream::failure &e) {
    std::cerr << "Shader File not successfully read: " << e.what() << std::endl;
  }

  const char *vShaderCode = vertexCode.c_str();
  const char *fShaderCode = fragmentCode.c_str();
        // 2. compile shaders
  unsigned int vertex, fragment;
        // vertex shader
  vertex = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex, 1, &vShaderCode, NULL);
  glCompileShader(vertex);
  checkCompileErrors(vertex, VertexError, vertexPath);
        // fragment Shader
  fragment = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment, 1, &fShaderCode, NULL);
  glCompileShader(fragment);
  checkCompileErrors(fragment, FragmentError, fragmentPath);
        // shader Program
  ID = glCreateProgram();
  glAttachShader(ID, vertex);
  glAttachShader(ID, fragment);
  glLinkProgram(ID);
  checkCompileErrors(ID, ProgramError, vertexPath);
  // delete the shaders as they're linked into our program now and no longer
  // necessary
  glDeleteShader(vertex);
  glDeleteShader(fragment);
}

void Shader::checkCompileErrors(unsigned int shader, ErrorType type, const char* fname)
{
  static const char *estrings[] = { "Program", "Vertex", "Fragment" };
  int success;
  char infoLog[1024];
  if (type != ProgramError) {
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(shader, 1024, NULL, infoLog);
      std::cerr << "Shader program log:" << std::endl << infoLog << std::endl;
    }
  }
  else {
    glGetProgramiv(shader, GL_LINK_STATUS, &success);
    if (!success) {
      glGetProgramInfoLog(shader, 1024, NULL, infoLog);
      std::cerr << "Shader link log:" << std::endl << infoLog << std::endl;
    }
  }
}

Shader::~Shader()
{
  if (ID) {
    glDeleteProgram(ID);
  }
}

// each type of link update uses a template instantiation
template <>
void Shader::link<glm::mat4>::update(const glm::mat4 &data, GLsizei count)
{
  glUniformMatrix4fv(linkid, count, GL_FALSE, &data[0][0]);
}

template <>
void Shader::link<glm::vec3>::update(const glm::vec3 &data, GLsizei count)
{
  glUniform3fv(linkid, count, &data[0]);
}

template <>
void Shader::link<glm::mat4>::update(const glm::mat4 &data)
{
  glUniformMatrix4fv(linkid, 1, GL_FALSE, &data[0][0]);
}

template <>
void Shader::link<glm::vec3>::update(const glm::vec3 &data)
{
  glUniform3fv(linkid, 1, &data[0]);
}

template <>
void Shader::link<bool>::update(const bool &data)
{
  glUniform1i(linkid, (int) data);
}

template <>
void Shader::link<int32_t>::update(const int32_t &data)
{
  glUniform1i(linkid, data);
}

template <>
void Shader::link<float>::update(const float &data)
{
  glUniform1f(linkid, data);
}
