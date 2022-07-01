/* ------------------------------------------------------------------   */
/*      item            : gtk3gl.cxx
        made by         : Rene' van Paassen
        date            : 191018
        category        : body file
        description     :
        changes         : 191018 first version
        language        : C++
        copyright       : (c) 19 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#include <gtk/gtk.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <iostream>
using namespace std;

double pitch = 0.0;
double roll = 0.0;
GtkWidget *area = NULL;
GtkWindow *window = NULL;

GLuint theCorner = 0u;
GLuint theWorld = 0u;
GLuint theRest = 0u;

double pixels_per_mm = 10;

int icount = 1000;

static gint call_environment_loop(gpointer arg)
{
  cout << "call_environment_loop " << icount;
  icount--;
  if (icount > 500) {
    roll += 0.1;
    usleep(10000);
    gtk_gl_area_queue_render(GTK_GL_AREA(area));
  }
  else if (icount > 1) {
    pitch += 0.1;
    usleep(10000);
    gtk_gl_area_queue_render(GTK_GL_AREA(area));
  }
  else {
    gtk_main_quit();
  }
  return TRUE;
}

static gboolean on_realize(GtkGLArea *area, gpointer self)
{
  cout << "on_realize" << endl;
  gtk_gl_area_make_current(GTK_GL_AREA(area));

  glViewport (0, 0, 640, 640);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(-640.0, 640.0, -512.0, 512.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  pixels_per_mm = 4.3*640/1400.0;

  glClearColor (0.0, 0.0, 0.0, 1.0);
  glShadeModel(GL_SMOOTH);

  theCorner = glGenLists(1);
  if(theCorner != 0) {
    glNewList(theCorner, GL_COMPILE);

    // draw one corner (upper right)
    glColor3f(0.0, 0.0, 0.0);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(31.0, 35.0);

    glVertex2d( 0.0, 34.0);
    glVertex2d( 0.1736481776669*34.0, 0.9848077530122*34.0);
    glVertex2d( 0.3420201433257*34.0, 0.9396926207859*34.0);
    glVertex2d( 0.5f           *34.0, 0.8660254037844*34.0);
    glVertex2d( 0.6427876096865*34.0, 0.766044443119 *34.0);

    glVertex2f(27.0, 21.0);
    glVertex2f(29.4, 17.0);
    glEnd();

    // end display list
    glEndList();
  }

  // create moving earth/sky display list
  theWorld = glGenLists(1);

  // new display list
  if(theWorld != 0) {
    glNewList(theWorld, GL_COMPILE);

    // earth
    glBegin(GL_POLYGON);
    //glColor3f(0.62f, 0.44f, 0.17f);
    glColor3f(0.855f, 0.484f, 0.332f);
    glVertex2f(-3000.0, 0.0);
    glVertex2f(3000.0, 0.0 );
    glColor3f(0.855f, 0.570f, 0.355f);
    glVertex2f(3000.0, -150.0);
    glVertex2f(-3000.0, -150.0);
    glEnd();

    // sky
    glBegin(GL_POLYGON);
    //glColor3f(0.15f, 0.15f, 0.85f);
    glColor3f(0.0f, 0.777f, 0.941f);
    glVertex2f(-3000.0, 0.0);
    glVertex2f(3000.0, 0.0 );
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex2f(3000.0, 150.0);
    glVertex2f(-3000.0, 150.0);
    glEnd();

    // horizon
    glBegin(GL_LINES);
    glColor3f(1.0, 1.0, 1.0);
    glVertex2f(-3000.0, 0.0);
    glVertex2f(3000.0, 0.0);
    glEnd();

    // pitch ladder
    glBegin(GL_LINES);
    glColor3f(1.0, 1.0, 1.0);
    for (int i=1;i<10;i++) {
      // major tick positive
      glVertex2f(-6.0, i*13.0);
      glVertex2f(6.0, i*13.0);

      // minor tick positive
      glVertex2f(-3.0, i*13.0-6.5);
      glVertex2f(3.0, i*13.0-6.5);

      // major tick negative
      glVertex2f(-6.0, -i*13.0);
      glVertex2f(6.0, -i*13.0);

      // minor tick negative
      glVertex2f(-3.0, -i*13.0+6.5);
      glVertex2f(3.0, -i*13.0+6.5);
    }
    glEnd();

    // numbers besides pitch ladder (DO NOT INCLUDE IN LOOP ABOVE (GLUT-BUG)!!!)
    for (int i=1; i<10; i++) {
      char text[12];
      snprintf(text, 12, "%i",i*10);
      //strokeString(7, i*13-1, text, GLUT_STROKE_ROMAN, 0.03);
      //strokeString(-12, i*13-1, text, GLUT_STROKE_ROMAN, 0.03);

      //strokeString(7, -i*13-1, text, GLUT_STROKE_ROMAN, 0.03);
      //strokeString(-12, -i*13-1, text, GLUT_STROKE_ROMAN, 0.03);
    }

    // end display list
    glEndList();
  }

  // create display list for the rest
  theRest = glGenLists(1);

  // new display list
  if(theRest != 0) {

    glNewList(theRest, GL_COMPILE);

    // flight path marker
    // original
    /*
    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_LINES);
    glVertex2f(-5.0, 0.0);
    glVertex2f(-2.0, 0.0);
    glVertex2f(2.0, 0.0);
    glVertex2f(5.0, 0.0);
    glVertex2f(0.0, 2.0);
    glVertex2f(0.0, 5.0);
    glEnd();
    glBegin(GL_LINE_LOOP);
    glVertex2f(-2.0, 0.0);
    glVertex2f(0.0, 2.0);
    glVertex2f(2.0, 0.0);
    glVertex2f(0.0, -2.0);
    glEnd();
    */
    // new fpm
    for(int i=0;i<2;i++) {
      if(i)
        glColor3f(1.0, 1.0, 0.0); // yellow outline
      else
        glColor3f(0.0, 0.0, 0.0); // black background

      glBegin(i?GL_LINE_STRIP:GL_POLYGON); // left top
      glVertex2f(-7.2, -0.6);
      glVertex2f(-17.4, -0.6);
      glVertex2f(-17.4, 0.6);
      glVertex2f(-6.0, 0.6);
      glEnd();
      glBegin(i?GL_LINE_STRIP:GL_POLYGON); // left bottom
      glVertex2f(-7.2, -0.6);
      glVertex2f(-7.2, -2.0);
      glVertex2f(-6.0, -2.0);
      glVertex2f(-6.0, 0.6);
      glEnd();

      glBegin(i?GL_LINE_STRIP:GL_POLYGON); // right top
      glVertex2f(7.2, -0.6);
      glVertex2f(17.4, -0.6);
      glVertex2f(17.4, 0.6);
      glVertex2f(6.0, 0.6);
      glEnd();
      glBegin(i?GL_LINE_STRIP:GL_POLYGON); // right bottom
      glVertex2f(7.2, -0.6);
      glVertex2f(7.2, -2.0);
      glVertex2f(6.0, -2.0);
      glVertex2f(6.0, 0.6);
      glEnd();

      glBegin(i?GL_LINE_LOOP:GL_POLYGON); // center
      glVertex2f(-0.6, 0.6);
      glVertex2f(-0.6, -0.6);
      glVertex2f(0.6, -0.6);
      glVertex2f(0.6, 0.6);
      glEnd();
    }

    // aircraft reference boxes on the side
    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_LINE_LOOP); // left
    glVertex2f(-29.4, -0.5);
    glVertex2f(-29.4, 0.5);
    glVertex2f(-26.4, 0.5);
    glVertex2f(-26.4, -0.5);
    glEnd();

    glBegin(GL_LINE_LOOP); // right
    glVertex2f(29.4, -0.5);
    glVertex2f(29.4, 0.5);
    glVertex2f(26.4, 0.5);
    glVertex2f(26.4, -0.5);
    glEnd();

    // bank indicator tickmarks
    for (int i=-60;i<61;i+=10) {
      glPushMatrix();
      glRotatef(i, 0.0, 0.0, 1.0);
      glTranslatef(0.0, 34.0, 0.0);

      if (i ==0) {
        glBegin(GL_LINE_LOOP);
        glVertex2f(0.0, 0.0);
        glVertex2f(2.0, 3.0);
        glVertex2f(-2.0, 3.0);
        glEnd();
      }
      else if(i==-50 || i==-40 || i==40 || i==50) {} // skip them
      else {
        glBegin(GL_LINES);
        glColor3f(1.0, 1.0, 1.0);
        glVertex2f(0.0, 0.0);
        glVertex2f(0.0, 2.0);
        glEnd();
      }

      glPopMatrix();
    }

    for(int i=-45;i<91;i+=90) {
      glPushMatrix();
      glRotatef(i, 0.0, 0.0, 1.0);
      glTranslatef(0.0, 34.0, 0.0);

      glBegin(GL_LINE_LOOP);
      glVertex2f(0.0, 0.0);
      glVertex2f(1.0, 1.5);
      glVertex2f(-1.0, 1.5);
      glEnd();

      glPopMatrix();
    }

    // speed indicator outline
    glBegin(GL_LINE_STRIP);
    glVertex2f(-41.4, -3.0);
    glVertex2f(-33.4, -3.0);
    glVertex2f(-33.4, -6.0);
    glVertex2f(-29.4, -6.0);
    glVertex2f(-29.4, 6.0);
    glVertex2f(-33.4, 6.0);
    glVertex2f(-33.4, 3.0);
    glVertex2f(-41.4, 3.0);
    glEnd();



    // sideforce indicator (ball)
    // TODO

    // end display list
    glEndList();
  }
  return TRUE;
}

static gboolean on_render(GtkGLArea *area,
                          GdkGLContext* context, gpointer self)
{
  cout << "on_render" << endl;

  gtk_gl_area_make_current(area);
  static double clip_top[4] = {0.0, -1.0, 0.0, 34.0};
  static double clip_left[4] = {1.0, 0.0, 0.0, 29.4};
  static double clip_right[4] = {-1.0, 0.0, 0.0, 29.4};
  static double clip_bottom[4] = {0.0, 1.0, 0.0, 34.0};

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  //glClearColor(0.0, 0.0, 0.0, 1.0);

  if (gtk_gl_area_get_error(area) != NULL)
    return FALSE;

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();

  glViewport( int(130*pixels_per_mm),
              int(112*pixels_per_mm),
              int(96*pixels_per_mm),
              int(105*pixels_per_mm));

  // scale to fill entire viewport
  glScalef(13.333f, 9.7, 1.0);
  glPushMatrix();
  glClipPlane(GL_CLIP_PLANE0, clip_top);
  glEnable(GL_CLIP_PLANE0);
  glClipPlane(GL_CLIP_PLANE1, clip_left);
  glEnable(GL_CLIP_PLANE1);
  glClipPlane(GL_CLIP_PLANE2, clip_right);
  glEnable(GL_CLIP_PLANE2);
  glClipPlane(GL_CLIP_PLANE3, clip_bottom);
  glEnable(GL_CLIP_PLANE3);

  glRotatef(roll, 0.0, 0.0, 1.0); // to rotate roll
  glTranslatef(0.0, -1.3*pitch, 0.0); // to translate pitch (13 mm /10 degrees)

  glCallList(theWorld);

  glTranslatef(0.0, 1.3*pitch + 34.0, 0.0);

  glColor4d(1.0, 1.0, 1.0, 0.7);
  glBegin(GL_POLYGON);
  glVertex2f(0.0, 0.0);
  glVertex2f(-2.0, -3.0);
  glVertex2f(2.0, -3.0);
  glEnd();

  // disable clipping planes for earth, sky etc.
  glDisable(GL_CLIP_PLANE0);
  glDisable(GL_CLIP_PLANE1);
  glDisable(GL_CLIP_PLANE2);
  glDisable(GL_CLIP_PLANE3);

  glPopMatrix(); // to get away from world pitch/roll

  // corners
  glPushMatrix(); // to save from mirroring

  // upper right corner
  glCallList(theCorner);
  // lower left corner
  glRotatef(180, 1.0, 0.0, 0.0);
  glCallList(theCorner);

  // lower right corner
  glRotatef(180, 0.0, 1.0, 0.0);
  glCallList(theCorner);

  glPopMatrix(); // to get away from mirroring the corners

  // rest of constant stuff
  glCallList(theRest);
  glFlush();

  return TRUE;
}

void setup_window()
{
  cout << "setup_window" << endl;
  window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
  gtk_window_set_title(window, "gtk3 gl test");

  area = gtk_gl_area_new();
  gtk_gl_area_set_required_version(GTK_GL_AREA(area), 4, 0);
  gtk_gl_area_set_has_depth_buffer(GTK_GL_AREA(area), TRUE);
  gtk_gl_area_set_has_stencil_buffer(GTK_GL_AREA(area), TRUE);
  gtk_gl_area_set_has_alpha(GTK_GL_AREA(area), TRUE);
  gtk_widget_set_size_request(GTK_WIDGET(area), 640, 640);
  g_signal_connect(window,  "delete-event", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(area, "render", G_CALLBACK(on_render), 0);
  g_signal_connect(area, "realize", G_CALLBACK(on_realize), 0);
  gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(area));
  gtk_widget_show_all(GTK_WIDGET(window));
}

int main(int argc, char* argv[])
{
  gtk_init(&argc, &argv);
  setup_window();

  g_idle_add(call_environment_loop, NULL);
  gtk_main();
  return 0;
}
