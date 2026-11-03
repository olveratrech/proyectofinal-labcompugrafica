#include "Physics.h"
#include <cassert>
#include <iostream>
double energy(const lab::State&q) {
    double l=.65;
    return l*l*q[2]*q[2]+.5*l*l*q[3]*q[3]+l*l*q[2]*q[3]*cos(q[0]-q[1])-9.81*l*(2*cos(q[0])+cos(q[1]));
}
int main() {
    for(int p=0;p<3;++p) {
        lab::Physics f;
        f.preset=p;
        double e=energy(f.pendulum);
        for(int i=0;i<14400;++i) {
            f.step(1./240,true);
            for(double v:f.pendulum)assert(std::isfinite(v));
            for(double v:f.force)assert(std::isfinite(v));
            assert(f.force[1]>.3&&f.force[1]<1.6);
            assert(std::abs(f.force[0])<.6);
        }
        assert(std::abs(energy(f.pendulum)-e)<.001);
        auto before=f.pendulum;
        f.step(.1,false);
        assert(f.pendulum==before);
        f.reset();
        assert(f.time==0);
    }
    std::cout<<"Fisica: 60 segundos por configuracion, energia y pausa correctas.\n";
}
