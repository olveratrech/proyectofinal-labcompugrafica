// Integra las clases base de las prácticas con la escena de RenderLab.
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
constexpr float WALK_HEIGHT=1.95f;
Camera camera(vec3(0,WALK_HEIGHT,8),vec3(0,1,0),-90,-8);
lab::Physics simulation;
bool card=false;
int selected=0;
float cardProgress=0;
bool cameraTraveling=false;
float travelProgress=0,travelDuration=1;
vec3 travelStart(0),travelEnd(0);
float travelStartYaw=-90,travelStartPitch=-8;
enum EstadoResorte {RESORTE_REPOSO,RESORTE_ESTIRAR,RESORTE_OSCILAR};
EstadoResorte estadoResorte=RESORTE_REPOSO;
float desplazamientoResorte=0,tiempoResorte=0;
bool agitarVarilla=false;
int cuadroVarilla=0;
float avanceVarilla=0;
vec3 desplazamientoVarilla(0);
bool arquimedesSumergido=false;
float avanceArquimedes=0;
bool vanDeGraaffOn=false;
float vanButton=0,vanTravel=0,vanRoll=0,vanCarousel=0;
enum EstadoPrisma {PRISMA_APAGADO,PRISMA_ENTRADA,PRISMA_DISPERSION,PRISMA_COMPLETO};
EstadoPrisma estadoPrisma=PRISMA_APAGADO;
float avanceEntrada=0,avanceEspectro=0,oscuridadLaboratorio=0;
// Los controles del microscopio conservan destinos independientes.
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
// La etiqueta conserva su proporción y un margen respecto a la esquina de la mesa.
const vec3 labelOffset(.88f,.015f,.891333f);
constexpr float labelWidth=1.30f;
constexpr float labelHeight=labelWidth*192.f/900.f;
const vec3 microscopeOffset(0,0,-.10f);
const char* names[]= {
    "Ley de Hooke","Cristalizacion","Pendulo doble","Principio de Arquimedes","Microscopio","Van de Graaff","Prisma de Newton"
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
// El resorte se estira y después oscila hasta que su amplitud es casi nula.
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
// La varilla interpola un recorrido cerrado definido por cinco posiciones.
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
// Una variable coordina el descenso del cuerpo y la respuesta del instrumento.
void animateArchimedes(float deltaTime) {
    avanceArquimedes=approach(avanceArquimedes,
                              arquimedesSumergido?1.f:0.f,deltaTime*.42f);
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
// Cada control se aproxima a su destino sin bloquear a los demás.
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
std::unique_ptr<Mesh> cube,quad,floorPlane,prism,beamWedge;
std::unique_ptr<Model> windowFrame,windowMetal,windowGlass;
GLuint floorTexture=0,tableTexture=0,skyboxTexture=0;
// Estas primitivas se reutilizan en la arquitectura, el mobiliario y la interfaz.
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
    // Plano horizontal con UV mayores que uno: GL_REPEAT forma el mosaico.
    v={{{-.5f,0,-.5f},{0,1,0},{0,0}},
       {{ .5f,0,-.5f},{0,1,0},{8,0}},
       {{ .5f,0, .5f},{0,1,0},{8,10}},
       {{-.5f,0, .5f},{0,1,0},{0,10}}};
    floorPlane=std::make_unique<Mesh>(v,std::vector<GLuint>{0,1,2,0,2,3},
                                      std::vector<Texture>{});
    // El prisma se arma con una malla triangular de baja complejidad.
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
    // El plano trapezoidal se orienta hacia la cámara durante el dibujo.
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
void archimedesWater(Shader&s) {
    float t=avanceArquimedes*avanceArquimedes*(3.f-2.f*avanceArquimedes);
    // Primero se llena el tanque hasta el tubo. El resto del desplazamiento
    // aparece en el vaso receptor, sin representar un chorro intermedio.
    const float overflowStart=.55f;
    float tankPhase=glm::clamp(t/overflowStart,0.f,1.f);
    float cupPhase=glm::clamp((t-overflowStart)/(1.f-overflowStart),0.f,1.f);
    cupPhase=cupPhase*cupPhase*(3.f-2.f*cupPhase);
    const float tankBottom=.175f;
    const float tankSurface=.64f+.055f*tankPhase;
    const float tankHeight=tankSurface-tankBottom;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glUniform1i(glGetUniformLocation(s.Program,"transparency"),1);
    // El volumen representa el agua y una lámina delgada marca su nivel.
    cube->surface.opacity=.64f;
    block(s,stations[3]+vec3(.20f,tankBottom+tankHeight*.5f,.04f),
          vec3(.78f,tankHeight,.54f),vec3(.018f,.25f,.62f));
    cube->surface.opacity=.82f;
    block(s,stations[3]+vec3(.20f,tankSurface,.04f),
          vec3(.78f,.012f,.54f),vec3(.06f,.40f,.78f));
    if(cupPhase>.001f) {
        const float cupBottom=.145f;
        const float cupHeight=.235f*cupPhase;
        cube->surface.opacity=.64f;
        block(s,stations[3]+vec3(.88f,cupBottom+cupHeight*.5f,.04f),
              vec3(.235f,cupHeight,.235f),vec3(.018f,.25f,.62f));
        cube->surface.opacity=.82f;
        block(s,stations[3]+vec3(.88f,cupBottom+cupHeight,.04f),
              vec3(.235f,.010f,.235f),vec3(.06f,.40f,.78f));
    }
    cube->surface.opacity=1.f;
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glUniform1i(glGetUniformLocation(s.Program,"transparency"),0);
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
GLuint labels[7],cards[7],posters[4],signals[3],hud,signTexture;
GLuint loadCubemap() {
    const std::array<std::string,6>faces={
        "right.png","left.png","top.png","bottom.png","front.png","back.png"
    };
    GLuint texture;
    glGenTextures(1,&texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP,texture);
    for(size_t i=0;i<faces.size();++i) {
        std::string file="assets/textures/environment/skybox/"+faces[i];
        int width=0,height=0,channels=0;
        unsigned char*image=stbi_load(file.c_str(),&width,&height,&channels,STBI_rgb);
        if(!image)throw std::runtime_error("No se pudo cargar cara del skybox: "+file);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,0,GL_RGB,width,height,0,
                     GL_RGB,GL_UNSIGNED_BYTE,image);
        stbi_image_free(image);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP,0);
    return texture;
}
// El manifiesto mantiene cada OBJ separado para conservar su pivote.
void load() {
    std::ifstream in("assets/models/parts.txt");
    if(!in)throw std::runtime_error("No se encontro parts.txt");
    const std::array<std::string,6> modelFolders= {
        "ley_de_hooke","cristalizacion","pendulo_doble","arquimedes",
        "microscopio","van_de_graaff"
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
    for(int i=0;i<4;++i)
        posters[i]=TextureFromFile(("poster"+std::to_string(i+1)+".png").c_str(),
                                   "assets/ui");
    for(int i=0;i<3;++i)
        signals[i]=TextureFromFile(("senal"+std::to_string(i+1)+".png").c_str(),
                                   "assets/ui");
    hud=TextureFromFile("hud.png","assets/ui");
    signTexture=TextureFromFile("sign.png","assets/ui");
    floorTexture=TextureFromFile("tile.jpeg","assets/textures");
    tableTexture=TextureFromFile("table_wood.jpg","assets/textures/environment");
    skyboxTexture=loadCubemap();
    windowFrame=std::make_unique<Model>("assets/models/environment/window_frame.obj");
    windowMetal=std::make_unique<Model>("assets/models/environment/window_metal.obj");
    windowGlass=std::make_unique<Model>("assets/models/environment/window_glass.obj");
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
// La mezcla alfa se limita al dibujo de la imagen y después se restaura.
void alphaPicture(Shader&s,GLuint texture,mat4 transform) {
    GLboolean blendWasEnabled=glIsEnabled(GL_BLEND);
    GLboolean previousDepthMask=GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK,&previousDepthMask);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glUniform1i(glGetUniformLocation(s.Program,"transparency"),1);
    picture(s,texture,transform);
    glUniform1i(glGetUniformLocation(s.Program,"transparency"),0);
    glDepthMask(previousDepthMask);
    if(!blendWasEnabled)glDisable(GL_BLEND);
}
void drawSkybox(Shader&s,const mat4&view,const mat4&projection) {
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    s.Use();
    matrix(s,"view",mat4(glm::mat3(view)));
    matrix(s,"projection",projection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP,skyboxTexture);
    glUniform1i(glGetUniformLocation(s.Program,"skybox"),0);
    cube->Draw(s);
    glBindTexture(GL_TEXTURE_CUBE_MAP,0);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}
void windows(Shader&s) {
    const std::array<float,3>centers={-7.f,-2.f,3.f};
    auto transform=[](float x,float z) {
        float angle=x<0?glm::radians(90.f):glm::radians(-90.f);
        return T(vec3(x,1.55f,z))*R(angle,vec3(0,1,0))*
               S(vec3(2.7f,1.85f,1.85f));
    };
    for(float x:{-7.82f,7.82f})for(float z:centers) {
        mat4 model=transform(x,z);
        matrix(s,"model",model);
        windowFrame->Draw(s);
        matrix(s,"model",model);
        windowMetal->Draw(s);
    }
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glUniform1i(glGetUniformLocation(s.Program,"transparency"),1);
    for(float x:{-7.82f,7.82f})for(float z:centers) {
        matrix(s,"model",transform(x,z));
        windowGlass->Draw(s);
    }
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glUniform1i(glGetUniformLocation(s.Program,"transparency"),0);
}
void room(Shader&s) {
    vec3 wall(.91,.92,.90),dark(.045,.07,.08),white(.97,.97,.94);
    Texture tile;
    tile.id=floorTexture;
    tile.type="texture_diffuse";
    floorPlane->textures={tile};
    floorPlane->surface.diffuse=vec3(1);
    matrix(s,"model",T(vec3(0,-.01f,-1))*S(vec3(16,1,20)));
    floorPlane->Draw(s);
    // Los muros laterales se dividen para dejar espacio a las ventanas.
    for(float x:{-7.92f,7.92f}) {
        block(s,{x,.75f,-1},{.16f,1.5f,20},wall);
        block(s,{x,4.5f,-1},{.16f,1.6f,20},wall);
        const std::array<glm::vec2,4>segments={glm::vec2(-11,-8.25f),
            glm::vec2(-5.75f,-3.25f),glm::vec2(-.75f,1.75f),
            glm::vec2(4.25f,9)};
        for(auto interval:segments)
            block(s,{x,2.6f,(interval.x+interval.y)*.5f},
                  {.16f,2.2f,interval.y-interval.x},wall);
    }
    block(s, {
        0,2.65,-11
    }
    , {
        16,5.3,.18
    }
    ,wall);
    block(s, {
        0,5.32,-1
    }
    , {
        16,.14,20
    }
    ,white);
    // La escena se presenta desde el interior y el muro frontal queda cerrado.
    block(s,{0,2.65f,9},{16,5.3f,.22f},wall);
    constexpr float signWidth=8.4f;
    constexpr float signHeight=signWidth*606.f/4512.f;
    alphaPicture(s,signTexture,T(vec3(0,3.80f,-10.88f))*
            S(vec3(signWidth,signHeight,1)));

    // Los carteles se reparten entre el fondo y los muros laterales.
    constexpr float posterWidth=1.725f;
    constexpr float posterHeight=posterWidth*2400.f/1800.f;
    alphaPicture(s,posters[0],T(vec3(-5.3f,2.55f,-10.88f))*
            S(vec3(posterWidth,posterHeight,1)));
    alphaPicture(s,posters[1],T(vec3(5.3f,2.55f,-10.88f))*
            S(vec3(posterWidth,posterHeight,1)));
    alphaPicture(s,posters[2],T(vec3(-7.80f,2.55f,-4.5f))*
            R(glm::radians(90.f),vec3(0,1,0))*
            S(vec3(posterWidth,posterHeight,1)));
    alphaPicture(s,posters[3],T(vec3(7.80f,2.55f,-4.5f))*
            R(glm::radians(-90.f),vec3(0,1,0))*
            S(vec3(posterWidth,posterHeight,1)));

    // Las señales de seguridad ocupan los espacios libres de los muros laterales.
    constexpr float signalSmallWidth=1.80f;
    constexpr float signalLargeWidth=3.375f;
    alphaPicture(s,signals[0],T(vec3(-7.80f,2.65f,.5f))*
            R(glm::radians(90.f),vec3(0,1,0))*
            S(vec3(signalSmallWidth,signalSmallWidth*717.f/1104.f,1)));
    alphaPicture(s,signals[1],T(vec3(7.80f,2.65f,.5f))*
            R(glm::radians(-90.f),vec3(0,1,0))*
            S(vec3(signalSmallWidth,signalSmallWidth*651.f/1104.f,1)));
    alphaPicture(s,signals[2],T(vec3(-7.80f,2.65f,6.6f))*
            R(glm::radians(90.f),vec3(0,1,0))*
            S(vec3(signalLargeWidth,signalLargeWidth*1740.f/3072.f,1)));
    windows(s);
    for(float z: {
        3.f,-2.f,-7.f
    }
    ) {
        block(s, {
            0,5.18,z
        }
        , {
            6,.08,.34
        }
        , {
            .95,.89,.68
        }
        );
    }
    for(int i=0;i<7;++i) {
        vec3 p=stations[i];
        block(s,p-vec3(0,.08,0),{3.3f,.16f,2.3f},vec3(.72,.61,.47));
        picture(s,tableTexture,T(p+vec3(0,.006f,0))*
                R(-1.5707963f,vec3(1,0,0))*S(vec3(3.16f,2.16f,1)));
        block(s,p-vec3(0,.2,0), {
            3.1,.12,2.1
        }
        ,dark);
        for(float x: {
            -1.35f,1.35f
        }
        )for(float z: {
            -.88f,.88f
        }
        )block(s, {
            p.x+x,.44,p.z+z
        }
        , {
            .10,.88,.10
        }
        ,dark);
        alphaPicture(s,labels[i],T(p+labelOffset)*R(-1.5707963f, {
            1,0,0
        }
        )*S( {
            labelWidth,labelHeight,1
        }
        ));
    }
}
// La matriz de cada pieza se conserva para reutilizarla en sus hijos.
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
        if(i==3) {
            float t=avanceArquimedes*avanceArquimedes*
                    (3.f-2.f*avanceArquimedes);
            if(n=="suspended")m=T(p.pivot+vec3(0,-.50f*t,0));
            if(n=="spring")m=T(p.pivot)*S(vec3(1,1.f+1.675f*t,1));
            if(n=="indicator")m=T(p.pivot+vec3(0,.06f*t,0));
            if(n=="spout")m=T(p.pivot+vec3(0,-.07f,0));
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
        // El agua se dibuja después de las piezas opacas y antes del tanque.
        if(i==3&&n=="glass")archimedesWater(s);
        bool transparent=(i==1&&(n=="solution"||n=="crystals"||
                                 n=="glass"||n=="stirrer"))||
                         (i==3&&n=="glass")||
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
        vec3 apparatusOffset=i==4?microscopeOffset:vec3(0);
        matrix(s,"model",T(stations[i]+apparatusOffset)*stationScale*m);
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
    // El pizarrón mate y el soporte completan el montaje del prisma.
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
        // Los haces usan mezcla aditiva y no reciben iluminación Phong.
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
        // Se recupera el shader principal antes de continuar con la escena.
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
// La cámara solo acepta posiciones dentro de los límites del salón.
bool validCameraPosition(vec3 position) {
    constexpr float left=-7.59f,right=7.59f;
    constexpr float back=-10.66f,front=8.64f;
    return position.x>=left&&position.x<=right&&position.z>=back&&position.z<=front;
}
glm::vec2 cameraAngles() {
    vec3 front=camera.GetFront();
    float yaw=glm::degrees(std::atan2(front.z,front.x));
    float pitch=glm::degrees(std::asin(glm::clamp(front.y,-1.f,1.f)));
    return {yaw,pitch};
}
void placeCamera(vec3 position,float yaw,float pitch) {
    position.y=WALK_HEIGHT;
    camera=Camera(position,vec3(0,1,0),yaw,pitch);
}
void moveCamera(Camera_Movement direction,float deltaTime) {
    vec3 forward=camera.GetFront();
    forward.y=0;
    if(glm::length(forward)<.001f)return;
    forward=glm::normalize(forward);
    vec3 right=glm::normalize(glm::cross(forward,vec3(0,1,0)));
    vec3 movement=(direction==FORWARD?forward:
                  direction==BACKWARD?-forward:
                  direction==LEFT?-right:right)*(6.f*deltaTime);
    vec3 proposed=camera.GetPosition()+movement;
    proposed.y=WALK_HEIGHT;
    if(validCameraPosition(proposed)) {
        glm::vec2 angles=cameraAngles();
        placeCamera(proposed,angles.x,angles.y);
    }
}
void animateCard(float deltaTime) {
    cardProgress=approach(cardProgress,card?1.f:0.f,deltaTime*1.65f);
}
void beginTravel(int station) {
    selected=station;
    card=false;
    travelStart=camera.GetPosition();
    travelStart.y=WALK_HEIGHT;
    travelEnd=stations[station]+vec3(0,WALK_HEIGHT-stations[station].y,3.2f);
    glm::vec2 angles=cameraAngles();
    travelStartYaw=angles.x;
    travelStartPitch=angles.y;
    travelProgress=0;
    travelDuration=glm::clamp(glm::distance(travelStart,travelEnd)/5.f,.55f,1.8f);
    cameraTraveling=true;
}
void animateTravel(float deltaTime) {
    if(!cameraTraveling)return;
    travelProgress=std::min(1.f,travelProgress+deltaTime/travelDuration);
    float t=travelProgress*travelProgress*(3.f-2.f*travelProgress);
    float yawDelta=std::fmod(-90.f-travelStartYaw+540.f,360.f)-180.f;
    placeCamera(glm::mix(travelStart,travelEnd,t),
                travelStartYaw+yawDelta*t,glm::mix(travelStartPitch,-10.f,t));
    if(travelProgress>=1)cameraTraveling=false;
}
void key(GLFWwindow*w,int k,int,int action,int) {
    if(action!=GLFW_PRESS)return;
    if(k==GLFW_KEY_ESCAPE) {
        if(card||cardProgress>.001f) {
            card=false;
        }
        else glfwSetWindowShouldClose(w,true);
    }
    if(k>=GLFW_KEY_1&&k<=GLFW_KEY_7)beginTravel(k-GLFW_KEY_1);
    if(k==GLFW_KEY_R)simulation.reset();
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
    if(selected==3&&k==GLFW_KEY_P)
        arquimedesSumergido=!arquimedesSumergido;
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
    if(k==GLFW_KEY_E&&!cameraTraveling) {
        // E cierra la ficha actual o busca la mesa más cercana frente a la cámara.
        if(card) {
            card=false;
            return;
        }
        vec3 forward=camera.GetFront();
        forward.y=0;
        forward=glm::normalize(forward);
        float bestScore=1e6f;
        int bestStation=-1;
        for(int i=0;i<7;++i) {
            vec3 toward=stations[i]-camera.GetPosition();
            toward.y=0;
            float distance=glm::length(toward);
            if(distance<.001f||distance>4.5f)continue;
            float facing=glm::dot(forward,toward/distance);
            if(facing<.25f)continue;
            float score=distance+(1.f-facing)*2.5f;
            if(score<bestScore) {
                bestScore=score;
                bestStation=i;
            }
        }
        if(bestStation>=0) {
            selected=bestStation;
            card=true;
        }
    }
}
void mouse(GLFWwindow*w,double x,double y) {
    static double px=x,py=y;
    if(glfwGetMouseButton(w,GLFW_MOUSE_BUTTON_RIGHT)==GLFW_PRESS&&
       cardProgress<.001f&&!cameraTraveling)
        camera.ProcessMouseMovement(x-px,py-y);
    px=x;
    py=y;
}
// Configura la iluminación general y las fuentes activas de cada experimento.
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
    // La lámpara nace debajo de la platina y apunta hacia la muestra.
    uniform(s,"spotLight.position",stations[4]+microscopeOffset+
            vec3(0,.43,.274f));
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
int main(int,char**argv) {
    try {
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
        glfwSwapInterval(1);
        glfwSetKeyCallback(w,key);
        glfwSetCursorPosCallback(w,mouse);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_MULTISAMPLE);
        Shader shader("shaders/lighting.vs","shaders/lighting.frag"),
               beamShader("shaders/beam.vs","shaders/beam.frag"),
               skyboxShader("shaders/skybox.vs","shaders/skybox.frag"),
               ui("shaders/ui.vs","shaders/ui.frag");
        for(GLuint p: {
            shader.Program,beamShader.Program,skyboxShader.Program,ui.Program
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
            if(cardProgress<.001f&&!cameraTraveling) {
                if(glfwGetKey(w,GLFW_KEY_W))moveCamera(FORWARD,dt);
                if(glfwGetKey(w,GLFW_KEY_S))moveCamera(BACKWARD,dt);
                if(glfwGetKey(w,GLFW_KEY_A))moveCamera(LEFT,dt);
                if(glfwGetKey(w,GLFW_KEY_D))moveCamera(RIGHT,dt);
            }
            accumulator+=dt;
            while(accumulator>=1./240) {
                simulation.step(1./240);
                accumulator-=1./240;
            }
            animateSpring(static_cast<float>(dt));
            animateStirrer(static_cast<float>(dt));
            animateArchimedes(static_cast<float>(dt));
            animateVanDeGraaff(static_cast<float>(dt));
            animatePrism(static_cast<float>(dt));
            animateMicroscope(static_cast<float>(dt));
            animateCard(static_cast<float>(dt));
            animateTravel(static_cast<float>(dt));
            int width,height;
            glfwGetFramebufferSize(w,&width,&height);
            if(height==0)continue;
            glViewport(0,0,width,height);
            glClearColor(.09,.15,.18,1);
            glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
            mat4 view=camera.GetViewMatrix();
            mat4 projection=glm::perspective(glm::radians(camera.GetZoom()),float(width)/height,.1f,80.f);
            drawSkybox(skyboxShader,view,projection);
            shader.Use();
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
            float hudWidth=std::min(float(width)*.96f,1500.f);
            float hudHeight=hudWidth*606.f/4512.f;
            alphaPicture(ui,hud,T(vec3(width*.5f,height-14.f-hudHeight*.5f,0))*
                    S(vec3(hudWidth,hudHeight,1)));
            if(cardProgress>.001f) {
                constexpr float cardAspect=2000.f/1200.f;
                float oldHeight=std::min(height*.78f,width*.7f*.72f);
                float ww=(oldHeight/.72f)*.85f;
                float h=ww/cardAspect;
                float t=cardProgress*cardProgress*(3.f-2.f*cardProgress);
                float scale=.55f+.45f*t;
                float centerY=glm::mix(height*.08f,height*.46f,t);
                float tilt=glm::radians(-3.f)*(1.f-t);
                alphaPicture(ui,cards[selected],
                    T(vec3(width*.5f,centerY,0))*R(tilt,vec3(0,0,1))*
                    S(vec3(ww*scale,h*scale,1)));
            }
            glEnable(GL_DEPTH_TEST);
            GLenum error=glGetError();
            if(error!=GL_NO_ERROR)throw std::runtime_error("Error OpenGL: "+std::to_string(error));
            glfwSwapBuffers(w);
            ++frame;
            std::string title=std::string("Laboratorio | ")+names[selected];
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
