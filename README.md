# crt_overlay

A lightweight CRT screen overlay for X11 Linux desktop environments.
This C++ and OpenGL application draws a hardware-accelerated, transparent, click-through overlay directly over your desktop.

## Features
- X11 Click-Through (`XFixes`)
- Always On Top (override-redirect)
- RGB Phosphor Mask
- Dynamic glitches (luminance flickering, static snow, VHS tracking bands)
- CRT curvature, scanlines, and vignetting

## Prerequisites
- Linux running X11 with an active composite manager
- C++17 compliant compiler
- `cmake`
- OpenGL / GLX
- X11 Libraries: `libx11-dev`, `libxext-dev`, `libxfixes-dev`, `libxrender-dev` (package names vary by distro)

## Build and Run
```bash
cmake -S . -B build
cmake --build build
./build/crt_overlay
```
Or use the provided script:
```bash
chmod +x run.sh
./run.sh
```

## Stopping the Overlay
The overlay is click-through, so you cannot click a window close button.
To close it:
1. Hit `Ctrl+C` in the terminal where it is running.
2. Or use `killall -9 crt_overlay`.
