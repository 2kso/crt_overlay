#include <GL/gl.h>
#include <GL/glx.h>
#include <X11/Xlib.h>
#include <iostream>

int main() {
    Display* display = XOpenDisplay(NULL);
    if (!display) return 1;
    int screen = DefaultScreen(display);

    int attribs[] = {
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DOUBLEBUFFER, True,
        None
    };

    int fbcount = 0;
    GLXFBConfig* fbc = glXChooseFBConfig(display, screen, attribs, &fbcount);
    if (fbc && fbcount > 0) {
        std::cout << "Found " << fbcount << " FBConfigs with Alpha!" << std::endl;
        XVisualInfo* vinfo = glXGetVisualFromFBConfig(display, fbc[0]);
        if (vinfo) {
            std::cout << "Successfully got VisualInfo! Depth: " << vinfo->depth << std::endl;
        } else {
            std::cout << "Failed to get VisualInfo from FBConfig." << std::endl;
        }
    } else {
        std::cout << "No FBConfigs found." << std::endl;
        // Let's try XMatchVisualInfo
        XVisualInfo vinfo;
        if (XMatchVisualInfo(display, screen, 32, TrueColor, &vinfo)) {
            std::cout << "XMatchVisualInfo found depth 32 visual." << std::endl;
        } else {
            std::cout << "XMatchVisualInfo failed." << std::endl;
        }
    }

    return 0;
}
