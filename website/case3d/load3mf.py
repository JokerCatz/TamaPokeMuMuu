# Shared helper for the Blender scripts: reads the parts of the Bambu Studio 3MF
# (extracted/) into Blender meshes, each centred on X/Y with its lowest point at z=0.
import os
import xml.etree.ElementTree as ET

import bpy

HERE = os.path.dirname(os.path.abspath(__file__))
X = os.path.join(HERE, "extracted")
CORE = "{http://schemas.microsoft.com/3dmanufacturing/core/2015/02}"
PATH = "{http://schemas.microsoft.com/3dmanufacturing/production/2015/06}path"


def _mat(t):
    return [float(x) for x in t.split()] if t else [1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0]


def _apply(m, p):
    x, y, z = p
    return (m[0] * x + m[3] * y + m[6] * z + m[9],
            m[1] * x + m[4] * y + m[7] * z + m[10],
            m[2] * x + m[5] * y + m[8] * z + m[11])


def _mul(a, b):
    o = _apply(a, _apply(b, (0, 0, 0)))
    cols = []
    for e in ((1, 0, 0), (0, 1, 0), (0, 0, 1)):
        q = _apply(a, _apply(b, e))
        cols += [q[0] - o[0], q[1] - o[1], q[2] - o[2]]
    return cols + list(o)


_files = {}


def _mesh(path, oid):
    if path not in _files:
        _files[path] = ET.parse(os.path.join(X, path.lstrip("/"))).getroot()
    for o in _files[path].iter(CORE + "object"):
        if o.get("id") == oid:
            vs = [(float(v.get("x")), float(v.get("y")), float(v.get("z"))) for v in o.iter(CORE + "vertex")]
            ts = [(int(t.get("v1")), int(t.get("v2")), int(t.get("v3"))) for t in o.iter(CORE + "triangle")]
            return vs, ts
    raise KeyError(oid)


def load_parts(wanted):
    """wanted: {objectid: blender name}. Returns {name: object}. Units: mm -> m / 1000."""
    root = ET.parse(os.path.join(X, "3D", "3dmodel.model")).getroot()
    objs = {o.get("id"): o for o in root.iter(CORE + "object")}
    out = {}
    for it in root.iter(CORE + "item"):
        oid = it.get("objectid")
        if oid not in wanted:
            continue
        T = _mat(it.get("transform"))
        verts, tris = [], []
        for c in objs[oid].findall(".//" + CORE + "component"):
            vs, ts = _mesh(c.get(PATH), c.get("objectid"))
            M = _mul(T, _mat(c.get("transform")))
            base = len(verts)
            verts += [_apply(M, v) for v in vs]
            tris += [(a + base, b + base, c2 + base) for a, b, c2 in ts]
        xs = [v[0] for v in verts]; ys = [v[1] for v in verts]; zs = [v[2] for v in verts]
        cx, cy, z0 = (min(xs) + max(xs)) / 2, (min(ys) + max(ys)) / 2, min(zs)
        verts = [((x - cx) / 1000, (y - cy) / 1000, (z - z0) / 1000) for x, y, z in verts]
        me = bpy.data.meshes.new(wanted[oid])
        me.from_pydata(verts, [], tris)
        me.update()
        ob = bpy.data.objects.new(wanted[oid], me)
        bpy.context.scene.collection.objects.link(ob)
        out[wanted[oid]] = ob
    return out


def material(name, rgb, rough=0.45):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    bsdf = next(n for n in m.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
    bsdf.inputs["Base Color"].default_value = (*rgb, 1)
    bsdf.inputs["Roughness"].default_value = rough
    m.diffuse_color = (*rgb, 1)
    return m


def clear():
    for ob in list(bpy.data.objects):
        bpy.data.objects.remove(ob, do_unlink=True)
