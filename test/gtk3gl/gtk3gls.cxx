
/*
https://stackoverflow.com/questions/30337845/gldrawarrays-not-working-using-gtkglarea-in-gtk3

https://www.bassi.io/articles/2015/02/17/using-opengl-with-gtk/
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>

#include <gdk/gdkx.h>
#include <epoxy/glx.h>
#include <epoxy/gl.h>
#include <gtk/gtk.h>

#define IGNORE_VAR(type, identifier) \
{ \
  type IGNORED_VARIABLE_abcd = identifier; \
  identifier = IGNORED_VARIABLE_abcd; \
}

const GLchar *vert_src ="\n" \
"#version 330                                  \n" \
"#extension GL_ARB_explicit_attrib_location: enable  \n" \
"                                              \n" \
"layout(location = 0) in vec2 in_position;     \n" \
"                                              \n" \
"void main()                                   \n" \
"{                                             \n" \
"  gl_Position = vec4(in_position, 0.0, 1.0);  \n" \
"}                                             \n";

const GLchar *frag_src ="\n" \
"void main (void)                              \n" \
"{                                             \n" \
"  gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);    \n" \
"}                                             \n";


GLuint gl_vao, gl_buffer, gl_program;

static gboolean realise(GtkGLArea *area, GdkGLContext *context)
{
  IGNORE_VAR(GdkGLContext*, context);

  gtk_gl_area_make_current(GTK_GL_AREA(area));
  if (gtk_gl_area_get_error (GTK_GL_AREA(area)) != NULL)
  {
    printf("Failed to initialiize buffers\n");
    return FALSE;
  }

  GLfloat verts[] =
  {
    +0.0f, +1.0f,
    -1.0f, -1.0f,
    +1.0f, -1.0f,
  };

  GLuint frag_shader, vert_shader;
  frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
  vert_shader = glCreateShader(GL_VERTEX_SHADER);

  glShaderSource(frag_shader, 1, &frag_src, NULL);
  glShaderSource(vert_shader, 1, &vert_src, NULL);

  glCompileShader(frag_shader);
  glCompileShader(vert_shader);

  gl_program = glCreateProgram();
  glAttachShader(gl_program, frag_shader);
  glAttachShader(gl_program, vert_shader);
  glLinkProgram(gl_program);

  glGenVertexArrays(1, &gl_vao);
  glBindVertexArray(gl_vao);

  glGenBuffers(1, &gl_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, gl_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
  glBindVertexArray(0);

  glDeleteBuffers(1, &gl_buffer);

  return TRUE;
}

static gboolean render(GtkGLArea *area, GdkGLContext *context)
{
  IGNORE_VAR(GdkGLContext*, context);
  IGNORE_VAR(GtkGLArea*, area);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearColor(0.0, 0.0, 0.0, 1.0);

  glUseProgram(gl_program);
  glBindVertexArray(gl_vao);
  glDrawArrays(GL_TRIANGLES, 0, 3);

  glBindVertexArray (0);
  glUseProgram (0);

  glFlush();

  return TRUE;
}

int main(int argc, char** argv)
{
  gtk_init(&argc, &argv);

  GtkWidget *window  = gtk_window_new(GTK_WINDOW_TOPLEVEL),
            *gl_area = gtk_gl_area_new();

  g_signal_connect(window,  "delete-event", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(gl_area, "realize",      G_CALLBACK(realise),       NULL);
  g_signal_connect(gl_area, "render",       G_CALLBACK(render),        NULL);

  gtk_container_add(GTK_CONTAINER(window), gl_area);

  gtk_widget_show_all(window);

  gtk_main();
  return 0;
}


