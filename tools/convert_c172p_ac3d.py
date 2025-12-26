#!/usr/bin/env python3
import argparse
from pathlib import Path
import shutil

ALLOWED_TEXTURES = {
    "fuselage.png",
    "wing.png",
    "tail.png",
    "glass-alpha.png",
}


def parse_object(lines, start_idx):
    line = lines[start_idx].strip()
    if not line.startswith("OBJECT "):
        raise ValueError(f"Expected OBJECT at line {start_idx + 1}")

    obj = {
        "name": None,
        "texture": None,
        "texrep": (1.0, 1.0),
        "texoff": (0.0, 0.0),
        "verts": [],
        "surfaces": [],
        "children": [],
    }
    idx = start_idx + 1

    while idx < len(lines):
        line = lines[idx].strip()
        if line.startswith("name "):
            obj["name"] = line.split(" ", 1)[1].strip().strip('"')
            idx += 1
        elif line.startswith("texture "):
            obj["texture"] = line.split(" ", 1)[1].strip().strip('"')
            idx += 1
        elif line.startswith("texrep "):
            parts = line.split()
            if len(parts) >= 3:
                obj["texrep"] = (float(parts[1]), float(parts[2]))
            idx += 1
        elif line.startswith("texoff "):
            parts = line.split()
            if len(parts) >= 3:
                obj["texoff"] = (float(parts[1]), float(parts[2]))
            idx += 1
        elif line.startswith("numvert "):
            count = int(line.split()[1])
            idx += 1
            verts = []
            for _ in range(count):
                x, y, z = (float(v) for v in lines[idx].split())
                verts.append((x, y, z))
                idx += 1
            obj["verts"] = verts
        elif line.startswith("numsurf "):
            count = int(line.split()[1])
            idx += 1
            surfaces = []
            for _ in range(count):
                if not lines[idx].strip().startswith("SURF"):
                    raise ValueError(f"Expected SURF at line {idx + 1}")
                idx += 1
                if not lines[idx].strip().startswith("mat"):
                    raise ValueError(f"Expected mat at line {idx + 1}")
                idx += 1
                ref_line = lines[idx].strip()
                if not ref_line.startswith("refs "):
                    raise ValueError(f"Expected refs at line {idx + 1}")
                ref_count = int(ref_line.split()[1])
                idx += 1
                refs = []
                for _ in range(ref_count):
                    parts = lines[idx].split()
                    vidx = int(parts[0])
                    u = float(parts[1])
                    v = float(parts[2])
                    refs.append((vidx, u, v))
                    idx += 1
                surfaces.append(refs)
            obj["surfaces"] = surfaces
        elif line.startswith("kids "):
            kids = int(line.split()[1])
            idx += 1
            children = []
            for _ in range(kids):
                child, idx = parse_object(lines, idx)
                children.append(child)
            obj["children"] = children
            break
        else:
            idx += 1
    return obj, idx


def flatten_objects(obj):
    objs = [obj]
    for child in obj["children"]:
        objs.extend(flatten_objects(child))
    return objs


def build_obj(ac_path, out_dir):
    lines = ac_path.read_text().splitlines()
    start_idx = None
    for idx, line in enumerate(lines):
        if line.strip().startswith("OBJECT "):
            start_idx = idx
            break
    if start_idx is None:
        raise ValueError("No OBJECT found in AC3D file")

    root, _ = parse_object(lines, start_idx)
    objects = flatten_objects(root)

    vertices = []
    uvs = []
    index_map = {}
    faces_by_texture = {tex: [] for tex in ALLOWED_TEXTURES}

    for obj in objects:
        texture = obj.get("texture")
        if texture not in ALLOWED_TEXTURES:
            continue
        texrep = obj.get("texrep", (1.0, 1.0))
        texoff = obj.get("texoff", (0.0, 0.0))
        verts = obj.get("verts", [])
        if not verts:
            continue
        for surf in obj.get("surfaces", []):
            face_indices = []
            for vidx, u, v in surf:
                try:
                    vx, vy, vz = verts[vidx]
                except IndexError:
                    continue
                u = u * texrep[0] + texoff[0]
                v = v * texrep[1] + texoff[1]
                key = (id(obj), vidx, u, v)
                if key not in index_map:
                    vertices.append((vx, vy, vz))
                    uvs.append((u, v))
                    index_map[key] = len(vertices)
                face_indices.append(index_map[key])
            if len(face_indices) < 3:
                continue
            if len(face_indices) == 3:
                faces_by_texture[texture].append(face_indices)
            else:
                for k in range(1, len(face_indices) - 1):
                    faces_by_texture[texture].append(
                        [face_indices[0], face_indices[k], face_indices[k + 1]]
                    )

    out_dir.mkdir(parents=True, exist_ok=True)
    obj_path = out_dir / "c172p.obj"
    mtl_path = out_dir / "c172p.mtl"

    material_map = {}
    with mtl_path.open("w") as mtl:
        for tex in sorted(ALLOWED_TEXTURES):
            mat_name = f"mat_{Path(tex).stem}"
            material_map[tex] = mat_name
            mtl.write(f"newmtl {mat_name}\n")
            mtl.write("Ka 1.000 1.000 1.000\n")
            mtl.write("Kd 1.000 1.000 1.000\n")
            mtl.write("Ks 0.000 0.000 0.000\n")
            if tex == "glass-alpha.png":
                mtl.write("d 0.6\n")
            else:
                mtl.write("d 1.0\n")
            mtl.write("illum 1\n")
            mtl.write(f"map_Kd {tex}\n\n")

    with obj_path.open("w") as obj:
        obj.write("mtllib c172p.mtl\n")
        for vx, vy, vz in vertices:
            obj.write(f"v {vx:.6f} {vy:.6f} {vz:.6f}\n")
        for u, v in uvs:
            obj.write(f"vt {u:.6f} {v:.6f}\n")

        for tex in sorted(ALLOWED_TEXTURES):
            faces = faces_by_texture.get(tex)
            if not faces:
                continue
            obj.write(f"g {Path(tex).stem}\n")
            obj.write(f"usemtl {material_map[tex]}\n")
            for face in faces:
                indices = " ".join(f"{i}/{i}" for i in face)
                obj.write(f"f {indices}\n")

    return obj_path, mtl_path


def copy_textures(source_dir, out_dir):
    for tex in ALLOWED_TEXTURES:
        src = source_dir / tex
        if not src.exists():
            raise FileNotFoundError(f"Missing texture: {src}")
        shutil.copy2(src, out_dir / tex)


def main():
    parser = argparse.ArgumentParser(description="Convert C172P AC3D to OBJ (exterior only).")
    parser.add_argument("ac3d", type=Path, help="Path to c172p.ac")
    parser.add_argument("out_dir", type=Path, help="Output directory for OBJ/MTL")
    args = parser.parse_args()

    obj_path, mtl_path = build_obj(args.ac3d, args.out_dir)
    copy_textures(args.ac3d.parent, args.out_dir)
    print(f"Wrote {obj_path} and {mtl_path}")


if __name__ == "__main__":
    main()
