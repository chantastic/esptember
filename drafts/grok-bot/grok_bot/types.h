#pragma once
struct RGB { float r, g, b; };
struct Rect { int x0, y0, x1, y1; };
// One eye in its own frame, in pixels.
struct EyeShape { float len, rad, arc, arcR, arcW, sinA, cosA; };
