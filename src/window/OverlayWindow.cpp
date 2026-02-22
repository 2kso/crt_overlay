#include "OverlayWindow.h"
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/shape.h>
#include <stdexcept>
#include <string.h>

OverlayWindow::OverlayWindow() {
  display = XOpenDisplay(NULL);
  if (!display) {
    throw std::runtime_error("Failed to open X display.");
  }

  int screen = DefaultScreen(display);
  Window root = RootWindow(display, screen);

  XVisualInfo *vinfo = findARGBVisual(screen);
  if (!vinfo) {
    XCloseDisplay(display);
    throw std::runtime_error(
        "No 32-bit ARGB visual found. A composite manager is required.");
  }

  Colormap colormap = XCreateColormap(display, root, vinfo->visual, AllocNone);

  // Create the transparent, borderless tracking window attributes
  XSetWindowAttributes attr;
  memset(&attr, 0, sizeof(attr));
  attr.colormap = colormap;
  attr.border_pixel = 0;
  attr.background_pixel =
      0; // Ensures the window background is physically transparent
  attr.override_redirect =
      True; // Forces the window manager to ignore us so we stay on top always

  width = DisplayWidth(display, screen);
  height = DisplayHeight(display, screen);

  window = XCreateWindow(
      display, root, 0, 0, width, height, 0, vinfo->depth, InputOutput,
      vinfo->visual,
      CWColormap | CWBorderPixel | CWBackPixel | CWOverrideRedirect, &attr);

  // XFixes extension makes the window completely transparent to mouse clicks
  XserverRegion region = XFixesCreateRegion(display, NULL, 0);
  XFixesSetWindowShapeRegion(display, window, ShapeInput, 0, 0, region);
  XFixesDestroyRegion(display, region);

  // Initialize GLX OpenGL context with our 32-bit visual
  glc = glXCreateContext(display, vinfo, NULL, GL_TRUE);
  if (!glc) {
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    throw std::runtime_error("Failed to create GLX context.");
  }

  delete vinfo;

  // Attach OpenGL context and make the window visible
  glXMakeCurrent(display, window, glc);
  XMapWindow(display, window);
}

OverlayWindow::~OverlayWindow() {
  if (display) {
    if (glc) {
      glXMakeCurrent(display, None, NULL);
      glXDestroyContext(display, glc);
    }
    if (window) {
      XDestroyWindow(display, window);
    }
    XCloseDisplay(display);
  }
}

void OverlayWindow::swapBuffers() const { glXSwapBuffers(display, window); }

bool OverlayWindow::handleEvents() const {
  XEvent xev;
  while (XPending(display)) {
    XNextEvent(display, &xev);
    // We generally ignore events since we are click-through and
    // override-redirected. If we were a normal window we'd check for Close
    // events here.
  }
  return true; // Keep running
}

XVisualInfo *OverlayWindow::findARGBVisual(int screen) {
  XVisualInfo *vinfo = new XVisualInfo;
  if (XMatchVisualInfo(display, screen, 32, TrueColor, vinfo)) {
    return vinfo;
  }
  delete vinfo;
  return nullptr;
}
