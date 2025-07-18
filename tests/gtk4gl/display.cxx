/* ------------------------------------------------------------------   */
/*      item            : display.cxx
        made by         : Rene van Paassen
        date            : 250712
        category        : body file
        description     : Display implementation
        changes         : 250712 first version
        language        : C++
        copyright       : (c)2025 Rene van Paassen
*/
#include "display.hxx"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

MyDisplay::MyDisplay() :
  heading(0.0f),
  font("/usr/share/fonts/open-sans/OpenSans-Regular.ttf")
{}


void MyDisplay::display()
{
 // background color
  glClearColor(0.2f, 0.0f, 0.2f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  // projection matrix, start with identity
  glm::mat4 projection = glm::mat4(1.0f);
  auto line_proj = lineshader->getLink("projection", projection);

  // fixed head-up triangle
  lineshader->use();
  line_proj.update(projection);

  // draws the heading triangle
  glBindVertexArray(headingtri.vao);
  glDrawArrays(GL_LINE_LOOP, 0, 3);

  // rotate the projection matrix for the compass rose
  projection = glm::rotate(projection, heading, glm::vec3(0.0f, 0.0f, 1.0f));
  line_proj.update(projection);

  // draws the compass lines
  glBindVertexArray(compass.vao);
  glDrawArrays(GL_LINES, 0, 72);

  // N(orth)
  texter->setProjection(projection);
  glm::vec3 green = { 0.0f, 1.0f, 0.0f };
  texter->renderText("N", -0.08, 0.6, 0.005, green);

  // shift by 90 degrees
  projection = glm::rotate(projection, M_PI_2f, glm::vec3(0.0f, 0.0f, 1.0f));
  texter->setProjection(projection);
  texter->renderText("W", -0.1, 0.6, 0.005, green);

  // again, for south
  projection = glm::rotate(projection, M_PI_2f, glm::vec3(0.0f, 0.0f, 1.0f));
  texter->setProjection(projection);
  texter->renderText("S", -0.08, 0.6, 0.005, green);

  // last one east
  projection = glm::rotate(projection, M_PI_2f, glm::vec3(0.0f, 0.0f, 1.0f));
  texter->setProjection(projection);

  texter->renderText("E", -0.08, 0.6, 0.005, green);
}

  /** Setup function */
void MyDisplay::initGL()
{
 // once and only once
  if (!lineshader) {

    // default opengl state for this project
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // shader for the lines
    lineshader.reset(new Shader("../lines_vrt.glsl",
                                "../lines_frg.glsl"));

    // ac triangle
    float headsuptri[] = { 0.0, 0.5, 0.2, -0.4, -0.2, -0.4 };

    // initialize the buffer and array objects
    headingtri.init();

    // set the triangle vertices
    glBufferData(GL_ARRAY_BUFFER, sizeof(headsuptri), headsuptri,
                 GL_STATIC_DRAW);

    // explains the set-up, each vertex is two floats, tightly packed
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), NULL);

    // enable this one
    glEnableVertexAttribArray(0);
    headingtri.unbind();

    // data for the compass
    // per 10 deg, with a larger line each 30
    std::vector<float> compassv;
    compassv.reserve(144);
    for (unsigned ii = 360; ii; ii -= 10) {
      compassv.push_back(0.98 * sin(ii / 180.0 * M_PI));
      compassv.push_back(0.98 * cos(ii / 180.0 * M_PI));
      if (ii % 30 == 0) {
        compassv.push_back(0.8 * sin(ii / 180.0 * M_PI));
        compassv.push_back(0.8 * cos(ii / 180.0 * M_PI));
      }
      else {
        compassv.push_back(0.9 * sin(ii / 180.0 * M_PI));
        compassv.push_back(0.9 * cos(ii / 180.0 * M_PI));
      }
    }

    // tie to vao/vbo pair
    compass.init();
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * compassv.size(),
                 compassv.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), NULL);
    glEnableVertexAttribArray(0);
    compass.unbind();

    // create a new text renderer
    texter.reset(new TextRenderer(font.c_str()));
  }
}

void MyDisplay::reshape(int w, int h)
{}

void MyDisplay::update()
{
  heading += 0.5f;
}
