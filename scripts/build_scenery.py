#!/usr/bin/env python3
import argparse
import json
import math
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path
from typing import Any, Optional

GDAL_GIS_ORDER = ["--config", "OGR_CT_FORCE_TRADITIONAL_GIS_ORDER", "YES"]


def run(cmd: list[str], verbose: bool = True) -> None:
    if verbose:
        print("+", " ".join(cmd), flush=True)
    subprocess.check_call(cmd)


def gdalinfo_json(path: Path) -> dict[str, Any]:
    out = subprocess.check_output(["gdalinfo", "-json", str(path)], text=True)
    return json.loads(out)


def utm_from_bbox(xmin: float, ymin: float, xmax: float, ymax: float) -> str:
    lon = (xmin + xmax) * 0.5
    lat = (ymin + ymax) * 0.5
    zone = int(math.floor((lon + 180.0) / 6.0)) + 1
    if lat >= 0:
        epsg = 32600 + zone
    else:
        epsg = 32700 + zone
    return f"EPSG:{epsg}"


def ensure_tools(tools: list[str]) -> None:
    missing = [t for t in tools if shutil.which(t) is None]
    if missing:
        joined = ", ".join(missing)
        raise RuntimeError(f"Missing required tool(s): {joined}")


def ensure_dir(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)


def scaled_size(width: int, height: int, max_dim: int) -> tuple[int, int]:
    if max_dim <= 0:
        return width, height
    scale = min(1.0, max_dim / max(width, height))
    out_w = max(1, int(round(width * scale)))
    out_h = max(1, int(round(height * scale)))
    return out_w, out_h


def read_json(path: Path) -> Optional[dict[str, Any]]:
    if not path.exists():
        return None
    with path.open("r", encoding="utf-8") as f:
        return json.load(f)


def write_json(path: Path, data: dict[str, Any]) -> None:
    with path.open("w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)
        f.write("\n")


def build_preview_albedo(src_path: Path, out_texture: Path, out_preview: Path, max_dim: int, quality: int) -> tuple[int, int]:
    info = gdalinfo_json(src_path)
    width, height = info.get("size", [0, 0])
    out_w, out_h = scaled_size(int(width), int(height), max_dim)
    run([
        "gdal_translate",
        *GDAL_GIS_ORDER,
        "-of", "JPEG",
        "-b", "1", "-b", "2", "-b", "3",
        "-co", f"QUALITY={quality}",
        "-co", "PHOTOMETRIC=YCBCR",
        "-outsize", str(out_w), str(out_h),
        str(src_path), str(out_texture)
    ])
    if out_preview != out_texture:
        shutil.copyfile(out_texture, out_preview)
    return out_w, out_h


def build_preview_height(src_path: Path, out_preview: Path, max_dim: int) -> tuple[int, int]:
    info = gdalinfo_json(src_path)
    width, height = info.get("size", [0, 0])
    out_w, out_h = scaled_size(int(width), int(height), max_dim)
    run([
        "gdal_translate",
        *GDAL_GIS_ORDER,
        "-of", "PNG",
        "-outsize", str(out_w), str(out_h),
        str(src_path), str(out_preview)
    ])
    return out_w, out_h


def build_uint16_height(src_path: Path, out_path: Path, height_min: float, height_max: float) -> None:
    run([
        "gdal_translate",
        *GDAL_GIS_ORDER,
        "-of", "GTiff",
        "-ot", "UInt16",
        "-scale", str(height_min), str(height_max), "0", "65535",
        str(src_path), str(out_path)
    ])


def tile_pyramid(
    src_path: Path,
    out_dir: Path,
    tile_size: int,
    levels: int,
    fmt: str,
    ext: str,
    resample: str,
    quality: int,
    tmp_dir: Path,
    keep_temp: bool,
    rel_root: Path,
) -> list[dict[str, Any]]:
    info = gdalinfo_json(src_path)
    width, height = info.get("size", [0, 0])
    base_w = int(width)
    base_h = int(height)
    if base_w <= 0 or base_h <= 0:
        raise RuntimeError(f"Invalid raster size for {src_path}")
    max_levels = min(levels, int(math.floor(math.log2(max(base_w, base_h)))) + 1)
    level_entries: list[dict[str, Any]] = []

    total_tiles = 0
    level_tile_counts: list[int] = []
    for level in range(max_levels):
        scale = 2 ** level
        level_w = max(1, int(math.ceil(base_w / scale)))
        level_h = max(1, int(math.ceil(base_h / scale)))
        tiles_x = int(math.ceil(level_w / tile_size))
        tiles_y = int(math.ceil(level_h / tile_size))
        count = tiles_x * tiles_y
        level_tile_counts.append(count)
        total_tiles += count

    tile_done = 0
    print(f"Tile pyramid: {max_levels} levels, {total_tiles} tiles total")
    for level in range(max_levels):
        scale = 2 ** level
        level_w = max(1, int(math.ceil(base_w / scale)))
        level_h = max(1, int(math.ceil(base_h / scale)))

        if level == 0:
            level_src = src_path
        else:
            level_src = tmp_dir / f"{src_path.stem}_lod{level}.tif"
            run([
                "gdal_translate",
                *GDAL_GIS_ORDER,
                "-of", "GTiff",
                "-r", resample,
                "-outsize", str(level_w), str(level_h),
                str(src_path), str(level_src)
            ])

        level_info = gdalinfo_json(level_src)
        level_size = level_info.get("size", [0, 0])
        level_w = int(level_size[0])
        level_h = int(level_size[1])
        tiles_x = int(math.ceil(level_w / tile_size))
        tiles_y = int(math.ceil(level_h / tile_size))

        level_dir = out_dir / f"L{level:02d}"
        ensure_dir(level_dir)

        level_total = level_tile_counts[level]
        level_done = 0
        print(f"Tiling L{level:02d}: {level_total} tiles")
        for ty in range(tiles_y):
            for tx in range(tiles_x):
                xoff = tx * tile_size
                yoff = ty * tile_size
                xsize = min(tile_size, level_w - xoff)
                ysize = min(tile_size, level_h - yoff)
                tile_path = level_dir / f"x{tx}_y{ty}.{ext}"
                cmd = [
                    "gdal_translate",
                    *GDAL_GIS_ORDER,
                    "-q",
                    "-srcwin", str(xoff), str(yoff), str(xsize), str(ysize),
                    "-of", fmt,
                    str(level_src), str(tile_path)
                ]
                if fmt == "JPEG":
                    cmd += ["-b", "1", "-b", "2", "-b", "3", "-co", f"QUALITY={quality}", "-co", "PHOTOMETRIC=YCBCR"]
                if fmt == "PNG":
                    cmd += ["-co", "ZLEVEL=6"]
                run(cmd, verbose=False)
                level_done += 1
                tile_done += 1
                if level_done == 1 or level_done % 250 == 0 or level_done == level_total:
                    print(f"  L{level:02d} {level_done}/{level_total} (overall {tile_done}/{total_tiles})", flush=True)

        geo = level_info.get("geoTransform", [0, 1, 0, 0, 0, -1])
        rel_path = os.path.relpath(level_dir, rel_root)
        level_entries.append({
            "level": level,
            "width": level_w,
            "height": level_h,
            "tilesX": tiles_x,
            "tilesY": tiles_y,
            "origin": [geo[0], geo[3]],
            "pixelSize": [geo[1], geo[5]],
            "path": rel_path,
        })

        if level != 0 and not keep_temp:
            try:
                level_src.unlink()
            except OSError:
                pass

    return level_entries


def main() -> int:
    ap = argparse.ArgumentParser(description="Build quadtree-ready scenery assets (imagery + DEM) and previews.")
    ap.add_argument("--name", required=True, help="Scenery name under assets/scenery/<name>")
    ap.add_argument("--xmin", type=float, required=True)
    ap.add_argument("--ymin", type=float, required=True)
    ap.add_argument("--xmax", type=float, required=True)
    ap.add_argument("--ymax", type=float, required=True)
    ap.add_argument("--start", type=str, help="ISO8601 start datetime for imagery")
    ap.add_argument("--end", type=str, help="ISO8601 end datetime for imagery")
    ap.add_argument("--utm", type=str, default="auto", help="Target projected CRS or 'auto'")
    ap.add_argument("--imagery-tr", type=float, default=10.0, help="Imagery resolution in meters")
    ap.add_argument("--dem-tr", type=float, default=30.0, help="DEM resolution in meters")
    ap.add_argument("--tile-size", type=int, default=512, help="Tile size in pixels")
    ap.add_argument("--levels", type=int, default=5, help="Number of LOD levels to generate")
    ap.add_argument("--preview-max", type=int, default=4096, help="Max preview dimension in pixels")
    ap.add_argument("--max-resolution", type=int, default=512, help="Max terrain mesh resolution for preview config")
    ap.add_argument("--quality", type=int, default=92, help="JPEG quality for albedo tiles/previews")
    ap.add_argument("--cloud-lt", type=float, default=5.0, help="Cloud cover threshold for imagery search")
    ap.add_argument("--imagery-provider", choices=["s2", "naip"], default="s2", help="Imagery provider preset")
    ap.add_argument("--collection", type=str, default="sentinel-2-l2a", help="Imagery STAC collection")
    ap.add_argument("--asset", type=str, default="", help="Preferred STAC asset key")
    ap.add_argument("--limit", type=int, default=50, help="Max imagery items from STAC")
    ap.add_argument("--skip-cloud-filter", action="store_true")
    ap.add_argument("--fill-nodata", action="store_true")
    ap.add_argument("--skip-imagery", action="store_true")
    ap.add_argument("--reuse-imagery", action="store_true", help="Skip imagery download/reprojection if albedo_utm.tif already exists")
    ap.add_argument("--coverage-report", action="store_true", help="Run STAC coverage preflight and exit.")
    ap.add_argument("--stac", choices=["earth-search", "planetary"], default="earth-search", help="STAC endpoint preset")
    ap.add_argument("--stac-fallback", choices=["planetary", "earth-search"], default="", help="Fallback STAC endpoint if the first fails")
    ap.add_argument("--stac-retries", type=int, default=4, help="STAC retry count for 5xx/network errors")
    ap.add_argument("--asset-stac", choices=["earth-search", "planetary"], default="", help="STAC endpoint to resolve asset hrefs (defaults to search endpoint)")
    ap.add_argument("--skip-dem", action="store_true")
    ap.add_argument("--skip-tiles", action="store_true")
    ap.add_argument("--keep-temp", action="store_true")
    ap.add_argument("--activate", action="store_true", help="Write assets/config/terrain.json to point at this scenery")
    ap.add_argument("--out-root", type=str, default="assets/scenery")
    args = ap.parse_args()

    tools = ["gdalinfo", "gdal_translate", "gdalwarp", "gdalbuildvrt"]
    if args.fill_nodata:
        tools += ["gdal_fillnodata.py", "gdal_merge.py"]
    ensure_tools(tools)

    if args.tile_size <= 0 or args.levels <= 0:
        print("tile-size and levels must be positive", file=sys.stderr)
        return 2

    if args.reuse_imagery and args.skip_imagery:
        print("Use either --reuse-imagery or --skip-imagery, not both.", file=sys.stderr)
        return 2

    if not args.skip_imagery and not args.reuse_imagery and (not args.start or not args.end):
        print("Imagery requires --start and --end", file=sys.stderr)
        return 2

    bbox = [args.xmin, args.ymin, args.xmax, args.ymax]
    utm = utm_from_bbox(*bbox) if args.utm == "auto" else args.utm

    scenery_dir = Path(args.out_root) / args.name
    imagery_dir = scenery_dir / "imagery"
    heights_dir = scenery_dir / "heights"
    tmp_dir = scenery_dir / "_tmp"
    ensure_dir(imagery_dir)
    ensure_dir(heights_dir)
    ensure_dir(tmp_dir)

    if args.reuse_imagery:
        print(f"Imagery: reusing existing {albedo_utm}")
    if not args.skip_imagery and not args.reuse_imagery:
        collection = args.collection
        skip_cloud = args.skip_cloud_filter
        aws_sign = False
        if args.imagery_provider == "naip":
            if collection == "sentinel-2-l2a":
                collection = "naip"
            skip_cloud = True
            aws_sign = True

        imagery_cmd = [
            sys.executable, "scripts/get_s2_albedo.py",
            "--xmin", str(args.xmin),
            "--ymin", str(args.ymin),
            "--xmax", str(args.xmax),
            "--ymax", str(args.ymax),
            "--start", args.start,
            "--end", args.end,
            "--cloud-lt", str(args.cloud_lt),
            "--collection", collection,
            "--limit", str(args.limit),
            "--outdir", str(imagery_dir),
            "--utm", utm,
            "--tr", str(args.imagery_tr),
            "--export", "jpg",
            "--quality", str(args.quality),
        ]
        if args.stac:
            imagery_cmd += ["--stac", args.stac]
        if args.stac_fallback:
            imagery_cmd += ["--stac-fallback", args.stac_fallback]
        if args.stac_retries:
            imagery_cmd += ["--stac-retries", str(args.stac_retries)]
        if args.asset_stac:
            imagery_cmd += ["--asset-stac", args.asset_stac]
        if args.coverage_report:
            imagery_cmd.append("--preflight")
        if args.asset:
            imagery_cmd += ["--asset", args.asset]
        if skip_cloud:
            imagery_cmd.append("--skip-cloud-filter")
        if args.fill_nodata:
            imagery_cmd.append("--fill-nodata")
        if aws_sign:
            imagery_cmd.append("--aws-sign")
        if args.coverage_report:
            run(imagery_cmd)
            return 0
        print("Imagery: download + mosaic + reproject")
        t0 = time.time()
        run(imagery_cmd)
        print(f"Imagery complete in {time.time() - t0:.1f}s")

    if not args.skip_dem:
        dem_cmd = [
            sys.executable, "scripts/get_copernicus_dem.py",
            "--xmin", str(args.xmin),
            "--ymin", str(args.ymin),
            "--xmax", str(args.xmax),
            "--ymax", str(args.ymax),
            "--outdir", str(heights_dir),
            "--basename", "dem",
            "--utm", utm,
            "--tr", str(args.dem_tr),
            "--export", "tif",
        ]
        if args.fill_nodata:
            dem_cmd.append("--fill-nodata")
        print("DEM: download + mosaic + reproject")
        t0 = time.time()
        run(dem_cmd)
        print(f"DEM complete in {time.time() - t0:.1f}s")

    albedo_utm = imagery_dir / "albedo_utm.tif"
    dem_utm = heights_dir / "dem_utm.tif"
    dem_meta_path = heights_dir / "dem_meta.json"

    if args.reuse_imagery and not albedo_utm.exists():
        print(f"Missing imagery source: {albedo_utm}", file=sys.stderr)
        return 3
    if not args.skip_imagery and not args.reuse_imagery and not albedo_utm.exists():
        print(f"Missing imagery source: {albedo_utm}", file=sys.stderr)
        return 3
    if not args.skip_dem and not dem_utm.exists():
        print(f"Missing DEM source: {dem_utm}", file=sys.stderr)
        return 3

    height_meta = read_json(dem_meta_path) if not args.skip_dem else None
    height_min = float(height_meta["heightMin"]) if height_meta else 0.0
    height_max = float(height_meta["heightMax"]) if height_meta else 1.0

    albedo_preview_w = albedo_preview_h = 0
    height_preview_w = height_preview_h = 0
    if not args.skip_imagery:
        albedo_texture = imagery_dir / "albedo_texture.jpg"
        albedo_preview = imagery_dir / "albedo_preview.jpg"
        albedo_preview_w, albedo_preview_h = build_preview_albedo(
            albedo_utm, albedo_texture, albedo_preview, args.preview_max, args.quality
        )

    height_uint16 = heights_dir / "dem_utm_uint16.tif"
    if not args.skip_dem:
        if not height_uint16.exists():
            build_uint16_height(dem_utm, height_uint16, height_min, height_max)
        height_preview = heights_dir / "height_preview.png"
        height_preview_w, height_preview_h = build_preview_height(height_uint16, height_preview, args.preview_max)

    manifest: dict[str, Any] = {
        "name": args.name,
        "bbox": bbox,
        "utm": utm,
        "tileSize": args.tile_size,
        "layers": {},
    }

    if not args.skip_tiles and not args.skip_imagery:
        imagery_tiles = imagery_dir / "tiles" / "albedo"
        ensure_dir(imagery_tiles)
        print("Imagery: tiling pyramid")
        t0 = time.time()
        levels = tile_pyramid(
            albedo_utm,
            imagery_tiles,
            args.tile_size,
            args.levels,
            "JPEG",
            "jpg",
            "cubic",
            args.quality,
            tmp_dir,
            args.keep_temp,
            scenery_dir,
        )
        print(f"Imagery tiling complete in {time.time() - t0:.1f}s")
        manifest["layers"]["albedo"] = {
            "format": "jpg",
            "levels": levels,
        }

    if not args.skip_tiles and not args.skip_dem:
        height_tiles = heights_dir / "tiles" / "height"
        ensure_dir(height_tiles)
        print("Height: tiling pyramid")
        t0 = time.time()
        levels = tile_pyramid(
            height_uint16,
            height_tiles,
            args.tile_size,
            args.levels,
            "PNG",
            "png",
            "bilinear",
            args.quality,
            tmp_dir,
            args.keep_temp,
            scenery_dir,
        )
        print(f"Height tiling complete in {time.time() - t0:.1f}s")
        manifest["layers"]["height"] = {
            "format": "png",
            "heightMin": height_min,
            "heightMax": height_max,
            "levels": levels,
        }

    write_json(scenery_dir / "manifest.json", manifest)

    if height_meta:
        albedo_path = "imagery/albedo_texture.jpg" if not args.skip_imagery else ""
        terrain_cfg = {
            "heightmap": "heights/height_preview.png",
            "sizeX": height_meta.get("sizeX", 20000.0),
            "sizeZ": height_meta.get("sizeZ", 20000.0),
            "heightMin": height_min,
            "heightMax": height_max,
            "maxResolution": min(
                args.max_resolution,
                height_preview_w or args.max_resolution,
                height_preview_h or args.max_resolution,
            ),
            "flipY": True,
            "albedo": albedo_path,
        }
        write_json(scenery_dir / "terrain_preview.json", terrain_cfg)

        if args.activate:
            active_cfg = {
                **terrain_cfg,
                "heightmap": f"../scenery/{args.name}/heights/height_preview.png",
                "albedo": f"../scenery/{args.name}/imagery/albedo_texture.jpg" if not args.skip_imagery else "",
            }
            write_json(Path("assets/config/terrain.json"), active_cfg)

    if not args.keep_temp:
        try:
            shutil.rmtree(tmp_dir)
        except OSError:
            pass

    print("Done.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
