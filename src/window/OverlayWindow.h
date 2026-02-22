#ifndef OVERLAY_WINDOW_H
#define OVERLAY_WINDOW_H

#include <GL/glx.h>
#include <X11/Xlib.h>

/**
 * @class OverlayWindow
 * @brief Abstracts X11 and GLX initialization to create a transparent,
 * click-through overlay.
 *
 * This class handles:
 * - Opening the X display and fetching the root window.
 * - Finding an ARGB (32-bit depth) visual for transparency.
 * - Creating a borderless, override-redirect window so it rests above all other
 * windows.
 * - Applying the XFixes extension to make the window transparent to mouse
 * clicks.
 * - Creating and managing the GLX OpenGL context.
 */
class OverlayWindow {
public:
  Display *display;
  Window window;
  GLXContext glc;
  int width;
  int height;

  /**
   * @brief Constructs the overlay window. Throws an exception if X11 or GLX
   * initialization fails.
   */
  OverlayWindow();

  /**
   * @brief Cleans up the OpenGL context, destroys the window, and closes the X
   * display.
   */
  ~OverlayWindow();

  /**
   * @brief Swaps the OpenGL buffers for this window.
   */
  void swapBuffers() const;

  /**
   * @brief Polls for pending X11 events to keep the connection healthy.
   * @return true if the window should remain open, false if it received a quit
   * signal.
   */
  bool handleEvents() const;

private:
  /**
   * @brief Helper function to locate a 32-bit visual on the X server.
   */
  XVisualInfo *findARGBVisual(int screen);
};

#endif // OVERLAY_WINDOW_H
