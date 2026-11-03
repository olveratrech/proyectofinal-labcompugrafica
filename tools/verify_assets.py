"""Comprueba que cada pieza exportada tenga geometría, UV y mapas disponibles."""
from pathlib import Path
import json,hashlib
r=Path(__file__).resolve().parents[1];parts=(r/'assets/models/parts.txt').read_text().splitlines();stations=set();total=0
for entry in parts:
 s,name,parent,*pivot=entry.split();stations.add(s);p=r/f'assets/models/{s}_{name}.obj';data=p.read_text();n=sum(line.startswith('f ') for line in data.splitlines());assert n>0 and '\nvt ' in data and '\nvn ' in data
 assert p.stat().st_size<100_000_000;total+=n
 for kind in ['albedo','ao']:assert (r/f'assets/textures/{s}_{kind}.png').is_file()
 assert (r/f'assets/models/{s}_{name}.mtl').is_file()
assert len(stations)==7
for line in (r/'docs/base-sha256.txt').read_text().splitlines():
 digest,path=line.split('  ');assert hashlib.sha256((r/'src/base'/Path(path).name).read_bytes()).hexdigest()==digest
assert total==sum(item['triangles'] for item in json.loads((r/'docs/model-stats.json').read_text()))
print(f'{len(stations)} estaciones, {len(parts)} piezas, {total} triangulos. Base original intacta.')
