#pragma once
#include <cmath>
#include <array>

namespace aurora::math {
struct Vec3 { float x,y,z; };
struct Mat4 { std::array<float,16> m; };

inline Mat4 identity(){ return Mat4{{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1}}; }
inline Mat4 perspective(float fovyRad, float aspect, float zNear, float zFar){
    float f = 1.0f/std::tan(fovyRad/2.0f); float nf = 1.f/(zNear - zFar);
    return Mat4{{f/aspect,0,0,0, 0,f,0,0, 0,0,(zFar+zNear)*nf,-1, 0,0,(2*zFar*zNear)*nf,0}};
}
inline Mat4 translate(const Mat4& a, Vec3 v){ Mat4 r=a; r.m[12]+=v.x; r.m[13]+=v.y; r.m[14]+=v.z; return r; }
inline Mat4 rotateZ(const Mat4& a, float rad){ float c=std::cos(rad), s=std::sin(rad); Mat4 r=a; r.m[0]=c; r.m[1]=s; r.m[4]=-s; r.m[5]=c; return r; }
inline Mat4 mul(const Mat4& A,const Mat4& B){ Mat4 R{}; for(int c=0;c<4;++c) for(int r=0;r<4;++r){ R.m[c*4+r]=A.m[0*4+r]*B.m[c*4+0]+A.m[1*4+r]*B.m[c*4+1]+A.m[2*4+r]*B.m[c*4+2]+A.m[3*4+r]*B.m[c*4+3]; } return R; }
} // namespace aurora::math
