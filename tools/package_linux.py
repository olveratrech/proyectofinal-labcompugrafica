"""Crea un paquete Linux transportable con el ejecutable, recursos y bibliotecas."""
from pathlib import Path
import os
import shutil
import subprocess
import tarfile

ROOT = Path(__file__).resolve().parents[1]
BUILD = Path(os.environ.get("BUILD_DIR", "build-linux"))
if not BUILD.is_absolute():
    BUILD = ROOT / BUILD
BUILD = BUILD.resolve()
OUT = ROOT / "dist" / "Laboratorio-linux"
ARCHIVE = ROOT / "dist" / "Laboratorio-linux.tar.gz"
# Bibliotecas que debe aportar el sistema; nunca se copian al paquete.
SYSTEM_LIBRARIES = {
    "libc.so.6", "libm.so.6", "libdl.so.2", "libpthread.so.0", "librt.so.1",
    "libresolv.so.2", "libutil.so.1", "libanl.so.1", "libnsl.so.1",
    "libmvec.so.1", "libgcc_s.so.1", "libstdc++.so.6", "libatomic.so.1",
    "libGL.so.1", "libGLX.so.0", "libOpenGL.so.0", "libGLdispatch.so.0",
    "libEGL.so.1", "libGLESv2.so.2",
    "libX11.so.6", "libXext.so.6", "libXrandr.so.2", "libXinerama.so.1",
    "libXcursor.so.1", "libXi.so.6", "libXxf86vm.so.1", "libXfixes.so.3",
    "libXrender.so.1", "libxcb.so.1", "libXau.so.6", "libXdmcp.so.6",
    "libbsd.so.0", "libmd.so.0",
    "libxkbcommon.so.0", "libxkbcommon-x11.so.0",
    "libwayland-client.so.0", "libwayland-cursor.so.0",
    "libwayland-egl.so.1", "libwayland-server.so.0",
    "libdrm.so.2", "libgbm.so.1", "libudev.so.1",
}
LICENSE_NAMES = {
    "libGLEW": ("glew", "libglew-dev", "libglew2.2", "libglew2.1"),
    "libglfw": ("glfw", "libglfw3-dev", "libglfw3"),
    "libassimp": ("assimp", "libassimp-dev", "libassimp5", "libassimp4"),
    "libdraco": ("draco", "libdraco-dev", "libdraco4"),
    "libminizip": ("minizip", "libminizip-dev", "libminizip1"),
    "libz": ("zlib", "zlib1g-dev", "zlib1g"),
}


def run(command, **kwargs):
    return subprocess.check_output(command, text=True, **kwargs)


def dependencies(binary, library_path=None):
    env = os.environ.copy()
    if library_path:
        current = env.get("LD_LIBRARY_PATH", "")
        env["LD_LIBRARY_PATH"] = str(library_path) + (":" + current if current else "")
    result = {}
    for line in run(["ldd", str(binary)], env=env).splitlines():
        line = line.strip()
        if "=>" not in line:
            continue
        name, rest = line.split("=>", 1)
        name = name.strip()
        path = rest.strip().split(" (", 1)[0].strip()
        result[name] = path
    return result


def check_resolved(binary, library_path=None):
    missing = [name for name, path in dependencies(binary, library_path).items()
               if path == "not found"]
    if missing:
        raise SystemExit(
            "Faltan bibliotecas para ejecutar el paquete: " + ", ".join(missing) +
            "\nSi están en una ruta no estándar, exporta LD_LIBRARY_PATH antes de empaquetar."
        )


def bundle_libraries(binary):
    """Copia el ejecutable y sus dependencias no aportadas por el sistema."""
    queue = [binary]
    bundled = {}
    while queue:
        current = queue.pop()
        for name, path in dependencies(current, OUT / "lib").items():
            if name.startswith("ld-linux") or name in SYSTEM_LIBRARIES:
                continue
            if path == "not found":
                raise SystemExit(f"No se pudo localizar {name}, necesario para {current.name}.")
            bundled[name] = path
            target = OUT / "lib" / name
            if target.exists():
                continue
            shutil.copy2(path, target)
            target.chmod(0o755)
            queue.append(target)
    return bundled


def copy_licenses(bundled):
    licenses = OUT / "licenses"
    licenses.mkdir()
    for library, path in bundled.items():
        base = library.split(".so", 1)[0]
        names = LICENSE_NAMES.get(base, (base[3:], f"{base[3:]}-dev"))
        found = []
        prefixes = list(Path(path).resolve().parents)
        prefixes.append(Path("/usr"))
        for prefix in prefixes:
            for name in names:
                for directory in (prefix / "share" / "licenses" / name,
                                  prefix / "share" / "doc" / name):
                    if directory.is_dir():
                        found.extend(item for item in directory.iterdir()
                                     if item.is_file())
        if not found:
            print(f"Aviso: no se encontró la licencia de {library}.")
        for source in dict.fromkeys(found):
            target = licenses / f"{base[3:].lower()}-{source.name}"
            shutil.copy2(source, target)
    shutil.copy2(ROOT / "LICENSE", licenses / "proyecto.txt")


def set_library_rpath():
    """Hace que cada biblioteca encuentre sus dependencias en lib/."""
    patchelf = shutil.which("patchelf")
    if not patchelf:
        print("Aviso: patchelf no está instalado; usa Laboratorio.sh para cargar lib/.")
        return
    for library in (OUT / "lib").iterdir():
        subprocess.run([patchelf, "--set-rpath", "$ORIGIN", str(library)],
                       check=True)


def write_launcher():
    launcher = OUT / "Laboratorio.sh"
    launcher.write_text(
        '#!/bin/sh\n'
        'cd "$(dirname "$0")"\n'
        'LD_LIBRARY_PATH="$(pwd)/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"\n'
        'export LD_LIBRARY_PATH\n'
        '# GLEW usa GLX; si hay una sesión Wayland, se fuerza X11/XWayland.\n'
        'if [ -n "${WAYLAND_DISPLAY:-}" ] && [ -n "${DISPLAY:-}" ]; then\n'
        '    unset WAYLAND_DISPLAY\n'
        'fi\n'
        'exec ./laboratorio "$@"\n'
    )
    launcher.chmod(0o755)


def write_readme():
    (OUT / "LEEME.txt").write_text(
        "Laboratorio de materia, fuerzas y movimiento\n"
        "============================================\n\n"
        "Ejecutar:\n"
        "    ./Laboratorio.sh\n\n"
        "El lanzador configura lib/ y, en Wayland, usa X11/XWayland.\n\n"
        "Requisitos:\n"
        "- Linux x86-64 con OpenGL 3.3.\n"
        "- GLEW, GLFW y Assimp se incluyen en lib/.\n"
        "- X11/Wayland, el controlador OpenGL y glibc los proporciona el sistema.\n"
        "- En Wayland, Laboratorio.sh fuerza X11/XWayland porque GLEW usa GLX.\n\n"
        "Controles:\n"
        "    WASD        caminar\n"
        "    Ratón der.  mirar (mantener pulsado)\n"
        "    1-7         ir a una estación\n"
        "    E           abrir/cerrar ficha cerca de la mesa\n"
        "    Espacio     iniciar/pausar demostraciones\n"
        "    R           restablecer simulaciones\n"
        "    C           cambiar configuración\n"
        "    P           acción de la estación (resorte, varilla, péndulo, Van de Graaff, prisma)\n"
        "    L / O / F   encender, cambiar objetivo y enfocar el microscopio\n"
        "    M           muestra del microscopio con su ficha abierta\n"
        "    Esc         salir\n"
    )


def main():
    binary = BUILD / "laboratorio"
    if not binary.is_file():
        raise SystemExit(f"No existe {binary}. Ejecuta antes tools/build_linux.sh.")

    if OUT.exists():
        shutil.rmtree(OUT)
    OUT.mkdir(parents=True)
    (OUT / "lib").mkdir()

    shutil.copy2(binary, OUT / "laboratorio")
    (OUT / "laboratorio").chmod(0o755)
    for name in ("assets", "shaders"):
        shutil.copytree(ROOT / name, OUT / name)

    check_resolved(binary)
    bundled = bundle_libraries(binary)
    set_library_rpath()
    copy_licenses(bundled)
    write_launcher()
    write_readme()

    packaged = OUT / "laboratorio"
    check_resolved(packaged, OUT / "lib")

    if ARCHIVE.exists():
        ARCHIVE.unlink()
    with tarfile.open(ARCHIVE, "w:gz") as archive:
        archive.add(OUT, arcname=OUT.name)

    print("Carpeta:", OUT)
    print("Archivo:", ARCHIVE)


if __name__ == "__main__":
    main()
