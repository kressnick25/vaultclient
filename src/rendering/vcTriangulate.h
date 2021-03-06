#ifndef vcTriangulate_h__
#define vcTriangulate_h__

/*!
**
** Copyright (c) 2007 by John W. Ratcliff mailto:jratcliff@infiniplex.net
**
** The MIT license:
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is furnished
** to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
** WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
** CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "vcMath.h"
#include <vector>

// triangulate a contour/polygon, places results in STL vector as series of triangles.
bool vcTriangulate_Process(const udDouble2 *pCountours, int counterCount, std::vector<udDouble2> *pResult);

// compute area of a contour/polygon
double vcTriangulate_CalculateArea(const udDouble2 *pCountours, int counterCount);

// decide if point p is inside triangle defined by a,b,c
bool vcTriangulate_InsideTriangle(const udDouble2 &p, const udDouble2 &a, const udDouble2 &b, const udDouble2 &c);


#endif//vcTriangulate_h__
