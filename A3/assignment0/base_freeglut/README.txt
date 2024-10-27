Template code for CS535 Assignment 0


LINUX =========================

1. Make sure you have all dependencies installed.

Debian-based systems (e.g. Ubuntu):
	$ sudo apt install glm freeglut

Arch-based systems (e.g. Manjaro):
	$ sudo pacman -Sy glm freeglut

2. Compile
	$ make

3. Run
	$ ./assignment0




WINDOWS =======================

1. Open Visual Studio
2. Build & run

If you have linker errors with freeglut .lib or .dll files, it is
probably due to a compiler mismatch. You can download the latest
FreeGLUT at http://freeglut.sourceforge.net/.


# Key Controls:

## increasing number of objects
Press n: left key - decrease, right key - increase

## changing m, k , d:
Press m/k/d: UP key - increase, DOWN key - decrease

## enable specular and diffuse light:
Press l to turn it on / off

## Reset:
press 'r' to reset everything to initial state