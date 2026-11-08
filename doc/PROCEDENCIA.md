# Procedencia del código y recursos

La única fuente local de código reutilizado es `Practicas`, en la carpeta del
laboratorio indicada por el alumno. No se copió código de los otros proyectos
presentes en el equipo. Los recursos externos del entorno se enumeran abajo.

## Archivos copiados sin modificaciones

De `Practica8/Practica1`: `Camera.h`, `Shader.h`, `Model.h`, `Mesh.h`,
`stb_image.h` y `stb_image.cpp`, conservados en `src/base/`.
De `Practica8/Shader`: `lighting.vs` y `lighting.frag`.
Los hashes del material de partida están en `base-sha256.txt`.
Estos archivos ya contienen modificaciones presentes en las prácticas del alumno;
no se afirma que cada línea sea del profesor.

## Integración adaptada

`src/main.cpp` retoma de `Practica8/Practica1/fuentesDeLuz.cpp` la inicialización
GLFW/GLEW, el bucle de dibujo, matrices de cámara, carga de modelos y configuración
de luces mediante uniforms. Usa directamente las clases copiadas, no otro motor.
La composición de transformaciones sigue el enfoque de `Practica4/Practica1/Modelado_Jerarquico.cpp`.
La apertura de fichas y el desplazamiento entre estaciones emplean estados e
interpolación entre dos cuadros clave, siguiendo la estructura general de
`Practica10/Practica1/KeyFrames.cpp`. La altura fija de la cámara y el cálculo
dinámico de sus puntos inicial y final son adaptaciones nuevas para este proyecto.
La estación de cristalización reutiliza de `Practica8/Practica1/fuentesDeLuz.cpp`
la activación de `GL_BLEND` y su función de mezcla. La varilla interpola cuadros
clave con la estructura mostrada en `Practica10/Practica1/KeyFrames.cpp`.
La estación del prisma se construye con primitivas OpenGL y reutiliza las cuatro
luces puntuales de la práctica 8; los haces y su secuencia didáctica son código nuevo.
El piso utiliza `GL_REPEAT`, igual que la configuración de texturas de la práctica
6 y de `Model.h`. La creación y carga del cubemap se apoya en las funciones y
conceptos de cubemap incluidos en SOIL2 dentro de las prácticas, pero el dibujo del
skybox y sus shaders son una ampliación nueva basada en la teoría del curso.

## Código nuevo escrito con asistencia de Codex

- `src/main.cpp`: geometría de la sala y mobiliario, composición de estaciones,
  jerarquías de animación, fichas activadas con teclado, controles, capturas,
  recorrido a altura fija y restricción de navegación mediante los límites del salón.
- `src/Physics.h`: integración RK4 del péndulo doble y del sistema de poleas;
  respuesta amortiguada y estado de carga ilustrativo.
- `shaders/ui.vs` y `shaders/ui.frag`: presentación de las fichas en pantalla.
- `shaders/skybox.vs` y `shaders/skybox.frag`: representación del cubemap exterior.
- `tools/build_models.py`: modelado mediante la API de Blender, UV, horneado de
  color y AO, exportación OBJ por pieza, pivotes, colecciones y archivo fuente.
- `tools/build_ui.py`: diseño y rasterizado de fichas, rótulos y muestra ilustrativa.
- Scripts de compilación, empaquetado, verificación, CMake y pruebas de física.

Los modelos, mapas y fichas son recursos nuevos generados para este laboratorio.
El aparato de poleas toma como referencia visual la fotografía proporcionada por
el alumno; esa fotografía no se redistribuye. La estación de Arquímedes es una maqueta didáctica propia.

## Recursos del entorno

- `assets/textures/skybox.exr` y `assets/textures/tile.jpeg`: archivos entregados
  por el alumno. El EXR panorámico se convirtió localmente en seis caras PNG para
  que OpenGL pueda cargarlo como cubemap. Debe conservarse la constancia de su
  fuente o licencia antes de publicar el proyecto.
- `assets/textures/environment/table_wood.jpg`: mapa de color
  **Wood Table 001**, Poly Haven, licencia CC0 1.0.
  <https://polyhaven.com/license>
- `assets/models/environment/casement-window.glb`: **Casement - Single Casement
  Window**, modelo original de Innerscene, licencia CC0 1.0. Para la ejecución se
  separó en marco, herrajes y vidrio, sin añadir geometría al recurso.
  <https://www.innerscene.com/tools/library/3d-parts/casement-single-casement-window-a85f86fc>

## Dependencias ya instaladas

Blender para generar recursos; GLFW, GLEW, GLM y Assimp para compilar. Pillow solo
para regenerar fichas. No se requieren Blender ni Python al ejecutar el programa.
El archivo fuente `.blend` contiene materiales procedurales y mapas empacados.
La geometría exportada usa los mapas de color con AO suavizada; no hay normal maps.

## Separación de modelos — 6 de noviembre de 2026

`tools/separate_models.py` es código nuevo: separa las colecciones originales, centra sus raíces, incorpora los mapas PNG existentes y verifica geometría y padres al reabrir cada archivo. No cambia las mallas ni los recursos de ejecución. Los archivos están en `assets/blender/independientes/`.
