/* ------------------------------------------------------------------   */
/*      item            : display.hxx
        made by         : Rene van Paassen
        date            : 250712
        category        : header file
        description     : description
        changes         : 250712 first version
        language        : C++
        copyright       : (c)2025 Rene van Paassen
*/

#pragma once

#include <boost/scoped_ptr.hpp>
#include <glm/glm.hpp>
#include <string>
#include "Shader.hxx"
#include "VaoVbo.hxx"
#include "TextRenderer.hxx"

/** Test some simple gl in gtk4 window */
class MyDisplay
{

  /** Rotation angle */
  float heading;

  /// font
  std::string font;

  /// GL links for triangle
  VaoVbo headingtri;

  /// GL links for compass
  VaoVbo compass;

  /// simple line shader that
  boost::scoped_ptr<Shader> lineshader;

  /// text renderer
  boost::scoped_ptr<TextRenderer> texter;

public:
  /** Constructor. */
  MyDisplay();

  /** Draw function */
  void display();

  /** Setup function */
  void initGL();

  /** Reshape response */
  void reshape(int w, int h);

  /** Do an update */
  void update();
};