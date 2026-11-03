# Validación de la implementación — 3 de noviembre de 2026

Comprobado en este Mac con Clang, OpenGL 3.3 core y Blender instalado:

- Compilación de la aplicación mediante `tools/build_macos.sh`, sin errores.
- Física: tres configuraciones, 60 segundos cada una a 240 pasos/s; valores finitos,
  nudo dentro del rango esperado y deriva de energía del péndulo menor a 0.001 J.
- Pausa y reinicio del estado físico.
- Siete estaciones, 29 piezas, 21,632 triángulos y mapas de color/AO presentes.
- Hashes de los seis archivos base idénticos a los de las prácticas.
- Shaders enlazados y 93 cuadros de integración sin errores OpenGL.
- Selección de una hoja por rayo, pausa, cambio de configuración, reinicio y muestra
  del microscopio comprobados dentro del modo `--smoke`.
- Inspección visual de capturas de las siete estaciones, una ficha y la fachada.
  Evidencias en `docs/previews`.
- Ejecutable empaquetado ejecutado desde `/tmp`, fuera de la raíz del proyecto.
  Sus dependencias GLEW/GLFW/Assimp se resuelven dentro de `dist/Laboratorio/lib`.

El modo de comprobación se ejecuta con `./build/laboratorio --smoke`; guarda
capturas PPM en `laboratorio-capturas` dentro de la carpeta temporal del sistema.

No se probó en un segundo equipo, Windows ni Linux. No se validó CMake en este
entorno, que no tiene CMake disponible. El paquete es para macOS y la arquitectura
del equipo donde se compiló; una firma ad hoc no equivale a notarización de Apple.
Las comprobaciones automatizadas no sustituyen la revisión académica del profesor.
