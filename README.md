# Laboratorio de materia, fuerzas y movimiento

Recreación 3D en OpenGL de un laboratorio didáctico con siete estaciones. La
arquitectura y las mesas se construyen por código. Los aparatos se modelan en
Blender, se hornean y se exportan como piezas OBJ con pivotes independientes.

**Base académica:** práctica 8 (cámara, shaders, modelos, mallas e iluminación).
Consulta [la procedencia](docs/PROCEDENCIA.md) para distinguir la base de las
ampliaciones nuevas. No todo el código proviene de las prácticas.

## Ejecutar en este Mac

Desde la raíz del repositorio:

```sh
sh tools/build_macos.sh
./build/laboratorio
```

Requiere las instalaciones de GLFW, GLEW, GLM y Assimp disponibles en
`/opt/homebrew`. El script usa Clang y ejecuta una prueba de física. Para preparar
una carpeta transportable en un Mac compatible, usa `sh tools/package_macos.sh`.

Alternativa CMake (requiere CMake y las dependencias localizables):

```sh
cmake -S . -B build
cmake --build build --config Release
```

La ruta CMake se proporciona para portabilidad; la compilación comprobada es el
script de macOS. En Linux, usa los scripts de la sección siguiente. Windows
requiere sus paquetes correspondientes y todavía no se ha probado. No existe un
ejecutable Windows verificado.

## Ejecutar y empaquetar en Linux

Requiere CMake 3.20 o posterior, un compilador C++17 y los paquetes de desarrollo
de GLFW, GLEW, GLM y Assimp. En Debian/Ubuntu:

```sh
sudo apt install build-essential cmake libglfw3-dev libglew-dev libglm-dev libassimp-dev zlib1g-dev
```

Desde la raíz del repositorio:

```sh
sh tools/build_linux.sh
./build-linux/laboratorio
```

Para generar una carpeta transportable y un archivo `.tar.gz` con las
bibliotecas GLEW, GLFW y Assimp incluidas:

```sh
sh tools/package_linux.sh
```

El resultado queda en `dist/Laboratorio-linux/` y `dist/Laboratorio-linux.tar.gz`.
El lanzador `Laboratorio.sh` fuerza X11/XWayland cuando la sesión es Wayland,
porque GLEW utiliza GLX.

## Controles

| Acción | Control |
|---|---|
| Caminar a altura fija | WASD |
| Mirar | Mantener botón derecho y mover ratón |
| Desplazarse a una estación | 1–7 |
| Consultar / guardar ficha | E cerca de la mesa |
| Cerrar ficha | E o Esc |
| Iniciar / pausar demostraciones | Espacio |
| Restablecer simulaciones | R |
| Cambiar configuración | C |
| Estirar / detener masa–resorte | P en la estación 1 |
| Agitar / detener varilla de vidrio | P en la estación 2 |
| Pausar / reanudar péndulo doble | P en la estación 3 |
| Encender / detener Van de Graaff | P en la estación 6 |
| Iniciar / apagar prisma de Newton | P en la estación 7 |
| Encender / apagar microscopio | L en la estación 5 |
| Cambiar objetivo 120° | O en la estación 5 |
| Subir / bajar enfoque | F en la estación 5 |
| Muestra ilustrativa del microscopio | M con su ficha abierta |
| Salir | Esc fuera de una ficha |

Los controles de simulación son globales. La cámara conserva una altura de caminata
y los límites del salón impiden atravesar sus muros.

## Estaciones

1. Masa–resorte: oscilación armónica y deformación del resorte.
2. Cristalización: preparación de sulfato de cobre pentahidratado con vidrio y líquidos transparentes.
3. Péndulo doble: dos ángulos calculados mediante RK4.
4. Principio de Arquímedes: dinamómetro, cuerpo suspendido y recipiente transparente.
5. Microscopio: cuerpo, platina, objetivos y ocular; muestra esquemática.
6. Van de Graaff: banda visible, rodillos, toma de tierra y carrusel electrostático animados.
7. Prisma de Newton: haz blanco y espectro progresivo proyectado sobre un pizarrón mate.

## Estructura

```text
assets/blender/    fuente editable con colecciones y padres
assets/models/     OBJ y MTL separados por aparato; parts.txt conserva la jerarquía
assets/textures/   color horneado y AO por estación
assets/ui/         fichas y rótulos
src/base/          código conservado de las prácticas
src/               integración y simulaciones nuevas
shaders/           iluminación de práctica 8 e interfaz nueva
tools/             generación, compilación y empaquetado
tests/             comprobaciones de dinámica
docs/              procedencia, manuales y datos de modelos
```

Para regenerar los aparatos:

```sh
/Applications/Blender.app/Contents/MacOS/Blender -b -t 4 --python tools/build_models.py
python3 tools/build_ui.py
```

La generación reemplaza los recursos generados; guarda antes cualquier edición
manual del `.blend` con otro nombre. `build_ui.py` requiere Pillow y usa Arial de
macOS. Las fichas incluidas ya están listas y no necesitan regenerarse.

## Alcance y entrega

Es una implementación inicial funcional, con siete modelos y cinco demostraciones
animadas. La clasificación de las animaciones complejas debe validarla el profesor.
No resuelve una simulación completa de fluidos: la inmersión muestra de forma didáctica el empuje, el cambio de nivel y la variación de peso aparente.
No incluye sombras dinámicas, normal mapping ni colisiones contra el mobiliario.

Antes de entregar: validar las imágenes de referencia de fachada/interior con el
profesor, completar los datos personales y las conclusiones del manual, grabar el
video, preparar la exposición y probar en el equipo de evaluación. Los manuales
Markdown son una base de documentación, no una entrega académica final revisada.

## Modelado individual

Los siete archivos editables están en [assets/blender/independientes](assets/blender/independientes/README.md). Conservan piezas y pivotes del original. La guía describe el orden de mejora y cómo evitar sobrescribir retoques manuales.
