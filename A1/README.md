COMPILATION

There isn't any change the standard compilation steps. I created additional
avatar.cpp and avatar.hpp files in this assignment but the compilation steps
didn't change at all.
1. premake4 gmake
2. make
3. ./A1

MANUAL

I finished all the requirements. Couple notes...

1. I assumed even when wall height is 0 avatar still can't move through maze.
2. Scroll action work kind of laggy because VM doesn't take the mouse scroll
input smoothly.
3. I assumed that Left Shift is enough for removing the wall section.
4. I used www.songho.ca/opengl/sl_sphere.html#cubesphere as a resource for my
sphere avatar code.
