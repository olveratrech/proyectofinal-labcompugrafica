# RenderLab: Tu laboratorio virtual de confianza

RenderLab es un laboratorio científico 3D desarrollado en C++ y OpenGL. La escena
reúne siete estaciones didácticas para demostrar carga de modelos, texturizado,
transparencias, iluminación, shaders, transformaciones jerárquicas, cuadros clave,
máquinas de estados y simulación numérica.

## Documentación

- [Documentación técnica y del proyecto](doc/esp/documentacion-esp.pdf)
- [Manual de usuario](doc/esp/manual-usuario.pdf)
- [Video demostrativo en YouTube](https://youtu.be/tM4SGBC2-oc)

## Estaciones

1. **Ley de Hooke:** masa suspendida y oscilación amortiguada del resorte.
2. **Cristalización:** preparación de cristales de sulfato de cobre pentahidratado.
3. **Péndulo doble:** movimiento calculado con Runge--Kutta de cuarto orden.
4. **Principio de Arquímedes:** inmersión de un cuerpo y desplazamiento de agua.
5. **Microscopio:** iluminación, cambio de objetivo y ajuste de enfoque.
6. **Van de Graaff:** bandas, rodillos y carrusel electrostático animados.
7. **Prisma de Newton:** entrada de luz blanca y dispersión cromática didáctica.

## Controles

| Acción | Control |
|---|---|
| Caminar | `W`, `A`, `S`, `D` |
| Mirar alrededor | Mantener presionado el botón derecho y mover el ratón |
| Viajar a una estación | `1` a `7` |
| Abrir o cerrar la ficha cercana | `E` |
| Reiniciar las simulaciones | `R` |
| Activar o detener la demostración de la estación seleccionada | `P` |
| Encender o apagar la luz del microscopio | `L` en la estación 5 |
| Girar el revólver del microscopio 120 grados | `O` en la estación 5 |
| Subir o bajar el enfoque del microscopio | `F` en la estación 5 |
| Cerrar una ficha o salir del programa | `Esc` |

## Compilación en macOS

Se requiere macOS con Homebrew y las dependencias GLFW, GLEW, GLM y Assimp
instaladas bajo `/opt/homebrew`.

```sh
brew install glfw glew glm assimp
sh tools/build_macos.sh
./build/laboratorio
```

También puede utilizarse CMake cuando las dependencias sean localizables:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/laboratorio
```

## Compilación en Linux

En Debian o Ubuntu se requieren CMake 3.20, un compilador con C++17 y los paquetes
de desarrollo correspondientes:

```sh
sudo apt update
sudo apt install build-essential cmake libglfw3-dev libglew-dev libglm-dev libassimp-dev zlib1g-dev
sh tools/build_linux.sh
./build-linux/laboratorio
```

El repositorio incluye un empaquetador Linux aportado mediante una rama separada:

```sh
sh tools/package_linux.sh
```

Genera `dist/Laboratorio-linux/` y `dist/Laboratorio-linux.tar.gz`. La carpeta
`dist/` no forma parte del repositorio.

## Windows

El paquete portable se publica en
[GitHub Releases](https://github.com/olveratrech/proyectofinal-labcompugrafica/releases/latest).
Descarga `RenderLab-windows-x64.zip`, extráelo por completo y ejecuta
`laboratorio.exe` sin separarlo de las DLL ni de las carpetas `assets` y `shaders`.
Se requiere Windows 10/11 de 64 bits y una GPU compatible con OpenGL 3.3.

Para compilarlo desde el código fuente se utilizan CMake, vcpkg y Visual Studio
Build Tools. Primero deben instalarse `glew:x64-windows`, `glfw3:x64-windows`,
`glm:x64-windows` y `assimp:x64-windows`. Después se configura `VCPKG_ROOT` y se
ejecutan los scripts desde PowerShell en la raíz del repositorio:

```powershell
$env:VCPKG_ROOT = "C:\vcpkg"
& .\tools\build_windows.ps1 -Generator "Visual Studio 18 2026"
& .\tools\package_windows.ps1 -Generator "Visual Studio 18 2026"
```

El empaquetado genera `dist/RenderLab-windows-x64/` y
`dist/RenderLab-windows-x64.zip` con el ejecutable, las DLL de terceros, recursos,
shaders, licencias y `LEEME.txt`. El generador `Visual Studio 18 2026` requiere
CMake 4.2 o posterior; con Visual Studio 2022 puede omitirse `-Generator`. Los
paquetes generados no se almacenan en Git: se distribuyen mediante Releases.

## Estructura del repositorio

```text
assets/blender/   modelos editables de Blender
assets/models/    piezas OBJ/MTL y manifiesto parts.txt
assets/textures/  texturas horneadas y recursos del entorno
assets/ui/        HUD, rótulos, fichas, carteles y señalización
doc/              documentación, manual e imagenes
shaders/          shaders de escena, interfaz, skybox y haces
src/base/         clases procedentes de las prácticas
src/              integración de la escena y simulación
tools/            scripts de compilación y empaquetado
```

`assets/models/parts.txt` registra la estación, el nombre, el padre y el pivote de
cada pieza independiente. Los cargadores `Model`, `Mesh`, `Shader`, `Camera` y
`stb_image` se conservan en `src/base/`; la integración del laboratorio está en
`src/main.cpp` y el péndulo doble en `src/Physics.h`.

## Alcance

RenderLab es una demostración didáctica, no un simulador científico de precisión.
Los líquidos no usan dinámica de fluidos, la dispersión del prisma es visual y las
colisiones se limitan a los muros del laboratorio. No se implementan sombras
dinámicas, trazado de rayos ni refracción físicamente exacta.

## Licencia y recursos

El código del proyecto se distribuye bajo los términos de [LICENSE](LICENSE). Los
créditos y licencias de recursos externos se documentan en la memoria técnica.
