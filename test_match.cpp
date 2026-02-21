#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <iostream>

int main() {
    Display* display = XOpenDisplay(NULL);
    if (!display) return 1;
    int screen = DefaultScreen(display);

    XVisualInfo vinfo;
    if (XMatchVisualInfo(display, screen, 32, TrueColor, &vinfo)) {
        std::cout << "Successfully found a 32-bit visual!" << std::endl;
    } else {
        std::cout << "Could NOT find a 32-bit visual." << std::endl;
    }
    return 0;
}
