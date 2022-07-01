#include <GL/gl.h>
#include <GL/glx.h>
#include <X11/Xlib.h>

#include <X11/Xlib.h>
#include <GL/glx.h>
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

using namespace std;

static int attributeList[] = { GLX_RGBA, GLX_DOUBLEBUFFER, None };

static Bool WaitForNotify(Display *d, XEvent *e, char *arg)
{ return(e->type == MapNotify) && (e->xmap.window == (Window)arg); }

int main(int argc, char**argv)
{
  Display *dpy;
  XVisualInfo *vi;
  Colormap cmap;
  XSetWindowAttributes swa;
  Window win;
  GLXContext cx;
  XEvent event;

  /* get a connection Makes a connection, looks like we need to
     maintain this connection for later use. */
  dpy   = XOpenDisplay(0);
  if (!dpy) {
    fprintf(stderr, "Cannot open display.\n");
    exit(-1);
  }

  /* get an appropriate visual */
  /* There may be multiple screens specified in the dpy structure. */
  vi = glXChooseVisual(dpy, DefaultScreen(dpy),
                       attributeList);
  if (!vi) {
    fprintf(stderr, "Cannot find visual with desired attributes.\n");
    exit(-1);
  }

  /* create a GLX context */
  /* third parameter can be a context to share display lists with. */
  /* GL_TRUE requests direct connection to graphics system */
  cx = glXCreateContext(dpy, vi, 0, GL_TRUE);
  if (!cx) {
    fprintf(stderr, "Cannot create context.\n");
    exit(-1);
  }

/* create a colormap -- AllocAll for color index mode */
#ifdef COLORMAP
  cmap = XCreateColormap(dpy, RootWindow(dpy, vi->screen),
                         vi->visual, AllocNone);
  if (!cmap) {
    fprintf(stderr, "Cannot allocate colormap.\n");
    exit(-1);
  }
#endif

  /* create a   window
     XCreateWindow(display, parent,
     x, y, width, height, border_width, depth,
     class, visual, valuemask, attributes)*/
  XSetWindowAttributes att;
  att.event_mask = KeyPress | KeyRelease | ButtonPress | ButtonRelease;
  //    PointerMotion | ButtonMotion;
  win = XCreateWindow(dpy, RootWindow(dpy, vi->screen), 0, 0, 400, 300, 4,
                      CopyFromParent, InputOutput, CopyFromParent,
                      CWEventMask, &att);

  /* select mapnotify events */
  XSelectInput(dpy, win, StructureNotifyMask);

  /* map the window */
  XMapWindow(dpy, win);

  sleep(1);

  swa.colormap = cmap;
  swa.border_pixel = 0;

  /* wait for the notify event */
  for (;;) {
    XEvent e;
    XNextEvent(dpy, &e);
    if (e.type == MapNotify) break;
  }

  /* connect the context to the window */
  glXMakeCurrent(dpy, win, cx);

  sleep(2);
  /* clear the buffer */
  glClearColor(0,0,0,1);
  glClear(GL_COLOR_BUFFER_BIT);
  glFlush();

  /* wait for a while */
    sleep(10);
  /* exit cleanly */
  XCloseDisplay(dpy);
  exit(0);
}
