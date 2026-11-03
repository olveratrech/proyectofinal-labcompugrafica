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
ctest --test-dir build --output-on-failure
```

La ruta CMake se proporciona para portabilidad; la compilación comprobada es el
script de macOS. Windows/Linux requieren sus paquetes correspondientes y todavía
no se han probado. No existe un ejecutable Windows verificado.

## Controles

| Acción | Control |
|---|---|
| Recorrer | WASD |
| Mirar | Mantener botón derecho y mover ratón |
| Vista de estación | 1–7 |
| Consultar ficha | Clic en la hoja o E cerca de la mesa |
| Cerrar ficha | Clic o Esc |
| Iniciar / pausar demostraciones | Espacio |
| Restablecer simulaciones | R |
| Cambiar configuración | C |
| Muestra ilustrativa del microscopio | M con su ficha abierta |
| Salir | Esc fuera de una ficha |

Los controles de simulación son globales. Las tres configuraciones afectan a
resorte, balanza y pesas. La cámara es libre; no tiene colisiones.

## Estaciones

1. Masa–resorte: oscilación armónica y deformación del resorte.
2. Balanza: brazo articulado con platillos que conservan su orientación.
3. Péndulo doble: dos ángulos calculados mediante RK4.
4. Benceno: maqueta de Kekulé con carbonos, hidrógenos y enlaces.
5. Microscopio: cuerpo, platina, objetivos y ocular; muestra esquemática.
6. Van de Graaff: apertura gradual de tiras por una carga representada.
7. Poleas: nudo y pesas acopladas mediante cuerdas de longitud fija.

## Estructura

```text
assets/blender/    fuente editable con colecciones y padres
assets/models/     OBJ, MTL y manifiesto de piezas
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
No simula electricidad ni óptica real; el benceno es una maqueta, no química cuántica.
No incluye sombras dinámicas, normal mapping ni colisiones del recorrido.

Antes de entregar: validar las imágenes de referencia de fachada/interior con el
profesor, completar los datos personales y las conclusiones del manual, grabar el
video, preparar la exposición y probar en el equipo de evaluación. Los manuales
Markdown son una base de documentación, no una entrega académica final revisada.
