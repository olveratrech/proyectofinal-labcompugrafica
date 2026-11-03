"""Modelos propios del laboratorio. Ejecutar con Blender en segundo plano.
Las coordenadas de las funciones son las de OpenGL: Y apunta hacia arriba.
Cada grupo se exporta alrededor de su pivote; la jerarquía viaja en parts.txt.
"""
import bpy, math, pathlib, json
from mathutils import Vector
ROOT=pathlib.Path(__file__).resolve().parents[1]
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=8
scene.render.bake.use_clear=False
materials={}
colors={'teal':(.025,.24,.27,1),'metal':(.53,.60,.64,1),'dark':(.035,.048,.06,1),'red':(.65,.065,.035,1),'white':(.8,.83,.79,1),'gold':(.72,.43,.12,1),'blue':(.04,.20,.56,1)}
for name,col in colors.items():
 m=bpy.data.materials.new(name);m.use_nodes=True
 bs=m.node_tree.nodes.get('Principled BSDF');bs.inputs['Base Color'].default_value=col
 bs.inputs['Metallic'].default_value=.7 if name in ['metal','gold'] else .1
 bs.inputs['Roughness'].default_value=.28 if name=='metal' else .45
 # Un grano suave evita superficies completamente planas al hornear el color.
 noise=m.node_tree.nodes.new('ShaderNodeTexNoise');noise.inputs['Scale'].default_value=55
 ramp=m.node_tree.nodes.new('ShaderNodeValToRGB')
 ramp.color_ramp.elements[0].color=tuple(v*.85 for v in col[:3])+(1,)
 ramp.color_ramp.elements[1].color=col
 m.node_tree.links.new(noise.outputs['Fac'],ramp.inputs[0]);m.node_tree.links.new(ramp.outputs[0],bs.inputs['Base Color'])
 materials[name]=m

def cv(p):return Vector((p[0],-p[2],p[1]))
parts=[]; current=[]; station=0

def finish_obj(o,name,mat):
 o.name=name;o.data.materials.append(materials[mat]);current.append(o)
 return o

def box(p,s,mat='teal'):
 bpy.ops.mesh.primitive_cube_add(size=1,location=cv(p));o=bpy.context.object;o.scale=(s[0],s[2],s[1]);bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 mod=o.modifiers.new('Bordes suaves','BEVEL');mod.width=.018;mod.segments=2
 bpy.ops.object.modifier_apply(modifier=mod.name)
 return finish_obj(o,'cuerpo',mat)

def sphere(p,r,mat='metal'):
 bpy.ops.mesh.primitive_uv_sphere_add(segments=20,ring_count=12,radius=r,location=cv(p));o=bpy.context.object
 for poly in o.data.polygons:poly.use_smooth=True
 return finish_obj(o,'esfera',mat)

def rod(a,b,r,mat='metal',vertices=20):
 d=cv(b)-cv(a);bpy.ops.mesh.primitive_cylinder_add(vertices=vertices,radius=r,depth=d.length,location=(cv(a)+cv(b))*.5)
 o=bpy.context.object;o.rotation_mode='QUATERNION';o.rotation_quaternion=d.to_track_quat('Z','Y')
 for poly in o.data.polygons:poly.use_smooth=len(poly.vertices)==4
 return finish_obj(o,'cilindro',mat)

def part(name,pivot=(0,0,0),parent='root'):
 global current
 bpy.ops.object.select_all(action='DESELECT')
 for o in current:o.select_set(True)
 bpy.context.view_layer.objects.active=current[0];bpy.ops.object.join();o=bpy.context.object
 o.name=f'{station}_{name}';scene.cursor.location=cv(pivot);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
 parts.append((station,name,parent,pivot,o));current=[]

def base():
 box((0,.055,0),(1.45,.11,.8));
 for x in [-.56,.56]:
  for z in [-.27,.27]:rod((x,0,z),(x,.035,z),.065,'dark')

def stand(height=1.7):
 base();rod((-.5,.1,0),(-.5,height,0),.035);rod((-.5,height,0),(.25,height,0),.035)
# 0: resorte y masa, piezas independientes.
station=0;stand();part('base')
# El resorte tiene longitud unitaria y se escala desde su punto superior.
verts=[];faces=[]
for i in range(241):
 angle=i*.22;center=Vector((.09*math.cos(angle),-i/240,.09*math.sin(angle)))
 tangent=Vector((-.09*.22*math.sin(angle),-1/240,.09*.22*math.cos(angle))).normalized()
 outward=Vector((math.cos(angle),0,math.sin(angle)));side=tangent.cross(outward).normalized()
 for j in range(8):verts.append(cv(center+.012*(math.cos(j*math.tau/8)*outward+math.sin(j*math.tau/8)*side)))
for i in range(240):
 for j in range(8):a=i*8+j;b=i*8+(j+1)%8;faces.append((a,b,b+8,a+8))
mesh=bpy.data.meshes.new('Espiral continua');mesh.from_pydata(verts,[],faces);mesh.update()
o=bpy.data.objects.new('Resorte',mesh);scene.collection.objects.link(o)
for face in mesh.polygons:face.use_smooth=True
finish_obj(o,'resorte','metal')
part('spring')
rod((0,-.17,0),(0,.03,0),.16,'red');rod((0,.03,0),(0,.09,0),.035);part('mass')
# 1: balanza de dos platillos.
station=1;base();rod((0,.1,0),(0,1.1,0),.075);box((0,.9,0),(.22,.25,.18),'white');part('base')
rod((-.58,1.12,0),(.58,1.12,0),.035,'gold');sphere((0,1.12,0),.085,'metal');part('beam',(0,1.12,0))
for side,x in [('left',-.55),('right',.55)]:
 for z in [-.14,.14]:rod((x,1.12,0),(x,.67,z),.012)
 rod((x,.62,0),(x,.67,0),.23,'metal');rod((x,.67,0),(x,.83,0),.085,'gold');part(side,(x,1.12,0),'beam')
# 2: pendulo doble; brazos locales dirigidos hacia abajo.
station=2;base();rod((0,.1,-.18),(0,1.9,-.18),.045);rod((0,1.9,-.18),(0,1.9,0),.045);part('base')
for name,col in [('arm1','gold'),('arm2','red')]:
 box((0,-.32,0),(.055,.64,.055),col);sphere((0,-.65,0),.095,'metal');sphere((0,0,0),.055,'dark');part(name,(0,0,0),'root' if name=='arm1' else 'arm1')
# 3: benceno, maqueta con enlaces alternados (representacion de Kekule).
station=3;base();rod((0,.1,0),(0,.55,0),.025);part('base')
C=[(.48*math.cos(i*math.pi/3),1.08+.48*math.sin(i*math.pi/3),0) for i in range(6)]
for i,a in enumerate(C):
 sphere(a,.105,'dark');h=(a[0]*1.55,1.08+(a[1]-1.08)*1.55,0);sphere(h,.07,'white');rod(a,h,.027,'metal');b=C[(i+1)%6];rod(a,b,.035,'metal')
 if i%2==0:
  aa=(a[0]*.84,1.08+(a[1]-1.08)*.84,.025);bb=(b[0]*.84,1.08+(b[1]-1.08)*.84,.025);rod(aa,bb,.02,'gold')
part('molecule')
# 4: microscopio compuesto con platina y revolver.
station=4;base();box((0,.18,0),(.65,.18,.48),'white');rod((.22,.25,0),(.32,1.08,0),.09,'white');rod((.32,1.08,0),(0,1.4,0),.09,'white')
box((0,.65,.1),(.55,.055,.42),'dark');box((0,.686,.1),(.22,.012,.1),'blue');rod((0,.3,.1),(0,.48,.1),.08,'metal');rod((.3,.68,-.13),(.3,.68,.13),.09,'dark');part('base')
rod((0,1.1,0),(-.16,1.5,0),.085,'white');rod((-.16,1.5,0),(-.2,1.6,0),.065,'dark');rod((0,1.07,0),(0,1.13,0),.14,'metal')
for x in [-.075,0,.075]:rod((x,1.07,0),(x,.89,.04),.028,'metal')
part('optics')
# 5: Van de Graaff, esfera metalica y tiras para una demostracion cualitativa.
station=5;base();box((0,.2,0),(.65,.25,.48),'white');rod((0,.3,0),(0,1.24,0),.105,'teal');sphere((0,1.45,0),.33);rod((.48,.1,0),(.48,.8,0),.025,'dark');sphere((.48,.84,0),.08);part('base')
for i in range(8):
 box((0,.17,0),(.024,.34,.006),'gold');part(f'strip{i}')
# 6: aparato de fuerzas inspirado en la imagen proporcionada.
station=6;base()
for x in [-.65,.65]:rod((x,.1,0),(x,1.85,0),.03);box((x,1.65,0),(.12,.14,.07),'dark')
part('base')
for name,x in [('pulleyL',-.65),('pulleyR',.65)]:
 rod((x,1.65,-.025),(x,1.65,.025),.16,'red');rod((x,1.65,-.04),(x,1.65,.04),.04,'metal');part(name,(x,1.65,0))
for name in ['weightL','weightR','weightC']:
 for y in [-.09,0,.09]:rod((0,y-.035,0),(0,y+.035,0),.075,'dark')
 rod((0,.12,0),(0,.21,0),.015);part(name)
# Bake por estacion: UV compartidas entre todas sus piezas, color y oclusion separados.
manifest=[];stats=[]
for s in range(7):
 entries=[p for p in parts if p[0]==s];objs=[p[4] for p in entries]
 for item in parts:item[4].hide_render=item[0]!=s
 bpy.ops.object.select_all(action='DESELECT')
 for o in objs:o.select_set(True)
 bpy.context.view_layer.objects.active=objs[0]
 bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project(island_margin=.025);bpy.ops.object.mode_set(mode='OBJECT')
 image=bpy.data.images.new(f'{s}_albedo',width=512,height=512,alpha=False)
 for m in materials.values():
  node=m.node_tree.nodes.get('Bake') or m.node_tree.nodes.new('ShaderNodeTexImage');node.name='Bake';node.image=image;m.node_tree.nodes.active=node
 bpy.ops.object.bake(type='DIFFUSE',pass_filter={'COLOR'},margin=8)
 image.filepath_raw=str(ROOT/f'assets/textures/{s}_albedo.png');image.file_format='PNG';image.save()
 ao=bpy.data.images.new(f'{s}_ao',width=512,height=512,alpha=False)
 for m in materials.values():m.node_tree.nodes['Bake'].image=ao
 bpy.ops.object.bake(type='AO',margin=8)
 ao.filepath_raw=str(ROOT/f'assets/textures/{s}_ao.png');ao.file_format='PNG';ao.save()
 # El mapa final conserva el color y oscurece suavemente los encuentros de piezas.
 color=list(image.pixels[:]);occ=ao.pixels[:]
 for i in range(0,len(color),4):
  factor=.72+.28*occ[i]
  for j in range(3):color[i+j]*=factor
 image.pixels[:]=color;image.save()
 for _,name,parent,pivot,o in entries:
  mesh=o.data;mesh.calc_loop_triangles();uv=mesh.uv_layers.active.data
  path=ROOT/f'assets/models/{s}_{name}.obj';mtl=path.with_suffix('.mtl')
  mtl.write_text(f'newmtl baked\nKd 1 1 1\nKs 0.35 0.35 0.35\nNs 48\nmap_Kd ../textures/{s}_albedo.png\n')
  lines=[f'mtllib {mtl.name}','usemtl baked'];n=0
  # Triangulos independientes: sencilla exportacion que conserva normales y UV.
  normalmat=o.matrix_world.to_3x3().inverted().transposed()
  for tri in mesh.loop_triangles:
   for li in tri.loops:
    v=mesh.vertices[mesh.loops[li].vertex_index];p=o.matrix_world@v.co-cv(pivot);normal=normalmat@(v.normal if mesh.polygons[tri.polygon_index].use_smooth else tri.normal);normal.normalize();t=uv[li].uv
    lines += [f'v {p.x:.6f} {p.z:.6f} {-p.y:.6f}',f'vt {t.x:.6f} {t.y:.6f}',f'vn {normal.x:.6f} {normal.z:.6f} {-normal.y:.6f}']
   lines.append('f '+' '.join(f'{k}/{k}/{k}' for k in range(n+1,n+4)));n+=3
  path.write_text('\n'.join(lines)+'\n');manifest.append(f'{s} {name} {parent} {pivot[0]} {pivot[1]} {pivot[2]}')
  stats.append({'station':s,'part':name,'triangles':n//3})
# Colecciones y padres editables en el archivo fuente.
for s in range(7):
 coll=bpy.data.collections.new(f'{s+1:02d}_estacion');scene.collection.children.link(coll)
 entries=[p for p in parts if p[0]==s];lookup={p[1]:p[4] for p in entries}
 for _,name,parent,pivot,o in entries:
  for old in list(o.users_collection):old.objects.unlink(o)
  coll.objects.link(o)
  if parent in lookup:
   matrix=o.matrix_world.copy();o.parent=lookup[parent];o.matrix_world=matrix
# Las estaciones se muestran separadas en Blender sin alterar los OBJ locales.
for s in range(7):
 coll=bpy.data.collections[f'{s+1:02d}_estacion']
 root=bpy.data.objects.new(f'{s+1:02d}_raiz',None);coll.objects.link(root)
 entries=[p for p in parts if p[0]==s];lookup={p[1]:p[4] for p in entries}
 for _,name,parent,pivot,o in entries:
  o.hide_render=False
  if o.parent is None:
   matrix=o.matrix_world.copy();o.parent=root;o.matrix_world=matrix
 if s==0:
  lookup['spring'].location=cv((0,1.7,0));lookup['spring'].scale.z=.85
  lookup['mass'].location=cv((0,.85,0))
 if s==2:
  lookup['arm1'].location=cv((0,1.9,0));lookup['arm2'].location=cv((0,-.65,0))
 if s==5:
  for k in range(8):lookup[f'strip{k}'].location=cv((0,1.76,0))
 if s==6:
  for name,p in [('weightL',(-.81,.55,.04)),('weightR',(.81,.55,.04)),('weightC',(-.12,.69,.04))]:lookup[name].location=cv(p)
 root.location=cv(((s%4)*2.5,0,(s//4)*3))
# Empacar los mapas mantiene el archivo fuente autocontenido.
for im in bpy.data.images:
 if im.source=='FILE' or im.name.endswith(('_albedo','_ao')):
  try:im.pack()
  except RuntimeError:pass
(ROOT/'assets/models/parts.txt').write_text('\n'.join(manifest)+'\n')
(ROOT/'docs/model-stats.json').write_text(json.dumps(stats,indent=2))
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'assets/blender/laboratorio.blend'))
print('MODELOS_COMPLETOS',sum(x['triangles'] for x in stats))
