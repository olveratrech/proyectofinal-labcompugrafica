#pragma once
#include <array>
#include <cmath>
// Código nuevo: los experimentos comparten un paso pequeño, independiente de los FPS.
namespace lab  {
    using State=std::array<double,4>;
    template<class F> void rk4(State& y,double h,F f) {
        auto a=f(y),t=y;
        for(int i=0;i<4;++i)t[i]=y[i]+h*a[i]/2;
        auto b=f(t);
        for(int i=0;i<4;++i)t[i]=y[i]+h*b[i]/2;
        auto c=f(t);
        for(int i=0;i<4;++i)t[i]=y[i]+h*c[i];
        auto d=f(t);
        for(int i=0;i<4;++i)y[i]+=h*(a[i]+2*b[i]+2*c[i]+d[i])/6;
    }
    struct Physics  {
        State pendulum {
            1.25,.65,0,0
        };
        bool pendulumRunning=true;
        void reset() {
            *this=Physics();
        }
        static State pendulumRate(const State& q) {
            double a=q[0],b=q[1],u=q[2],v=q[3],delta=a-b,l=.65,g=9.81,den=3-std::cos(2*delta);
            return  {
                u,v,(-3*g*std::sin(a)-g*std::sin(a-2*b)-2*std::sin(delta)*(v*v*l+u*u*l*std::cos(delta)))/(l*den),    2*std::sin(delta)*(2*u*u*l+2*g*std::cos(a)+v*v*l*std::cos(delta))/(l*den)
            }
            ;
        }
        void step(double h) {
            if(pendulumRunning)rk4(pendulum,h,pendulumRate);
        }
    }
    ;
}
