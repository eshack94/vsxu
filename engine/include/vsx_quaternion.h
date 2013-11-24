/**
* Project: VSXu Engine: Realtime modular visual programming engine.
*
* This file is part of Vovoid VSXu Engine.
*
* @author Jonatan Wallmander, Robert Wenzel, Vovoid Media Technologies AB Copyright (C) 2003-2013
* @see The GNU Lesser General Public License (LGPL)
*
* VSXu Engine is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
* for more details.
*
* You should have received a copy of the GNU Lesser General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/


#ifndef VSX_QUATERNION_H
#define VSX_QUATERNION_H

#include <vsx_vector.h>
#include <vsx_matrix.h>

inline float vsx_math_3d_max(const float& x, const float& y)
{
  return ( x > y ) ? x : y;
}

class vsx_quaternion
{
public:
  float x;
  float y;
  float z;
  float w;
  
  vsx_quaternion()
  {
    x = 0.0f;
    y = 0.0f;
    z = 0.0f;
    w = 1.0f;
  }

  vsx_quaternion(float xx, float yy, float zz)
  {
    x = xx;
    y = yy;
    z = zz;
  }

  vsx_quaternion(float xx, float yy, float zz, float ww)
  {
    x = xx;
    y = yy;
    z = zz;
    w = ww;
  }

  inline void cos_slerp(vsx_quaternion& from, vsx_quaternion& to, float t)
  {
    slerp(from,to,(float)sin(t*HALF_PI));
  }

  inline void slerp(vsx_quaternion& from, vsx_quaternion& to, float t)
  {
    float         to1[4];
    double        omega, cosom, sinom, scale0, scale1;
    // calc cosine
    cosom = from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w;
    // adjust signs (if necessary)
    if (cosom < 0.0)
    { 
      cosom = -cosom; to1[0] = - to.x;
  		to1[1] = - to.y;
  		to1[2] = - to.z;
  		to1[3] = - to.w;
    } else {
  		to1[0] = to.x;
  		to1[1] = to.y;
  		to1[2] = to.z;
  		to1[3] = to.w;
    }
    // calculate coefficients
    if ( (1.0 - cosom) > 0.00001f )
    {
      // standard case (slerp)
      omega = acos(cosom);
      sinom = sin(omega);
      scale0 = sin((1.0 - t) * omega) / sinom;
      scale1 = sin(t * omega) / sinom;
    }
    else
    {
      // "from" and "to" quaternions are very close 
      //  ... so we can do a linear interpolation
      scale0 = 1.0 - t;
      scale1 = t;
    }
  	// calculate final values
  	x = (float)(scale0 * from.x + scale1 * to1[0]);
  	y = (float)(scale0 * from.y + scale1 * to1[1]);
  	z = (float)(scale0 * from.z + scale1 * to1[2]);
  	w = (float)(scale0 * from.w + scale1 * to1[3]);
  }
  
  vsx_quaternion operator +=(const vsx_quaternion &t)
  {
    x+=t.x;
    y+=t.y;
    z+=t.z;
    w+=t.w;
    return *this;
  }

  vsx_quaternion operator -(const vsx_quaternion &t)
  {
    vsx_quaternion temp;
    temp.x = x-t.x;
    temp.y = y-t.y;
    temp.z = z-t.z;
    return temp;
  }

  vsx_quaternion operator -(const vsx_vector &t)
  {
    vsx_quaternion temp;
    temp.x = x-t.x;
    temp.y = y-t.y;
    temp.z = z-t.z;
    return temp;
  }


  vsx_quaternion operator *(const vsx_vector &t) const
  {
    vsx_quaternion temp;
    temp.x = x*t.x;
    temp.y = y*t.y;
    temp.z = z*t.z;
    return temp;
  }


  inline float dot_product(vsx_vector* ov) const
  {
    return x * ov->x   +   y * ov->y   +   z * ov->z;
  }


  inline void from_axis_angle( vsx_vector &source_axis, float &source_angle)
  {
    w = sin( source_angle * 0.5f );
    x = source_axis.x * w;
    y = source_axis.y * w;
    z = source_axis.z * w;
    w = cos( source_angle * 0.5f );
  }

  inline void to_axis_angle( vsx_vector &result_axis, float &result_angle)
  {
    result_angle = 2 * acos(w);
    float sq = 1.0f / sqrt(1-w*w);
    result_axis.x = x * sq;
    result_axis.y = y * sq;
    result_axis.z = z * sq;
  }

  inline void mul(const vsx_quaternion &q1, const vsx_quaternion &q2)
  {
    x =  q1.x * q2.w + q1.y * q2.z - q1.z * q2.y + q1.w * q2.x;
    y = -q1.x * q2.z + q1.y * q2.w + q1.z * q2.x + q1.w * q2.y;
    z =  q1.x * q2.y - q1.y * q2.x + q1.z * q2.w + q1.w * q2.z;
    w = -q1.x * q2.x - q1.y * q2.y - q1.z * q2.z + q1.w * q2.w;
  }
  
  inline vsx_matrix matrix()
  {
    vsx_matrix mat;
    GLfloat n, s;
    GLfloat xs, ys, zs;
    GLfloat wx, wy, wz;
    GLfloat xx, xy, xz;
    GLfloat yy, yz, zz;

    n = (x * x) + (y * y) + (z * z) + (w * w);
    s = (n > 0.0f) ? (2.0f / n) : 0.0f;

    xs = x * s;  ys = y * s;  zs = z * s;
    wx = w * xs; wy = w * ys; wz = w * zs;
    xx = x * xs; xy = x * ys; xz = x * zs;
    yy = y * ys; yz = y * zs; zz = z * zs;

    mat.m[0]     = 1.0f - (yy + zz); mat.m[1]     =         xy - wz;  mat.m[2]     =         xz + wy;
    mat.m[4]     =         xy + wz;  mat.m[5]     = 1.0f - (xx + zz); mat.m[6]     =         yz - wx;
    mat.m[8]     =         xz - wy;  mat.m[9]     =         yz + wx;  mat.m[10]    = 1.0f - (xx + yy);
    return mat;
  }

  inline void from_matrix(vsx_matrix* mm)
  {
    #define m00 mm->m[0]
    #define m01 mm->m[4]
    #define m02 mm->m[8]
    #define m03 mm->m[12]
    #define m10 mm->m[1]
    #define m11 mm->m[5]
    #define m12 mm->m[9]
    #define m13 mm->m[13]
    #define m20 mm->m[2]
    #define m21 mm->m[6]
    #define m22 mm->m[10]
    #define m23 mm->m[14]
    #define m30 mm->m[3]
    #define m31 mm->m[7]
    #define m32 mm->m[11]
    #define m33 mm->m[15]

    /*#define m00 mm->m[0]
    #define m01 mm->m[1]
    #define m02 mm->m[2]
    #define m03 mm->m[3]
    #define m10 mm->m[4]
    #define m11 mm->m[5]
    #define m12 mm->m[6]
    #define m13 mm->m[7]
    #define m20 mm->m[8]
    #define m21 mm->m[9]
    #define m22 mm->m[10]
    #define m23 mm->m[11]
    #define m30 mm->m[12]
    #define m31 mm->m[13]
    #define m32 mm->m[14]
    #define m33 mm->m[15]
*/
    w = sqrt( vsx_math_3d_max( 0, 1 + m00 + m11 + m22 ) ) * 0.5;
    x = sqrt( vsx_math_3d_max( 0, 1 + m00 - m11 - m22 ) ) * 0.5;
    y = sqrt( vsx_math_3d_max( 0, 1 - m00 + m11 - m22 ) ) * 0.5;
    z = sqrt( vsx_math_3d_max( 0, 1 - m00 - m11 + m22 ) ) * 0.5;

    x = copysign( x, m21 - m12 );
    y = copysign( y, m02 - m20 );
    z = copysign( z, m10 - m01 );
    
  }

  
  // normalizes the quaternion ensuring it stays a unit-quaternion
  inline void normalize()
  {
    float len = (float)(1.0 / sqrt(
      x*x +
      y*y +
      z*z +
      w*w
    ));
    x *= len;
    y *= len;
    z *= len;
    w *= len;
  }

  vsx_vector transform(const vsx_vector &p1)
  {
    vsx_vector p2;
    p2.x = w*w*p1.x + 2*y*w*p1.z - 2*z*w*p1.y + x*x*p1.x + 2*y*x*p1.y + 2*z*x*p1.z - z*z*p1.x - y*y*p1.x;
    p2.y = 2*x*y*p1.x + y*y*p1.y + 2*z*y*p1.z + 2*w*z*p1.x - z*z*p1.y + w*w*p1.y - 2*x*w*p1.z - x*x*p1.y;
    p2.z = 2*x*z*p1.x + 2*y*z*p1.y + z*z*p1.z - 2*w*y*p1.x - y*y*p1.z + 2*w*x*p1.y - x*x*p1.z + w*w*p1.z;
    return p2;
  }


#ifdef VSXFST_H
 /*****************************************************************************/
/** Initializes the quaternion from a vsx_string
  *
  * 
  *****************************************************************************/
  void from_string(vsx_string& str) {
    vsx_avector<vsx_string> parts;
    vsx_string deli = ",";
    explode(str, deli, parts);
    if (parts.size() == 4) {
      x = s2f(parts[0]);
      y = s2f(parts[1]);
      z = s2f(parts[2]);
      w = s2f(parts[3]);
    }
  }
#endif

  
};



#endif
