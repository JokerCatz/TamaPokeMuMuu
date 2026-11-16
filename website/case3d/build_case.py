# blender -b -P build_case.py -- [export]
# Assembles the TamaPoke Poke Ball case from the 3MF parts, adds the round screen
# in the slanted opening, renders previews and (with "export") writes the web GLB.
import math
import os
import sys

import bmesh
import bpy
from mathutils import Matrix, Vector

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from load3mf import clear, load_parts, material  # noqa: E402

ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
OUT = os.path.join(HERE, "renders")
os.makedirs(OUT, exist_ok=True)
MM = 0.001
BAND = 3.0 * MM  # the black ring between the halves (two thin printed rings)

clear()
p = load_parts({"2": "Shell", "10": "Lid", "14": "BtnInner", "16": "BtnOuter"})
shell, lid, b_in, b_out = p["Shell"], p["Lid"], p["BtnInner"], p["BtnOuter"]

RED = material("Red", (0.62, 0.008, 0.008), 0.38)
WHITE = material("White", (0.92, 0.92, 0.90), 0.45)
BLACK = material("Black", (0.02, 0.02, 0.025), 0.5)
GLASS = material("Glass", (0.005, 0.005, 0.008), 0.15)

# ---- bottom: the white dome upside down (rotate about Y keeps the front notch at -Y)
lid.data.transform(Matrix.Rotation(math.pi, 4, "Y"))
lid.data.update()
lid.data.materials.append(WHITE)

# ---- top: the red shell as printed (rim down), above the band
shell.location.z = BAND
shell.data.materials.append(RED)

# outer radius at the rim of the shell (for the band)
rim_r = max(math.hypot(v.co.x, v.co.y) for v in shell.data.vertices if v.co.z < 0.4 * MM)

# ---- black band: a short tube between the halves
bpy.ops.mesh.primitive_cylinder_add(vertices=128, radius=rim_r - 0.15 * MM, depth=BAND, location=(0, 0, BAND / 2))
band = bpy.context.active_object
band.name = "Band"
band.data.materials.append(BLACK)

# ---- the slanted screen opening: the plane that most top-edge vertices lie on
verts = [v.co.copy() for v in shell.data.vertices]
best = None
for ti in range(-60, 61):
    th = math.radians(ti * 0.5)
    for fi in range(-10, 11):
        ph = math.radians(fi * 0.5)
        n = Vector((math.sin(ph), math.sin(th), math.cos(th) * math.cos(ph))).normalized()
        d = max(n.dot(v) for v in verts)
        cnt = sum(1 for v in verts if n.dot(v) > d - 0.25 * MM)
        if best is None or cnt > best[0]:
            best = (cnt, n, d)
cnt, n, d = best
on_plane = [v for v in verts if n.dot(v) > d - 0.25 * MM]
c = sum(on_plane, Vector()) / len(on_plane)
inner = min((v - c - n * n.dot(v - c)).length for v in on_plane)
print(f"opening plane normal {tuple(round(x, 3) for x in n)} tilt {math.degrees(math.acos(n.z)):.1f} deg, "
      f"{cnt} verts, inner radius {inner / MM:.1f} mm, centre {tuple(round(x / MM, 1) for x in c)}")

# screen frame: z' = n, y' = towards the back (+Y) projected on the plane
up = (Vector((0, 1, 0)) - n * n.y).normalized()
right = up.cross(n).normalized()
frame = Matrix((right, up, n)).transposed().to_4x4()
c_world = c + Vector((0, 0, BAND))

def disc(name, radius, depth, mat, uv=False):
    bm = bmesh.new()
    bmesh.ops.create_circle(bm, cap_ends=True, radius=radius, segments=128)
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    if uv:
        uvl = me.uv_layers.new(name="UVMap")
        for loop in me.loops:
            co = me.vertices[loop.vertex_index].co
            uvl.data[loop.index].uv = (co.x / (2 * radius) + 0.5, co.y / (2 * radius) + 0.5)
    ob = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(ob)
    ob.matrix_world = Matrix.Translation(c_world - n * depth) @ frame
    me.materials.append(mat)
    return ob

SCREEN_R = min(inner - 0.6 * MM, 22.2 * MM)  # 1.75" round AMOLED: about 44 mm across
glass = disc("Glass", inner + 1.5 * MM, 1.6 * MM, GLASS)
scr_mat = material("ScreenMat", (1, 1, 1), 0.3)
screen = disc("Screen", SCREEN_R, 1.5 * MM, scr_mat, uv=True)

# ---- the front button, in the round hole the two half-notches make at -Y
r_out = max(math.hypot(v.co.x, v.co.y) for v in lid.data.vertices if abs(v.co.z) < 0.5 * MM)
for ob, depth, mat in ((b_out, 1.2, BLACK), (b_in, 0.2, WHITE)):
    ob.data.materials.append(mat)
    h = ob.dimensions.z
    ob.rotation_euler = (math.pi / 2, 0, 0)  # its flat face (+Z) now looks at -Y
    ob.location = (0, -(r_out - depth * MM) + h, BAND / 2 - 0 * MM)

bpy.context.view_layer.update()

# ---- previews
sc = bpy.context.scene
sc.render.engine = "BLENDER_WORKBENCH"
sc.render.resolution_x = sc.render.resolution_y = 520
sc.display.shading.light = "STUDIO"
sc.display.shading.color_type = "MATERIAL"
sc.display.shading.show_cavity = True
sc.render.film_transparent = True
cd = bpy.data.cameras.new("cam")
cd.lens = 70
cam = bpy.data.objects.new("cam", cd)
sc.collection.objects.link(cam)
sc.camera = cam
target = Vector((0, 0, 0))
for name, direction in (("front", (0, -1, 0.15)), ("three_quarter", (0.8, -1, 0.7)), ("screen", tuple(n)), ("back", (0.4, 1, 0.5))):
    dvec = Vector(direction).normalized()
    cam.location = target + dvec * 0.36
    cam.rotation_euler = (target - cam.location).to_track_quat("-Z", "Z" if name != "screen" else "Y").to_euler()
    sc.render.filepath = os.path.join(OUT, f"case_{name}.png")
    bpy.ops.render.render(write_still=True)

if "export" in ARGS:
    # smooth curves with the sharp edges kept. The red shell keeps every triangle
    # (collapsing it left blotches around the opening); the plain white dome is thinned.
    m = lid.modifiers.new("dec", "DECIMATE")
    m.ratio = 0.4
    for ob in (shell, lid, band, b_in, b_out):
        ob.data.shade_smooth()
        ob.data.set_sharp_from_angle(angle=math.radians(30))
        w = ob.modifiers.new("wn", "WEIGHTED_NORMAL")
        w.keep_sharp = True
    for ob in (shell, lid, band, b_in, b_out, glass, screen):
        ob.select_set(True)
    # turn the whole thing so the screen faces Blender -Y (glTF +Z, towards the web camera)
    root = bpy.data.objects.new("TamaPokeCase", None)
    sc.collection.objects.link(root)
    for ob in (shell, lid, band, b_in, b_out, glass, screen):
        ob.parent = root
        ob.matrix_parent_inverse = Matrix.Identity(4)
    root.matrix_world = Matrix.Translation(Vector((0, 0, 0))) @ (
        Vector((0, -1, 0)).rotation_difference(n).inverted().to_matrix().to_4x4()
    ) @ Matrix.Translation(-c_world)
    bpy.ops.object.select_all(action="DESELECT")
    for ob in (root, shell, lid, band, b_in, b_out, glass, screen):
        ob.select_set(True)
    path = os.path.join(HERE, "tamapoke_case.glb")
    bpy.ops.export_scene.gltf(filepath=path, export_format="GLB", use_selection=True, export_apply=True,
                              export_yup=True, export_materials="EXPORT", export_texcoords=True, export_normals=True)
    print("exported", path, os.path.getsize(path) // 1024, "KB")
