// Base: Practica8/fuentesDeLuz.cpp y clases de src/base.
// La sala, las estaciones, las fichas y su coordinación son código nuevo.
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>
#include <memory>
#include <map>
#include "base/Model.h"
#include "base/Camera.h"
#include "Physics.h"
using glm::vec3;
using glm::mat4;
namespace fs=std::filesystem;
Camera camera(vec3(0,2.4,10),vec3(0,1,0),-90,-8);
lab::Physics simulation;
bool running=true,card=false,showSample=false;
int selected=0;
enum EstadoResorte {RESORTE_REPOSO,RESORTE_ESTIRAR,RESORTE_OSCILAR};
EstadoResorte estadoResorte=RESORTE_REPOSO;
float desplazamientoResorte=0,tiempoResorte=0;
bool agitarVarilla=false;
int cuadroVarilla=0;
float avanceVarilla=0;
vec3 desplazamientoVarilla(0);
bool vanDeGraaffOn=false;
float vanButton=0,vanTravel=0,vanRoll=0,vanCarousel=0;
enum EstadoPrisma {PRISMA_APAGADO,PRISMA_ENTRADA,PRISMA_DISPERSION,PRISMA_COMPLETO};
EstadoPrisma estadoPrisma=PRISMA_APAGADO;
float avanceEntrada=0,avanceEspectro=0,oscuridadLaboratorio=0;
// Cada mecanismo guarda su propio destino para que ninguno bloquee a los demas.
bool microscopeLightOn=false,microscopeFocusLow=false;
float microscopeButton=0,microscopeLight=0;
float microscopeFocus=0,microscopeFocusAngle=0;
float microscopeObjectiveAngle=0,microscopeObjectiveTarget=0;
const vec3 stations[]= {
    {
        -3.4,1.05,2
    }
    , {
        3.4,1.05,2
    }
    , {
        -3.4,1.05,-2
    }
    , {
        3.4,1.05,-2
    }
    , {
        -3.4,1.05,-6
    }
    , {
        3.4,1.05,-6
    }
    , {
        0,1.05,-9
    }
}
;
const char* names[]= {
    "Masa y resorte","Cristalizacion","Pendulo doble","Benceno","Microscopio","Van de Graaff","Prisma de Newton"
}
;
void uniform(Shader&s,const std::string&n,vec3 v) {
    glUniform3fv(glGetUniformLocation(s.Program,n.c_str()),1,glm::value_ptr(v));
}
void uniform(Shader&s,const std::string&n,float v) {
    glUniform1f(glGetUniformLocation(s.Program,n.c_str()),v);
}
void matrix(Shader&s,const char*n,const mat4&m) {
    glUniformMatrix4fv(glGetUniformLocation(s.Program,n),1,GL_FALSE,glm::value_ptr(m));
}
mat4 T(vec3 v) {
    return glm::translate(mat4(1),v);
}
mat4 S(vec3 v) {
    return glm::scale(mat4(1),v);
}
mat4 R(float angle,vec3 axis= {
    0,0,1
}
) {
    return glm::rotate(mat4(1),angle,axis);
}
float approach(float value,float target,float amount) {
    if(value<target)return std::min(value+amount,target);
    return std::max(value-amount,target);
}
// Maquina de estados como en la practica 9: primero se tira de la masa y luego
// se libera. La envolvente decreciente hace visible el amortiguamiento.
void animateSpring(float deltaTime) {
    const float estiramiento=.18f;
    if(estadoResorte==RESORTE_REPOSO)return;
    if(estadoResorte==RESORTE_ESTIRAR) {
        desplazamientoResorte=std::min(estiramiento,
                                      desplazamientoResorte+deltaTime*.30f);
        if(desplazamientoResorte>=estiramiento) {
            tiempoResorte=0;
            estadoResorte=RESORTE_OSCILAR;
        }
        return;
    }
    tiempoResorte+=deltaTime;
    float amplitud=estiramiento*std::exp(-1.05f*tiempoResorte);
    desplazamientoResorte=amplitud*std::cos(7.2f*tiempoResorte);
    if(amplitud<.002f) {
        desplazamientoResorte=0;
        estadoResorte=RESORTE_REPOSO;
    }
}
// Interpolacion entre cuadros clave siguiendo la estructura de la practica 10.
void animateStirrer(float deltaTime) {
    const std::array<vec3,5> cuadros={vec3(0),vec3(.045f,0,-.025f),
        vec3(0,0,-.05f),vec3(-.045f,0,-.025f),vec3(0)};
    if(!agitarVarilla)return;
    avanceVarilla+=deltaTime/0.28f;
    while(avanceVarilla>=1) {
        avanceVarilla-=1;
        cuadroVarilla=(cuadroVarilla+1)%4;
    }
    desplazamientoVarilla=glm::mix(cuadros[cuadroVarilla],
                                   cuadros[cuadroVarilla+1],avanceVarilla);
}
void animateVanDeGraaff(float deltaTime) {
    vanButton=approach(vanButton,vanDeGraaffOn?.020f:0.f,deltaTime*.14f);
    if(!vanDeGraaffOn)return;
    const float recorrido=2.033f-.660f;
    vanTravel=std::fmod(vanTravel+deltaTime*.34f,recorrido);
    vanRoll=std::fmod(vanRoll+deltaTime*3.24f,6.2831853f);
    vanCarousel=std::fmod(vanCarousel+deltaTime*4.5f,6.2831853f);
}
void animatePrism(float deltaTime) {
    oscuridadLaboratorio=approach(oscuridadLaboratorio,
        estadoPrisma==PRISMA_APAGADO?0.f:1.f,deltaTime*1.4f);
    if(estadoPrisma==PRISMA_ENTRADA) {
        avanceEntrada=std::min(1.f,avanceEntrada+deltaTime*.75f);
        if(avanceEntrada>=1)estadoPrisma=PRISMA_DISPERSION;
    }
    if(estadoPrisma==PRISMA_DISPERSION) {
        avanceEspectro=std::min(1.f,avanceEspectro+deltaTime*.34f);
        if(avanceEspectro>=1)estadoPrisma=PRISMA_COMPLETO;
    }
}
// Igual que en la maquina de estados de la practica 9: cada destino avanza
// con deltaTime y puede cambiar aunque las otras animaciones sigan en curso.
void animateMicroscope(float deltaTime) {
    microscopeButton=approach(microscopeButton,microscopeLightOn?.018f:0.f,
                              deltaTime*.12f);
    microscopeLight=approach(microscopeLight,microscopeLightOn?1.f:0.f,
                             deltaTime*4.f);
    microscopeObjectiveAngle=approach(microscopeObjectiveAngle,
                                      microscopeObjectiveTarget,
                                      deltaTime*glm::radians(150.f));
    float oldFocus=microscopeFocus;
    microscopeFocus=approach(microscopeFocus,
                             microscopeFocusLow?-.13f:0.f,deltaTime*.12f);
    microscopeFocusAngle+=(microscopeFocus-oldFocus)*32.f;
}
std::unique_ptr<Mesh> cube,quad,prism,beamWedge;
// Un cubo y un plano se reutilizan para toda la arquitectura y las fichas.
void primitives() {
    std::vector<Vertex>v;
    std::vector<GLuint>ind;
    vec3 normals[]= {
        {
            1,0,0
        }
        , {
            -1,0,0
        }
        , {
            0,1,0
        }
        , {
            0,-1,0
        }
        , {
            0,0,1
        }
        , {
            0,0,-1
        }
    }
    ;
    for(vec3 n:normals) {
        vec3 u=std::abs(n.y)>.5?vec3(1,0,0):glm::normalize(glm::cross(vec3(0,1,0),n));
        vec3 w=glm::cross(n,u);
        GLuint k=v.size();
        for(auto p:std::array<glm::vec2,4> {
            {
                {
                    0,0
                }
                , {
                    1,0
                }
                , {
                    1,1
                }
                , {
                    0,1
                }
            }
        }
        )v.push_back( {
            n*.5f+u*(p.x-.5f)+w*(p.y-.5f),n,p
        }
        );
        for(int j: {
            0,1,2,0,2,3
        }
        )ind.push_back(k+j);
    }
    cube=std::make_unique<Mesh>(v,ind,std::vector<Texture> {
    }
    );
    v= {
        {
            {
                -.5,-.5,0
            }
            , {
                0,0,1
            }
            , {
                0,1
            }
        }
        , {
            {
                .5,-.5,0
            }
            , {
                0,0,1
            }
            , {
                1,1
            }
        }
        , {
            {
                .5,.5,0
            }
            , {
                0,0,1
            }
            , {
                1,0
            }
        }
        , {
            {
                -.5,.5,0
            }
            , {
                0,0,1
            }
            , {
                0,0
            }
        }
    }
    ;
    quad=std::make_unique<Mesh>(v,std::vector<GLuint> {
        0,1,2,0,2,3
    }
    ,std::vector<Texture> {
    }
    );
    // Prisma triangular sencillo, construido igual que las primitivas del curso.
    std::vector<Vertex>pv;
    std::vector<GLuint>pi;
    auto triangle=[&](vec3 a,vec3 b,vec3 c) {
        vec3 n=glm::normalize(glm::cross(b-a,c-a));
        GLuint start=pv.size();
        pv.push_back({a,n,{0,0}});pv.push_back({b,n,{1,0}});pv.push_back({c,n,{.5,1}});
        pi.insert(pi.end(),{start,start+1,start+2});
    };
    vec3 a(-.28,0,.18),b(.28,0,.18),c(0,.48,.18);
    vec3 d(-.28,0,-.18),e(.28,0,-.18),f(0,.48,-.18);
    triangle(a,b,c);triangle(d,f,e);
    triangle(a,d,e);triangle(a,e,b);
    triangle(b,e,f);triangle(b,f,c);
    triangle(c,f,d);triangle(c,d,a);
    prism=std::make_unique<Mesh>(pv,pi,std::vector<Texture>{});
    // Plano trapezoidal; su orientacion se actualiza para mirar a la camara.
    pv={{{-.025f,-.5f,0},{0,0,1},{0,0}},
        {{ .025f,-.5f,0},{0,0,1},{1,0}},
        {{ .5f, .5f,0},{0,0,1},{1,1}},
        {{-.5f, .5f,0},{0,0,1},{0,1}}};
    pi={0,1,2,0,2,3};
    beamWedge=std::make_unique<Mesh>(pv,pi,std::vector<Texture>{});
}
void block(Shader&s,vec3 p,vec3 size,vec3 color) {
    matrix(s,"model",T(p)*S(size));
    cube->surface.diffuse=color;
    cube->Draw(s);
}
void line(Shader&s,vec3 a,vec3 b,float width,vec3 color) {
    vec3 d=b-a;
    vec3 unit=glm::normalize(d);
    vec3 axis=glm::cross(vec3(0,1,0),unit);
    mat4 rot=glm::length(axis)>.0001f?R(std::acos(glm::clamp(unit.y,-1.f,1.f)),glm::normalize(axis)):(unit.y<0?R(3.14159265f, {
        1,0,0
    }
    ):mat4(1));
    matrix(s,"model",T((a+b)*.5f)*rot*S( {
        width,glm::length(d),width
    }
    ));
    cube->surface.diffuse=color;
    cube->Draw(s);
}
void wideningBeam(Shader&s,vec3 a,vec3 b,float endWidth,vec3 color,float alpha) {
    if(glm::length(b-a)<.0001f)return;
    vec3 forward=glm::normalize(b-a),middle=(a+b)*.5f;
    vec3 towardCamera=glm::normalize(camera.GetPosition()-middle);
    vec3 side=glm::cross(forward,towardCamera);
    if(glm::length(side)<.001f)side=glm::cross(forward,vec3(0,1,0));
    side=glm::normalize(side);
    vec3 normal=glm::normalize(glm::cross(side,forward));
    mat4 model(1);
    model[0]=glm::vec4(side*endWidth,0);
    model[1]=glm::vec4(forward*glm::length(b-a),0);
    model[2]=glm::vec4(normal,0);
    model[3]=glm::vec4(middle,1);
    matrix(s,"model",model);
    uniform(s,"beamColor",color);
    uniform(s,"beamAlpha",alpha);
    beamWedge->Draw(s);
}
struct Part {
    int station;
    std::string name,parent;
    vec3 pivot;
    std::unique_ptr<Model>model;
}
;
std::vector<Part>parts;
GLuint labels[7],cards[7],hud,signTexture,sampleTexture;
// Cada OBJ tiene su propio pivote; no se fusionan las piezas móviles.
void load() {
    std::ifstream in("assets/models/parts.txt");
    if(!in)throw std::runtime_error("No se encontro parts.txt");
    const std::array<std::string,7> modelFolders= {
        "masa_resorte","balanza","pendulo_doble","benceno",
        "microscopio","van_de_graaff","equilibrio_fuerzas"
    };
    Part p;
    while(in>>p.station>>p.name>>p.parent>>p.pivot.x>>p.pivot.y>>p.pivot.z) {
        if(p.station<0||p.station>=static_cast<int>(modelFolders.size()))
            throw std::runtime_error("Estacion de modelo fuera de rango");
        auto file="assets/models/"+modelFolders[p.station]+"/"+
                  std::to_string(p.station)+"_"+p.name+".obj";
        p.model=std::make_unique<Model>(file.c_str());
        parts.push_back(std::move(p));
    }
    for(int i=0;i<7;++i) {
        labels[i]=TextureFromFile(("label"+std::to_string(i)+".png").c_str(),"assets/ui");
        cards[i]=TextureFromFile(("card"+std::to_string(i)+".png").c_str(),"assets/ui");
    }
    hud=TextureFromFile("hud.png","assets/ui");
    signTexture=TextureFromFile("sign.png","assets/ui");
    sampleTexture=TextureFromFile("sample.png","assets/ui");
}
void picture(Shader&s,GLuint texture,mat4 transform) {
    matrix(s,"model",transform);
    Texture tex;
    tex.id=texture;
    tex.type="texture_diffuse";
    quad->textures= {
        tex
    }
    ;
    quad->surface.diffuse=vec3(1);
    quad->Draw(s);
}
void room(Shader&s) {
    vec3 wall(.73,.79,.77),dark(.045,.14,.16),wood(.44,.29,.15),white(.83,.85,.81);
    block(s, {
        0,-.16,-2
    }
    , {
        12,.3,18
    }
    , {
        .38,.43,.43
    }
    );
    // Juntas del piso; la arquitectura usa la misma malla de cubo reutilizada.
    for(int x=-6;x<=6;++x)block(s, {
        float(x),.002,-2
    }
    , {
        .012,.006,18
    }
    , {
        .22,.28,.29
    }
    );
    for(int z=-11;z<=7;++z)block(s, {
        0,.003,float(z)
    }
    , {
        12,.006,.012
    }
    , {
        .22,.28,.29
    }
    );
    block(s, {
        -6,2.3,-2
    }
    , {
        .18,4.6,18
    }
    ,wall);
    block(s, {
        6,2.3,-2
    }
    , {
        .18,4.6,18
    }
    ,wall);
    block(s, {
        0,2.3,-11
    }
    , {
        12,4.6,.18
    }
    ,wall);
    block(s, {
        0,4.65,-2
    }
    , {
        12,.15,18
    }
    ,white);
    for(float x: {
        -4.f,4.f
    }
    )block(s, {
        x,2.3,7
    }
    , {
        4,4.6,.22
    }
    ,wall);
    block(s, {
        0,3.8,7
    }
    , {
        4,1.6,.22
    }
    ,dark);
    picture(s,signTexture,T( {
        0,3.8,7.13
    }
    )*S( {
        3.7,.68,1
    }
    ));
    picture(s,signTexture,T( {
        0,3.6,-10.88
    }
    )*S( {
        4.8,.88,1
    }
    ));
    for(float x: {
        -5.87f,5.87f
    }
    ) {
        block(s, {
            x,.18,-2
        }
        , {
            .08,.3,18
        }
        ,dark);
        for(float z: {
            -7.f,-2.f,3.f
        }
        ) {
            block(s, {
                x,2.8,z
            }
            , {
                .06,1.5,2.8
            }
            , {
                .12,.27,.32
            }
            );
            for(float dz: {
                -.95f,0.f,.95f
            }
            )block(s, {
                x+(x<0?.035f:-.035f),2.8,z+dz
            }
            , {
                .025,1.5,.03
            }
            ,white);
        }
    }
    for(float z: {
        3.f,-2.f,-7.f
    }
    ) {
        block(s, {
            0,4.54,z
        }
        , {
            5,.08,.3
        }
        , {
            .95,.89,.68
        }
        );
    }
    for(int i=0;i<7;++i) {
        vec3 p=stations[i];
        block(s,p-vec3(0,.08,0), {
            2.6,.16,1.8
        }
        ,wood);
        block(s,p-vec3(0,.2,0), {
            2.4,.12,1.6
        }
        ,dark);
        for(float x: {
            -1.05f,1.05f
        }
        )for(float z: {
            -.65f,.65f
        }
        )block(s, {
            p.x+x,.44,p.z+z
        }
        , {
            .09,.88,.09
        }
        ,dark);
        picture(s,labels[i],T(p+vec3(.72,.015,.55))*R(-1.5707963f, {
            1,0,0
        }
        )*S( {
            .83,.42,1
        }
        ));
    }
}
// Conservamos el marco del padre antes de añadir el movimiento del hijo.
void apparatus(Shader&s) {
    std::map<std::string,mat4>world;
    for(auto&p:parts) {
        mat4 m=T(p.pivot);
        int i=p.station;
        const auto&n=p.name;
        if(i==0) {
            // El resorte se exporta con su extremo superior en el origen local.
            // Al escalar desde ese punto, la unión con el soporte permanece fija.
            const float springTop=1.518701f;
            const float restLength=.838133f;
            if(n=="spring")m=T( {
                0,springTop,0
            }
            )*S( {
                1,1.0f+desplazamientoResorte/restLength,1
            }
            );
            if(n=="mass")m=T( {
                0,-desplazamientoResorte,0
            }
            );
        }
        if(i==1&&n=="stirrer")m=T(p.pivot+desplazamientoVarilla);
        if(i==2) {
            if(n=="arm1")m=T( {
                0,1.9,0
            }
            )*R(simulation.pendulum[0]);
            if(n=="arm2")m=world["2arm1"]*T( {
                0,-.65,0
            }
            )*R(simulation.pendulum[1]-simulation.pendulum[0]);
        }
        if(i==4) {
            if(n=="button")m=T(p.pivot+vec3(0,-microscopeButton,0));
            if(n=="focusL")m=T(p.pivot)*R(microscopeFocusAngle,vec3(1,0,0));
            if(n=="focusR")m=T(p.pivot)*R(-microscopeFocusAngle,vec3(1,0,0));
            if(n=="head"||n=="eyepiece")
                m=T(p.pivot+vec3(0,microscopeFocus,0));
            if(n=="nosepiece"||n=="objectives")
                m=T(p.pivot+vec3(0,microscopeFocus,0))*
                  R(microscopeObjectiveAngle,vec3(0,1,0));
        }
        if(i==5) {
            if(n=="button")m=T(p.pivot+vec3(0,0,-vanButton));
            if(n=="lower"||n=="upper")m=T(p.pivot)*R(-vanRoll,vec3(1,0,0));
            if(n=="carousel")m=T(p.pivot)*R(vanCarousel,vec3(0,1,0));
            if(n.rfind("front",0)==0||n.rfind("back",0)==0) {
                const float lower=.660f,span=2.033f-lower;
                float direction=n.rfind("front",0)==0?1.f:-1.f;
                float relative=std::fmod(p.pivot.y-lower+direction*vanTravel,span);
                if(relative<0)relative+=span;
                m=T(p.pivot+vec3(0,lower+relative-p.pivot.y,0));
            }
        }
        world[std::to_string(i)+n]=m;
        bool transparent=(i==1&&(n=="solution"||n=="crystals"||
                                 n=="glass"||n=="stirrer"))||
                         (i==5&&n=="vinyl");
        if(transparent) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
        }
        else {
            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
        }
        glUniform1i(glGetUniformLocation(s.Program,"transparency"),transparent?1:0);
        // La estación química conserva su composición, pero se reduce completa
        // para que el frasco y las muestras permanezcan dentro de la mesa.
        mat4 stationScale=i==1?S(vec3(.82f)):mat4(1);
        matrix(s,"model",T(stations[i])*stationScale*m);
        p.model->Draw(s);
    }
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glUniform1i(glGetUniformLocation(s.Program,"transparency"),0);
}
void newtonExperiment(Shader&s,Shader&beamShader,const mat4&view,const mat4&projection) {
    const vec3 origin=stations[6];
    const vec3 prismCenter=origin+vec3(0,.08,0);
    const vec3 inputStart=origin+vec3(0,.30,.68);
    const vec3 inputEnd=origin+vec3(0,.30,.18);
    const vec3 outputStart=origin+vec3(0,.31,-.18);
    const std::array<vec3,7>colors={vec3(.95,.03,.02),vec3(1,.25,.02),
        vec3(1,.78,.03),vec3(.05,.85,.15),vec3(.04,.35,1),
        vec3(.20,.08,.72),vec3(.58,.05,.92)};
    // Pizarron mate sobre la pared posterior y soporte del prisma.
    block(s,{0,2.35,-10.84},{3.15,1.55,.055},{.88,.88,.84});
    block(s,prismCenter+vec3(0,-.02,0),{.78,.06,.58},{.10,.13,.14});
    block(s,inputStart+vec3(0,0,.13),{.34,.28,.32},{.055,.065,.07});
    block(s,inputStart+vec3(0,0,-.04),{.10,.10,.035},{.92,.88,.68});

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glUniform1i(glGetUniformLocation(s.Program,"transparency"),1);
    prism->surface.diffuse={.55,.82,.88};
    prism->surface.specular=vec3(.8);
    prism->surface.opacity=.30f;
    matrix(s,"model",T(prismCenter));
    prism->Draw(s);
    if(estadoPrisma!=PRISMA_APAGADO) {
        // Los haces no reciben Phong: emiten color y usan mezcla aditiva.
        beamShader.Use();
        matrix(beamShader,"view",view);
        matrix(beamShader,"projection",projection);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE);
        wideningBeam(beamShader,inputStart,glm::mix(inputStart,inputEnd,avanceEntrada),
                     .075f,{1,1,.92},1.f);
        if(estadoPrisma>=PRISMA_DISPERSION)for(int i=0;i<7;++i) {
            float progress=glm::clamp(avanceEspectro*7-i,0.f,1.f);
            if(progress<=0)continue;
            vec3 target(-1.05f+i*.35f,2.28f,-10.76f);
            wideningBeam(beamShader,outputStart,glm::mix(outputStart,target,progress),
                         .04f+.31f*progress,colors[i],.62f);
        }
        // Se vuelve al shader principal; la proyeccion queda a cargo de las luces.
        s.Use();
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glUniform1i(glGetUniformLocation(s.Program,"transparency"),1);
    }
    cube->surface.opacity=1;
    prism->surface.opacity=1;
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glUniform1i(glGetUniformLocation(s.Program,"transparency"),0);
}
void go(int i) {
    selected=i;
    card=false;
    showSample=false;
    camera=Camera(stations[i]+vec3(0,1.7,3.2),vec3(0,1,0),-90,-10);
}
void key(GLFWwindow*w,int k,int,int action,int) {
    if(action!=GLFW_PRESS)return;
    if(k==GLFW_KEY_ESCAPE) {
        if(card||showSample) {
            card=false;
            showSample=false;
        }
        else glfwSetWindowShouldClose(w,true);
    }
    if(k>=GLFW_KEY_1&&k<=GLFW_KEY_7)go(k-GLFW_KEY_1);
    if(k==GLFW_KEY_SPACE)running=!running;
    if(k==GLFW_KEY_R)simulation.reset();
    if(k==GLFW_KEY_C) {
        simulation.preset=(simulation.preset+1)%3;
        simulation.reset();
    }
    if(selected==0&&k==GLFW_KEY_P) {
        if(estadoResorte==RESORTE_REPOSO) {
            desplazamientoResorte=0;
            tiempoResorte=0;
            estadoResorte=RESORTE_ESTIRAR;
        }
        else {
            desplazamientoResorte=0;
            estadoResorte=RESORTE_REPOSO;
        }
    }
    if(selected==1&&k==GLFW_KEY_P) {
        agitarVarilla=!agitarVarilla;
        if(!agitarVarilla) {
            cuadroVarilla=0;
            avanceVarilla=0;
            desplazamientoVarilla=vec3(0);
        }
    }
    if(selected==2&&k==GLFW_KEY_P)
        simulation.pendulumRunning=!simulation.pendulumRunning;
    if(selected==5&&k==GLFW_KEY_P)vanDeGraaffOn=!vanDeGraaffOn;
    if(selected==6&&k==GLFW_KEY_P) {
        if(estadoPrisma==PRISMA_APAGADO) {
            avanceEntrada=0;
            avanceEspectro=0;
            estadoPrisma=PRISMA_ENTRADA;
        }
        else {
            estadoPrisma=PRISMA_APAGADO;
            avanceEntrada=0;
            avanceEspectro=0;
        }
    }
    if(selected==4&&k==GLFW_KEY_L)microscopeLightOn=!microscopeLightOn;
    if(selected==4&&k==GLFW_KEY_O)
        microscopeObjectiveTarget+=glm::radians(120.f);
    if(selected==4&&k==GLFW_KEY_F)microscopeFocusLow=!microscopeFocusLow;
    if(k==GLFW_KEY_E) {
        float best=1e6;
        for(int i=0;i<7;++i) {
            float d=glm::distance(camera.GetPosition(),stations[i]);
            if(d<best) {
                best=d;
                selected=i;
            }
        }
        if(best<4)card=!card;
    }
    if(k==GLFW_KEY_M&&selected==4&&card)showSample=!showSample;
}
void mouse(GLFWwindow*w,double x,double y) {
    static double px=x,py=y;
    if(glfwGetMouseButton(w,GLFW_MOUSE_BUTTON_RIGHT)==GLFW_PRESS&&!card)camera.ProcessMouseMovement(x-px,py-y);
    px=x;
    py=y;
}
// El clic se convierte en un rayo y se comprueba contra la hoja sobre la mesa.
void click(GLFWwindow*w,int button,int action,int) {
    if(button!=GLFW_MOUSE_BUTTON_LEFT||action!=GLFW_PRESS)return;
    if(card) {
        card=false;
        showSample=false;
        return;
    }
    int width,height;
    glfwGetWindowSize(w,&width,&height);
    double x,y;
    glfwGetCursorPos(w,&x,&y);
    mat4 projection=glm::perspective(glm::radians(camera.GetZoom()),float(width)/height,.1f,80.f);
    glm::vec4 v=glm::inverse(projection*camera.GetViewMatrix())*glm::vec4(2*x/width-1,1-2*y/height,1,1);
    vec3 dir=glm::normalize(vec3(v)/v.w-camera.GetPosition());
    if(std::abs(dir.y)<.0001)return;
    for(int i=0;i<7;++i) {
        vec3 center=stations[i]+vec3(.72,.015,.55);
        float t=(center.y-camera.GetPosition().y)/dir.y;
        vec3 hit=camera.GetPosition()+t*dir;
        if(t>0&&t<5&&std::abs(hit.x-center.x)<.415&&std::abs(hit.z-center.z)<.21) {
            selected=i;
            card=true;
            break;
        }
    }
}
// Luz general y cuatro fuentes interiores, usando el shader de la práctica 8.
void lights(Shader&s) {
    float roomLevel=1.f-.88f*oscuridadLaboratorio;
    uniform(s,"viewPos",camera.GetPosition());
    uniform(s,"objectTint",vec3(1));
    uniform(s,"objectAlpha",1);
    uniform(s,"dirLight.direction", {
        -.3,-1,-.2
    }
    );
    uniform(s,"dirLight.ambient",vec3(.32)*roomLevel);
    uniform(s,"dirLight.diffuse",vec3(.42)*roomLevel);
    uniform(s,"dirLight.specular",vec3(.18)*roomLevel);
    const std::array<vec3,4>projectionColors={vec3(1,.04,.02),vec3(1,.72,.03),
        vec3(.04,.85,.18),vec3(.08,.25,1)};
    for(int i=0;i<4;++i) {
        auto n="pointLights["+std::to_string(i)+"].";
        bool projection=estadoPrisma!=PRISMA_APAGADO;
        uniform(s,n+"position",projection?vec3(-.9f+i*.6f,2.28f,-10.52f):
            vec3(i%2?-3.f:3.f,3.9f,i<2?1.f:-7.f));
        uniform(s,n+"constant",1);
        uniform(s,n+"linear",projection?.7f:.09f);
        uniform(s,n+"quadratic",projection?1.8f:.032f);
        vec3 color=projection?projectionColors[i]*avanceEspectro:vec3(.65,.59,.46)*roomLevel;
        uniform(s,n+"ambient",projection?color*.035f:vec3(.015)*roomLevel);
        uniform(s,n+"diffuse",color);
        uniform(s,n+"specular",projection?color*.45f:vec3(.5)*roomLevel);
    }
    uniform(s,"spotLight.constant",1);
    uniform(s,"spotLight.linear",1.4);
    uniform(s,"spotLight.quadratic",2.8);
    // La lampara nace debajo de la platina y apunta hacia la muestra.
    uniform(s,"spotLight.position",stations[4]+vec3(0,.43,.274f));
    uniform(s,"spotLight.cutOff",std::cos(glm::radians(16.f)));
    uniform(s,"spotLight.outerCutOff",std::cos(glm::radians(24.f)));
    uniform(s,"spotLight.direction", {
        0,1,0
    }
    );
    uniform(s,"spotLight.ambient",vec3(0));
    uniform(s,"spotLight.diffuse",vec3(1.f,.91f,.68f)*microscopeLight);
    uniform(s,"spotLight.specular",vec3(.65f,.58f,.42f)*microscopeLight);
}
void screenshot(const std::string&file,int w,int h) {
    std::vector<unsigned char>pixels(w*h*3);
    glPixelStorei(GL_PACK_ALIGNMENT,1);
    glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    fs::create_directories(fs::path(file).parent_path());
    std::ofstream out(file,std::ios::binary);
    if(!out)throw std::runtime_error("No se pudo guardar la captura");
    out<<"P6\n"<<w<<" "<<h<<"\n255\n";
    for(int y=h-1;y>=0;--y)out.write((char*)pixels.data()+y*w*3,w*3);
}
int main(int argc,char**argv) {
    try {
        bool smoke=argc>1&&std::string(argv[1])=="--smoke";
        fs::path exe=fs::absolute(argv[0]).parent_path();
        if(!fs::exists("assets/models/parts.txt")) {
            if(fs::exists(exe/"assets"))fs::current_path(exe);
            else fs::current_path(exe.parent_path());
        }
        if(!glfwInit())throw std::runtime_error("GLFW no pudo iniciar");
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
        glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);
        glfwWindowHint(GLFW_SAMPLES,4);
        GLFWwindow*w=glfwCreateWindow(1440,900,"Laboratorio de materia, fuerzas y movimiento",nullptr,nullptr);
        if(!w)throw std::runtime_error("No se pudo crear la ventana OpenGL");
        glfwMakeContextCurrent(w);
        glewExperimental=GL_TRUE;
        if(glewInit()!=GLEW_OK)throw std::runtime_error("GLEW no pudo iniciar");
        glGetError();
        glfwSwapInterval(smoke?0:1);
        glfwSetKeyCallback(w,key);
        glfwSetCursorPosCallback(w,mouse);
        glfwSetMouseButtonCallback(w,click);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_MULTISAMPLE);
        Shader shader("shaders/lighting.vs","shaders/lighting.frag"),
               beamShader("shaders/beam.vs","shaders/beam.frag"),
               ui("shaders/ui.vs","shaders/ui.frag");
        for(GLuint p: {
            shader.Program,beamShader.Program,ui.Program
        }
        ) {
            GLint ok;
            glGetProgramiv(p,GL_LINK_STATUS,&ok);
            if(!ok)throw std::runtime_error("Fallo al enlazar shaders");
        }
        primitives();
        load();
        double last=glfwGetTime(),accumulator=0;
        int frame=0;
        while(!glfwWindowShouldClose(w)) {
            double now=glfwGetTime(),dt=std::min(now-last,.05);
            last=now;
            glfwPollEvents();
            if(smoke)dt=1./60;
            if(!card) {
                if(glfwGetKey(w,GLFW_KEY_W))camera.ProcessKeyboard(FORWARD,dt);
                if(glfwGetKey(w,GLFW_KEY_S))camera.ProcessKeyboard(BACKWARD,dt);
                if(glfwGetKey(w,GLFW_KEY_A))camera.ProcessKeyboard(LEFT,dt);
                if(glfwGetKey(w,GLFW_KEY_D))camera.ProcessKeyboard(RIGHT,dt);
            }
            accumulator+=dt;
            while(accumulator>=1./240) {
                simulation.step(1./240,running);
                accumulator-=1./240;
            }
            animateSpring(static_cast<float>(dt));
            animateStirrer(static_cast<float>(dt));
            animateVanDeGraaff(static_cast<float>(dt));
            animatePrism(static_cast<float>(dt));
            animateMicroscope(static_cast<float>(dt));
            if(smoke&&frame%12==0&&frame/12<7)go(frame/12);
            int width,height;
            glfwGetFramebufferSize(w,&width,&height);
            if(height==0)continue;
            glViewport(0,0,width,height);
            glClearColor(.09,.15,.18,1);
            glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
            shader.Use();
            mat4 view=camera.GetViewMatrix();
            mat4 projection=glm::perspective(glm::radians(camera.GetZoom()),float(width)/height,.1f,80.f);
            matrix(shader,"view",view);
            matrix(shader,"projection",projection);
            lights(shader);
            room(shader);
            apparatus(shader);
            newtonExperiment(shader,beamShader,view,projection);
            glDisable(GL_DEPTH_TEST);
            ui.Use();
            matrix(ui,"projection",glm::ortho(0.f,float(width),0.f,float(height)));
            glUniform1i(glGetUniformLocation(ui.Program,"image"),0);
            picture(ui,hud,T( {
                width*.5f,height-48.f,0
            }
            )*S( {
                float(width),96,1
            }
            ));
            if(card) {
                float h=std::min(height*.78f,width*.7f*.72f),ww=h/ .72f;
                picture(ui,showSample?sampleTexture:cards[selected],T( {
                    width*.5f,height*.46f,0
                }
                )*S( {
                    ww,h,1
                }
                ));
            }
            glEnable(GL_DEPTH_TEST);
            if(smoke&&frame%12==10)screenshot((fs::temp_directory_path()/"laboratorio-capturas/station").string()+std::to_string(frame/12)+".ppm",width,height);
            if(smoke&&frame==84) {
                go(4);
                int ww,hh;
                glfwGetWindowSize(w,&ww,&hh);
                glm::vec4 pos=glm::perspective(glm::radians(camera.GetZoom()),float(ww)/hh,.1f,80.f)*camera.GetViewMatrix()*glm::vec4(stations[4]+vec3(.72,.015,.55),1);
                glfwSetCursorPos(w,(pos.x/pos.w+1)*ww/2,(1-pos.y/pos.w)*hh/2);
                click(w,GLFW_MOUSE_BUTTON_LEFT,GLFW_PRESS,0);
                if(!card||selected!=4)throw std::runtime_error("La seleccion de la hoja fallo");
                key(w,GLFW_KEY_SPACE,0,GLFW_PRESS,0);
                if(running)throw std::runtime_error("Pausa incorrecta");
                key(w,GLFW_KEY_C,0,GLFW_PRESS,0);
                key(w,GLFW_KEY_R,0,GLFW_PRESS,0);
            }
            if(smoke&&frame==86)screenshot((fs::temp_directory_path()/"laboratorio-capturas/card.ppm").string(),width,height);
            GLenum error=glGetError();
            if(error!=GL_NO_ERROR)throw std::runtime_error("Error OpenGL: "+std::to_string(error));
            if(smoke&&frame==87) {
                key(w,GLFW_KEY_M,0,GLFW_PRESS,0);
                if(!showSample)throw std::runtime_error("Muestra incorrecta");
            }
            if(smoke&&frame==89) {
                card=false;
                showSample=false;
                camera=Camera(vec3(0,2.4,12),vec3(0,1,0),-90,0);
            }
            if(smoke&&frame==91)screenshot((fs::temp_directory_path()/"laboratorio-capturas/facade.ppm").string(),width,height);
            glfwSwapBuffers(w);
            ++frame;
            if(smoke&&frame>92)break;
            std::string title=std::string("Laboratorio | ")+names[selected]+(running?" | En marcha":" | Pausa")+" | Configuracion "+std::to_string(simulation.preset+1);
            glfwSetWindowTitle(w,title.c_str());
        }
        std::cout<<"Render finalizado: "<<frame<<" cuadros, "<<parts.size()<<" piezas.\n";
        glfwTerminate();
        return 0;
    }
    catch(const std::exception&e) {
        std::cerr<<e.what()<<"\n";
        return 1;
    }
}
