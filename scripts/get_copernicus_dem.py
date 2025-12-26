#!/usr/bin/env python3
import argparse
import json
import math
import os
import subprocess
import sys
import urllib.request
from typing import Any

EARTH_SEARCH = "https://earth-search.aws.element84.com/v1"
GDAL_GIS_ORDER = ["--config", "OGR_CT_FORCE_TRADITIONAL_GIS_ORDER", "YES"]


def run(cmd: list[str]) -> None:
    print("+", " ".join(cmd), flush=True)
    subprocess.check_call(cmd)


def http_json(url: str, payload: dict) -> dict:  # pyright: ignore[reportUnknownParameterType]
    data = json.dumps(payload).encode("utf-8")
    req = urllib.request.Request(
        url, data=data, headers={"Content-Type": "application/json"}, method="POST"
    )
    with urllib.request.urlopen(req) as resp:
        return json.loads(resp.read().decode("utf-8"))


def download_asset(href: str, out_path: str) -> None:
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    if href.startswith("s3://"):
        run(["aws", "s3", "cp", "--no-sign-request", href, out_path])
    else:
        run(["curl", "-L", href, "-o", out_path])


def utm_from_bbox(xmin: float, ymin: float, xmax: float, ymax: float) -> str:
    lon = (xmin + xmax) * 0.5
    lat = (ymin + ymax) * 0.5
    zone = int(math.floor((lon + 180.0) / 6.0)) + 1
    if lat >= 0:
        epsg = 32600 + zone
    else:
        epsg = 32700 + zone
    return f"EPSG:{epsg}"


def gdalinfo_json(path: str, stats: bool = False) -> dict[str, Any]:
    cmd = ["gdalinfo", "-json"]
    if stats:
        cmd.append("-stats")
    cmd.append(path)
    out = subprocess.check_output(cmd, text=True)
    return json.loads(out)


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Download Copernicus DEM over bbox, mosaic, clip, reproject, export."
    )
    ap.add_argument("--xmin", type=float, required=True)
    ap.add_argument("--ymin", type=float, required=True)
    ap.add_argument("--xmax", type=float, required=True)
    ap.add_argument("--ymax", type=float, required=True)
    ap.add_argument("--collection", type=str, default="cop-dem-glo-30")
    ap.add_argument("--limit", type=int, default=100, help="Max items from STAC")
    ap.add_argument("--outdir", type=str, default="assets", help="Output directory")
    ap.add_argument("--basename", type=str, default="heights_dem", help="Output base name")
    ap.add_argument("--export", choices=["png", "tif", "both"], default="png")
    ap.add_argument("--utm", type=str, default="auto", help="Target projected CRS or 'auto'")
    ap.add_argument("--tr", type=float, default=30.0, help="Target resolution in meters")
    ap.add_argument("--fill-nodata", action="store_true", help="Fill nodata gaps")
    args = ap.parse_args()

    bbox = [args.xmin, args.ymin, args.xmax, args.ymax]
    utm = utm_from_bbox(*bbox) if args.utm == "auto" else args.utm

    search_payload = {
        "collections": [args.collection],
        "bbox": bbox,
        "limit": args.limit,
    }
    res = http_json(f"{EARTH_SEARCH}/search", search_payload)
    feats = res.get("features", [])
    if not feats:
        print("No DEM tiles found for that bbox.", file=sys.stderr)
        return 2

    dl_dir = os.path.join(args.outdir, "dem_tiles")
    os.makedirs(dl_dir, exist_ok=True)

    local_tifs: list[str] = []
    for feat in feats:
        item_id = feat.get("id", "unknown")
        assets = feat.get("assets", {})
        data = assets.get("data")
        href = data.get("href") if isinstance(data, dict) else None
        if not href:
            print(f"Skipping {item_id}: no data asset", file=sys.stderr)
            continue
        out_tif = os.path.join(dl_dir, f"{item_id}.tif")
        if not os.path.exists(out_tif):
            print(f"Downloading {item_id}")
            download_asset(href, out_tif)
        else:
            print(f"Already exists: {out_tif}")
        local_tifs.append(out_tif)

    if not local_tifs:
        print("No DEM GeoTIFFs downloaded.", file=sys.stderr)
        return 3

    os.makedirs(args.outdir, exist_ok=True)
    vrt = os.path.join(args.outdir, f"{args.basename}_mosaic.vrt")
    run(["gdalbuildvrt", *GDAL_GIS_ORDER, vrt] + local_tifs)

    clip_wgs84 = os.path.join(args.outdir, f"{args.basename}_clip_wgs84.tif")
    run([
        "gdalwarp", "-overwrite",
        *GDAL_GIS_ORDER,
        "-t_srs", "EPSG:4326",
        "-te", str(args.xmin), str(args.ymin), str(args.xmax), str(args.ymax),
        "-te_srs", "EPSG:4326",
        "-r", "bilinear",
        vrt, clip_wgs84
    ])

    dem_utm = os.path.join(args.outdir, f"{args.basename}_utm.tif")
    run([
        "gdalwarp", "-overwrite",
        *GDAL_GIS_ORDER,
        "-t_srs", utm,
        "-tr", str(args.tr), str(args.tr),
        "-r", "bilinear",
        clip_wgs84, dem_utm
    ])

    export_source = dem_utm
    if args.fill_nodata:
        filled_utm = os.path.join(args.outdir, f"{args.basename}_utm_filled.tif")
        run(["gdal_fillnodata.py", "-q", dem_utm, filled_utm])
        export_source = filled_utm

    info = gdalinfo_json(export_source, stats=True)
    bands = info.get("bands", [])
    if not bands:
        print("Failed to read DEM stats.", file=sys.stderr)
        return 4

    stats = bands[0].get("statistics")
    if not stats:
        band_meta = bands[0].get("metadata", {})
        stats = band_meta.get("", {})

    height_min = float(stats.get("minimum", stats.get("STATISTICS_MINIMUM", 0.0)))
    height_max = float(stats.get("maximum", stats.get("STATISTICS_MAXIMUM", 1.0)))
    if math.isclose(height_min, height_max):
        height_max = height_min + 1.0

    geotransform = info.get("geoTransform", [0, 1, 0, 0, 0, -1])
    pixel_size_x = abs(float(geotransform[1]))
    pixel_size_y = abs(float(geotransform[5]))
    width = int(info.get("size", [0, 0])[0])
    height = int(info.get("size", [0, 0])[1])
    size_x = pixel_size_x * width
    size_z = pixel_size_y * height

    meta = {
        "heightMin": height_min,
        "heightMax": height_max,
        "sizeX": size_x,
        "sizeZ": size_z,
        "utm": utm,
        "bbox": bbox,
        "width": width,
        "height": height,
        "tr": args.tr,
    }

    meta_path = os.path.join(args.outdir, f"{args.basename}_meta.json")
    with open(meta_path, "w", encoding="utf-8") as f:
        json.dump(meta, f, indent=2)
        f.write("\n")

    if args.export in ("png", "both"):
        out_png = os.path.join(args.outdir, f"{args.basename}.png")
        run([
            "gdal_translate",
            *GDAL_GIS_ORDER,
            "-of", "PNG",
            "-ot", "UInt16",
            "-scale", str(height_min), str(height_max), "0", "65535",
            export_source, out_png
        ])
        print(f"Wrote {out_png}")

    if args.export in ("tif", "both"):
        out_tif = os.path.join(args.outdir, f"{args.basename}.tif")
        run(["gdal_translate", *GDAL_GIS_ORDER, export_source, out_tif])
        print(f"Wrote {out_tif}")

    print(f"Meta: {meta_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
