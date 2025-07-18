
/*
https://stackoverflow.com/questions/30337845/gldrawarrays-not-working-using-gtkglarea-in-gtk3

https://www.bassi.io/articles/2015/02/17/using-opengl-with-gtk/
*/

#include "gio/gio.h"
#include <gtk/gtk.h>
#include <epoxy/gl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>


const GLchar *vert_src = R"glsl( 
#version 330 core                              
                                              
layout(location = 0) in vec2 in_position;     
                                              
void main()                                   
{
  gl_Position = vec4(in_position, 0.0, 1.0);
}
)glsl";

const GLchar *frag_src = R"glsl(
#version 330 core
void main (void)
{
  gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);
}
)glsl";


GLuint gl_vao = 0, gl_buffer = 0, gl_program = 0;

static void realise(GtkGLArea *area, gpointer)
{

  gtk_gl_area_make_current(area);
  if (gtk_gl_area_get_error (GTK_GL_AREA(area)) != NULL)
  {
    printf("Failed to initialiize buffers\n");
    return;
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

  // reset
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  //glDeleteBuffers(1, &gl_buffer);

  return ;
}

static gboolean render(GtkGLArea *area, GdkGLContext *context, gpointer)
{
  std::cerr << "render called" << std::endl;

  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glUseProgram(gl_program);
  glBindVertexArray(gl_vao);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glBindVertexArray (0);

  return TRUE;
}

void app_activate(GApplication *app, gpointer user_data)
{
  GtkWidget *window  = gtk_application_window_new(GTK_APPLICATION(app));
  gtk_window_set_title(GTK_WINDOW(window), "Test GL (GTK4)");
  gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);

  GtkWidget *gl_area = gtk_gl_area_new();
  gtk_gl_area_set_has_depth_buffer(GTK_GL_AREA(gl_area), FALSE);
  gtk_gl_area_set_required_version(GTK_GL_AREA(gl_area), 3, 3);
  gtk_widget_set_hexpand(gl_area, TRUE);
  gtk_widget_set_vexpand(gl_area, TRUE);

  g_signal_connect(gl_area, "realize",      G_CALLBACK(realise),       NULL);
  g_signal_connect(gl_area, "render",       G_CALLBACK(render),        NULL);

  gtk_window_set_child(GTK_WINDOW(window), gl_area);
  gtk_window_present(GTK_WINDOW(window));
}


static const char *APPLICATION_ID = "nl.tudelft.dueca.gtk4gl";
static GtkApplication *app = NULL;
int main(int argc, char** argv)
{

  app = gtk_application_new(APPLICATION_ID, G_APPLICATION_NON_UNIQUE);
  g_signal_connect(G_APPLICATION(app), "activate", G_CALLBACK(app_activate),
                   NULL);
  int res = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);
  return res;
}


