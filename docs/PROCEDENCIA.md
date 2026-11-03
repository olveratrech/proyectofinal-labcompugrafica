# Procedencia del código y recursos

La única fuente local de código reutilizado es `Practicas`, en la carpeta del
laboratorio indicada por el alumno. No se copió código de los otros proyectos
presentes en el equipo ni se descargaron modelos.

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

## Código nuevo escrito con asistencia de Codex

- `src/main.cpp`: geometría de la sala y mobiliario, composición de estaciones,
  jerarquías de animación, selección por rayo, fichas, controles y capturas.
- `src/Physics.h`: integración RK4 del péndulo doble y del sistema de poleas;
  respuesta amortiguada de la balanza y estado de carga ilustrativo.
- `shaders/ui.vs` y `shaders/ui.frag`: presentación de las fichas en pantalla.
- `tools/build_models.py`: modelado mediante la API de Blender, UV, horneado de
  color y AO, exportación OBJ por pieza, pivotes, colecciones y archivo fuente.
- `tools/build_ui.py`: diseño y rasterizado de fichas, rótulos y muestra ilustrativa.
- Scripts de compilación, empaquetado, verificación, CMake y pruebas de física.

Los modelos, mapas y fichas son recursos nuevos generados para este laboratorio.
El aparato de poleas toma como referencia visual la fotografía proporcionada por
el alumno; esa fotografía no se redistribuye. El benceno es una maqueta esquemática.

## Dependencias ya instaladas

Blender para generar recursos; GLFW, GLEW, GLM y Assimp para compilar. Pillow solo
para regenerar fichas. No se requieren Blender ni Python al ejecutar el programa.
El archivo fuente `.blend` contiene materiales procedurales y mapas empacados.
La geometría exportada usa los mapas de color con AO suavizada; no hay normal maps.
