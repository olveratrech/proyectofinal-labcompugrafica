#pragma once
#include <array>
#include <cmath>
#include <algorithm>
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
        }
        ,force {
            -.12,1.05,0,0
        }
        ;
        double time=0,charge=0,balance=0,velocity=0;
        bool pendulumRunning=true;
        int preset=0;
        void reset() {
            int p=preset;
            *this=Physics();
            preset=p;
        }
        static State pendulumRate(const State& q) {
            double a=q[0],b=q[1],u=q[2],v=q[3],delta=a-b,l=.65,g=9.81,den=3-std::cos(2*delta);
            return  {
                u,v,(-3*g*std::sin(a)-g*std::sin(a-2*b)-2*std::sin(delta)*(v*v*l+u*u*l*std::cos(delta)))/(l*den),    2*std::sin(delta)*(2*u*u*l+2*g*std::cos(a)+v*v*l*std::cos(delta))/(l*den)
            }
            ;
        }
        State forceRate(const State& q) const  {
            double x=q[0],y=q[1],vx=q[2],vy=q[3],A=1,B=0,D=1,fx=0,fy=-9.81;
            // Las alturas de las pesas laterales dependen de la distancia del nudo a cada polea.
            // Esta relación conserva la longitud de las cuerdas durante todo el movimiento.
            for(int side: {
                -1,1
            }
            ) {
                double dx=x-side*.65,dy=y-1.65,r=std::hypot(dx,dy),gx=dx/r,gy=dy/r;
                double mass=side<0?.7+.1*preset:.7;
                double curve=(vx*vx+vy*vy-std::pow(gx*vx+gy*vy,2))/r;
                A+=mass*gx*gx;
                B+=mass*gx*gy;
                D+=mass*gy*gy;
                fx-=mass*gx*(9.81+curve);
                fy-=mass*gy*(9.81+curve);
            }
            fx-=.35*vx;
            fy-=.35*vy;
            double det=A*D-B*B;
            return  {
                vx,vy,(D*fx-B*fy)/det,(A*fy-B*fx)/det
            }
            ;
        }
        void step(double h,bool running) {
            charge+=((running?1.:0.)-charge)*std::min(1.,h*2);
            if(!running)return;
            time+=h;
            if(pendulumRunning)rk4(pendulum,h,pendulumRate);
            rk4(force,h,[this](const State&q) {
                return forceRate(q);
            }
            );
            double target=.12*(preset-1);
            velocity+=(18*(target-balance)-5*velocity)*h;
            balance+=velocity*h;
        }
    }
    ;
}
