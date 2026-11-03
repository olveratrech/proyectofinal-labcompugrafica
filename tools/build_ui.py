"""Fichas propias, rasterizadas como texturas para no añadir una biblioteca de interfaz."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import textwrap
R=Path(__file__).resolve().parents[1]/'assets/ui'
font='/System/Library/Fonts/Supplemental/Arial.ttf'
def f(n):return ImageFont.truetype(font,n)
entries=[('MASA Y RESORTE','Mecánica · Oscilaciones','Una masa suspendida estira el resorte. Al separarla de su posición de equilibrio aparece una fuerza restauradora: F = −kx.','Observa cómo el resorte cambia de longitud con la masa. Las tres configuraciones cambian la frecuencia de la oscilación.','Modelo ideal sin rozamiento. La amplitud permanece constante.'),('BALANZA DE LABORATORIO','Mecánica · Equilibrio','Una balanza compara masas mediante el momento que ejercen sobre un brazo. Con brazos iguales, masas iguales producen equilibrio.','Cambia la configuración para inclinar el brazo a la izquierda, equilibrarlo o inclinarlo a la derecha.','Transición amortiguada ilustrativa; no es un instrumento de medición.'),('PÉNDULO DOBLE','Dinámica · Movimiento acoplado','Dos brazos unidos intercambian energía. Su movimiento puede ser muy sensible a las condiciones iniciales.','Sigue la segunda esfera. Su trayectoria surge de resolver las ecuaciones de movimiento, no de una animación grabada.','Brazos de 0.65 m, masas iguales y articulaciones ideales.'),('BENCENO · C₆H₆','Química · Estructura molecular','Seis carbonos forman un anillo plano unido a seis hidrógenos. La maqueta usa una representación de Kekulé.','Las esferas oscuras representan carbono y las claras hidrógeno. Los enlaces alternados son una convención de representación.','Los electrones están deslocalizados: los enlaces reales del anillo son equivalentes.'),('MICROSCOPIO COMPUESTO','Óptica · Instrumentación','El objetivo y el ocular permiten observar estructuras pequeñas. La platina sostiene la muestra y los mandos permiten enfocar.','Identifica el ocular, el revólver de objetivos, la platina y el sistema de iluminación. Pulsa M para ver una muestra esquemática.','La imagen de muestra es una ilustración educativa, no una microfotografía.'),('GENERADOR DE VAN DE GRAAFF','Electrostática · Carga eléctrica','Una banda transporta carga hacia una esfera conductora. Tiras con carga del mismo signo tienden a separarse.','Activa y pausa la demostración para observar la apertura y caída gradual de las tiras sobre la esfera.','Representación cualitativa: no calcula voltaje ni descarga eléctrica.'),('EQUILIBRIO DE FUERZAS','Estática y dinámica · Poleas','Las tensiones de las cuerdas actúan sobre un nudo central. En equilibrio, la suma de fuerzas es cero.','Cambia las pesas y observa el desplazamiento del nudo. Las pesas laterales y las poleas se mueven de manera coordinada.','Cuerdas inextensibles, poleas ideales y amortiguamiento suave.')]
for i,(title,sub,body,task,note) in enumerate(entries):
 im=Image.new('RGB',(1000,720),'#f3f0e6');d=ImageDraw.Draw(im)
 d.rectangle((0,0,1000,12),fill='#d4a84b');d.text((55,40),f'ESTACIÓN {i+1:02} / LABORATORIO',font=f(20),fill='#347579');d.text((55,92),title,font=f(37),fill='#12383e');d.text((55,151),sub,font=f(23),fill='#567477')
 y=218
 for heading,txt in [('QUÉ ES',body),('QUÉ OBSERVAR',task),('ALCANCE',note)]:
  d.text((55,y),heading,font=f(17),fill='#9b742c');y+=28
  for line in textwrap.wrap(txt,72):d.text((55,y),line,font=f(22),fill='#233d43');y+=29
  y+=19
 d.rounded_rectangle((50,622,950,684),radius=12,fill='#153f45');d.text((76,641),'ESPACIO  iniciar / pausar     R  reiniciar     C  configuración',font=f(22),fill='white')
 im.save(R/f'card{i}.png')
 im=Image.new('RGB',(512,256),'#f3f0e6');d=ImageDraw.Draw(im);d.text((28,24),f'EXPERIMENTO {i+1:02}',font=f(20),fill='#478085')
 y=74
 for line in textwrap.wrap(title,23):d.text((28,y),line,font=f(26),fill='#153f45');y+=34
 d.text((28,210),'Clic para consultar',font=f(19),fill='#946f25');im.save(R/f'label{i}.png')
im=Image.new('RGB',(1600,120),'#12383e');d=ImageDraw.Draw(im);d.text((30,18),'MATERIA / FUERZAS / MOVIMIENTO',font=f(30),fill='#edd9a6');d.text((30,65),'WASD mover · Botón derecho mirar · 1–7 estaciones · E ficha · Espacio pausa · R reinicia · C cambia · Esc cierra',font=f(23),fill='white');im.save(R/'hud.png')
im=Image.new('RGB',(1200,220),'#12383e');d=ImageDraw.Draw(im);d.text((65,35),'LABORATORIO',font=f(72),fill='#f0dfb4');d.text((69,130),'MATERIA   /   FUERZAS   /   MOVIMIENTO',font=f(33),fill='white');im.save(R/'sign.png')
im=Image.new('RGB',(800,800),'#101c26');d=ImageDraw.Draw(im);d.ellipse((45,45,755,755),fill='#e0d2ae')
for row in range(7):
 for col in range(5):
  x=80+col*135+(row%2)*30;y=85+row*90
  if (x-400)**2+(y-400)**2<250**2:
   d.rounded_rectangle((x,y,x+125,y+80),20,outline='#997a58',width=3);d.ellipse((x+45,y+24,x+64,y+43),fill='#86658c')
d.text((105,740),'Células vegetales · esquema ilustrativo',font=f(23),fill='white');im.save(R/'sample.png')
