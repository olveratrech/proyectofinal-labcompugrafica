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
    "Masa y resorte","Balanza","Pendulo doble","Benceno","Microscopio","Van de Graaff","Equilibrio de fuerzas"
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
std::unique_ptr<Mesh> cube,quad;
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
    double t=simulation.time;
    float angle=simulation.balance;
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
            const float displacement=.14f*std::cos(t*(2.8-.4*simulation.preset));
            if(n=="spring")m=T( {
                0,springTop,0
            }
            )*S( {
                1,1.0f+displacement/restLength,1
            }
            );
            if(n=="mass")m=T( {
                0,-displacement,0
            }
            );
        }
        if(i==1) {
            if(n=="beam")m=T(p.pivot)*R(angle);
            if(n=="left"||n=="right")m=world["1beam"]*T( {
                n=="left"?-.55f:.55f,0,0
            }
            )*R(-angle);
        }
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
        if(i==5&&n.rfind("strip",0)==0) {
            int k=std::stoi(n.substr(5));
            m=T( {
                0,1.76,0
            }
            )*R(k*6.2831853f/8, {
                0,1,0
            }
            )*R(.12f+1.25f*simulation.charge, {
                0,0,1
            }
            );
        }
        if(i==6) {
            vec3 q(simulation.force[0],simulation.force[1],.04);
            float l=glm::length(q-vec3(-.65,1.65,.04)),r=glm::length(q-vec3(.65,1.65,.04));
            if(n=="weightL")m=T( {
                -.81f,l-.35f,.04
            }
            );
            if(n=="weightR")m=T( {
                .81f,r-.35f,.04
            }
            );
            if(n=="weightC")m=T(q-vec3(0,.36,0));
            if(n=="pulleyL")m=T(p.pivot)*R(l/.16f);
            if(n=="pulleyR")m=T(p.pivot)*R(-r/.16f);
        }
        world[std::to_string(i)+n]=m;
        matrix(s,"model",T(stations[i])*m);
        p.model->Draw(s);
    }
    // La pesa adicional permite reconocer visualmente el lado más cargado.
    if(simulation.preset!=1) {
        auto side=simulation.preset==0?"1right":"1left";
        matrix(s,"model",T(stations[1])*world[side]*T( {
            .12,-.38,0
        }
        )*S( {
            .09,.13,.09
        }
        ));
        cube->surface.diffuse= {
            .72,.43,.12
        }
        ;
        cube->Draw(s);
    }
    vec3 o=stations[6],q(simulation.force[0],simulation.force[1],.04),a(-.65,1.65,.04),b(.65,1.65,.04);
    float l=glm::length(q-a),r=glm::length(q-b);
    for(auto pair:std::array<std::pair<vec3,vec3>,5> {
        {
            {
                a,q
            }
            , {
                b,q
            }
            , {
                {
                    -.81f,1.65,.04
                }
                , {
                    -.81f,l-.14f,.04
                }
            }
            , {
                {
                    .81f,1.65,.04
                }
                , {
                    .81f,r-.14f,.04
                }
            }
            , {
                q,q-vec3(0,.15,0)
            }
        }
    }
    )line(s,o+pair.first,o+pair.second,.012, {
        .12,.12,.1
    }
    );
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
    uniform(s,"viewPos",camera.GetPosition());
    uniform(s,"objectTint",vec3(1));
    uniform(s,"objectAlpha",1);
    uniform(s,"dirLight.direction", {
        -.3,-1,-.2
    }
    );
    uniform(s,"dirLight.ambient",vec3(.32));
    uniform(s,"dirLight.diffuse",vec3(.42));
    uniform(s,"dirLight.specular",vec3(.18));
    for(int i=0;i<4;++i) {
        auto n="pointLights["+std::to_string(i)+"].";
        uniform(s,n+"position", {
            i%2?-3.f:3.f,3.9f,i<2?1.f:-7.f
        }
        );
        uniform(s,n+"constant",1);
        uniform(s,n+"linear",.09);
        uniform(s,n+"quadratic",.032);
        uniform(s,n+"ambient",vec3(.015));
        uniform(s,n+"diffuse", {
            .65,.59,.46
        }
        );
        uniform(s,n+"specular",vec3(.5));
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
        Shader shader("shaders/lighting.vs","shaders/lighting.frag"),ui("shaders/ui.vs","shaders/ui.frag");
        for(GLuint p: {
            shader.Program,ui.Program
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
            animateMicroscope(static_cast<float>(dt));
            if(smoke&&frame%12==0&&frame/12<7)go(frame/12);
            int width,height;
            glfwGetFramebufferSize(w,&width,&height);
            if(height==0)continue;
            glViewport(0,0,width,height);
            glClearColor(.09,.15,.18,1);
            glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
            shader.Use();
            matrix(shader,"view",camera.GetViewMatrix());
            matrix(shader,"projection",glm::perspective(glm::radians(camera.GetZoom()),float(width)/height,.1f,80.f));
            lights(shader);
            room(shader);
            apparatus(shader);
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
