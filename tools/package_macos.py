"""Copia el ejecutable y sus bibliotecas de Homebrew a una carpeta independiente."""
from pathlib import Path
import shutil,subprocess
root=Path(__file__).resolve().parents[1];out=root/'dist/Laboratorio'
out.mkdir(parents=True,exist_ok=True);(out/'lib').mkdir(exist_ok=True)
shutil.copy2(root/'build/laboratorio',out/'laboratorio')
for name in ['assets','shaders']:shutil.copytree(root/name,out/name,dirs_exist_ok=True)
licenses=out/'licenses';licenses.mkdir(exist_ok=True)
for package in ['glew','glfw','assimp']:
 base=Path('/opt/homebrew/opt')/package
 for source in base.glob('LICENSE*'):shutil.copy2(source,licenses/(package+'-'+source.name))
shutil.copy2(root/'LICENSE',licenses/'proyecto.txt')
queue=[out/'laboratorio'];done=set()
while queue:
 binary=queue.pop()
 if binary in done:continue
 done.add(binary)
 deps=subprocess.check_output(['otool','-L',str(binary)],text=True).splitlines()[1:]
 for entry in deps:
  dep=entry.strip().split(' (')[0]
  if not dep.startswith('/opt/homebrew/'):continue
  target=out/'lib'/Path(dep).name
  if not target.exists():shutil.copy2(dep,target);queue.append(target)
  replacement=('@loader_path/lib/' if binary.parent==out else '@loader_path/')+target.name
  subprocess.run(['install_name_tool','-change',dep,replacement,str(binary)],check=True)
 if binary.suffix=='.dylib':subprocess.run(['install_name_tool','-id','@loader_path/'+binary.name,str(binary)],check=True)
 subprocess.run(['codesign','--force','--sign','-',str(binary)],check=True)
(out/'Abrir.command').write_text('#!/bin/sh\ncd "$(dirname "$0")"\nexec ./laboratorio\n')
(out/'Abrir.command').chmod(0o755)
print('Paquete:',out)
