#!/usr/bin/env python3
import argparse
import json
import os
import subprocess
import sys
import urllib.request
from collections import defaultdict
from datetime import datetime
from typing import Any

EARTH_SEARCH = "https://earth-search.aws.element84.com/v1"
GDAL_GIS_ORDER = ["--config", "OGR_CT_FORCE_TRADITIONAL_GIS_ORDER", "YES"]


def run(cmd: list[str]) -> None:
    print("+", " ".join(cmd), flush=True)
    subprocess.check_call(cmd)


def http_json(url: str, payload: dict) -> dict: # pyright: ignore[reportUnknownParameterType]
    data = json.dumps(payload).encode("utf-8")
    req = urllib.request.Request(
        url, data=data, headers={"Content-Type": "application/json"}, method="POST"
    )
    with urllib.request.urlopen(req) as resp:
        return json.loads(resp.read().decode("utf-8"))


def http_get_json(url: str) -> dict:
    with urllib.request.urlopen(url) as resp:
        return json.loads(resp.read().decode("utf-8"))


def download_asset(href: str, out_path: str) -> None:
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    if href.startswith("s3://"):
        # Public bucket/object; no credentials needed
        run(["aws", "s3", "cp", "--no-sign-request", href, out_path])
    else:
        # HTTPS fallback
        print(f"+ curl -L {href} -o {out_path}", flush=True)
        run(["curl", "-L", href, "-o", out_path])


def iso_dt(s: str) -> datetime:
    # e.g. 2025-10-28T19:14:07.929000Z
    return datetime.fromisoformat(s.replace("Z", "+00:00"))


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Download Sentinel-2 L2A RGB (visual) over bbox, mosaic, clip, reproject, export."
    )
    ap.add_argument("--xmin", type=float, required=True)
    ap.add_argument("--ymin", type=float, required=True)
    ap.add_argument("--xmax", type=float, required=True)
    ap.add_argument("--ymax", type=float, required=True)
    ap.add_argument("--start", type=str, required=True, help="ISO8601 start, e.g. 2025-06-01T00:00:00Z")
    ap.add_argument("--end", type=str, required=True, help="ISO8601 end, e.g. 2025-10-31T23:59:59Z")
    ap.add_argument("--cloud-lt", type=float, default=5.0, help="Cloud cover threshold (percent). Default 5")
    ap.add_argument("--limit", type=int, default=50, help="How many candidates to fetch from STAC. Default 50")
    ap.add_argument("--outdir", type=str, default="assets/imagery", help="Output directory")
    ap.add_argument("--export", choices=["png", "jpg", "both"], default="png")
    ap.add_argument("--quality", type=int, default=92, help="JPG quality if exporting jpg/both")
    ap.add_argument("--utm", type=str, default="EPSG:32610", help="Target projected CRS (Bay Area: EPSG:32610)")
    ap.add_argument("--tr", type=float, default=25.0, help="Target resolution in meters (Sentinel-2 visual ~10m). Default 25m for reasonable texture size.")
    ap.add_argument("--fill-nodata", action="store_true", help="Fill nodata gaps after reprojection (avoids black holes).")
    args = ap.parse_args()

    bbox = [args.xmin, args.ymin, args.xmax, args.ymax]
    dt = f"{args.start}/{args.end}"

    # 1) STAC search
    search_payload = {
        "collections": ["sentinel-2-l2a"],
        "bbox": bbox,
        "datetime": dt,
        "limit": args.limit,
        "query": {"eo:cloud_cover": {"lt": args.cloud_lt}},
    }
    res = http_json(f"{EARTH_SEARCH}/search", search_payload)
    feats = res.get("features", [])
    if not feats:
        print("No scenes found for that bbox/date/cloud threshold.", file=sys.stderr)
        return 2

    # 2) Pick the best scene per MGRS tile so the mosaic covers the full bbox.
    #    Fallback to datetime grouping if tile ids are missing.
    tile_groups: dict[str, list[dict]] = defaultdict(list)
    for f in feats:
        props = f.get("properties", {})
        utm = props.get("mgrs:utm_zone")
        band = props.get("mgrs:latitude_band")
        grid = props.get("mgrs:grid_square")
        tile = None
        if utm and band and grid:
            tile = f"{utm}{band}{grid}"
        else:
            tile = props.get("mgrs:tile") or props.get("s2:mgrs_tile")
        if not tile:
            tile = props.get("s2:tile_id")
        if tile:
            tile_groups[str(tile)].append(f)

    best_items: list[dict] = []
    if tile_groups:
        for tile, fs in tile_groups.items():
            scored = []
            for f in fs:
                t = f["properties"].get("datetime")
                if not t:
                    continue
                cloud = float(f["properties"].get("eo:cloud_cover", 1000.0))
                scored.append((cloud, iso_dt(t), f))
            if scored:
                scored.sort(key=lambda x: (x[0], x[1]))  # lowest cloud, then earliest
                best_items.append(scored[0][2])
        print(f"Selected {len(best_items)} item(s) from {len(tile_groups)} tile(s) for mosaic")
    else:
        # Fallback: group by acquisition datetime; pick best datetime (lowest average cloud)
        groups: dict[str, list[dict]] = defaultdict(list)
        for f in feats:
            t = f["properties"].get("datetime")
            if t:
                groups[t].append(f)

        scored = []
        for t, fs in groups.items():
            clouds = [float(x["properties"].get("eo:cloud_cover", 1000.0)) for x in fs]
            avg = sum(clouds) / max(len(clouds), 1)
            scored.append((avg, iso_dt(t), t, len(fs)))

        scored.sort(key=lambda x: (x[0], x[1]))  # lowest cloud, then earliest
        best_avg, best_dt_obj, best_dt_str, best_count = scored[0]
        print(f"Selected acquisition datetime: {best_dt_str} (avg cloud {best_avg:.6f}, items {best_count})")
        best_items = groups[best_dt_str]

    # 3) Download visual assets for all items in that datetime group
    dl_dir = os.path.join(args.outdir, "s2_visual")
    os.makedirs(dl_dir, exist_ok=True)

    local_tifs = []
    for item in best_items:
        item_id = item["id"]
        item_url = f"{EARTH_SEARCH}/collections/sentinel-2-l2a/items/{item_id}"
        full = http_get_json(item_url)

        visual = full.get("assets", {}).get("visual", {})
        href = visual.get("href")
        if not href:
            print(f"Skipping {item_id}: no visual asset", file=sys.stderr)
            continue

        out_tif = os.path.join(dl_dir, f"{item_id}_visual.tif")
        if not os.path.exists(out_tif):
            print(f"Downloading {item_id}")
            download_asset(href, out_tif)
        else:
            print(f"Already exists: {out_tif}")
        local_tifs.append(out_tif)

    if not local_tifs:
        print("No visual GeoTIFFs downloaded.", file=sys.stderr)
        return 3

    # 4) Mosaic VRT
    vrt = os.path.join(args.outdir, "s2_visual_mosaic.vrt")
    run(["gdalbuildvrt", *GDAL_GIS_ORDER, vrt] + local_tifs)

    # 5) Clip to bbox (EPSG:4326)
    clip_wgs84 = os.path.join(args.outdir, "albedo_clip_wgs84.tif")
    run([
        "gdalwarp", "-overwrite",
        *GDAL_GIS_ORDER,
        "-t_srs", "EPSG:4326",
        "-te", str(args.xmin), str(args.ymin), str(args.xmax), str(args.ymax),
        "-te_srs", "EPSG:4326",
        "-r", "cubic",
        vrt, clip_wgs84
    ])

    # 6) Reproject to UTM and set meter resolution
    albedo_utm = os.path.join(args.outdir, "albedo_utm.tif")
    run([
        "gdalwarp", "-overwrite",
        *GDAL_GIS_ORDER,
        "-t_srs", args.utm,
        "-tr", str(args.tr), str(args.tr),
        "-r", "cubic",
        clip_wgs84, albedo_utm
    ])

    # 7) Optional nodata fill to avoid black gaps near tile edges.
    export_source = albedo_utm
    if args.fill_nodata:
        # gdal_fillnodata only works on a single band, so fill each band and re-stack.
        filled_utm = os.path.join(args.outdir, "albedo_utm_filled.tif")
        filled_bands = []
        for band in (1, 2, 3):
            band_src = os.path.join(args.outdir, f"albedo_utm_band{band}.tif")
            band_filled = os.path.join(args.outdir, f"albedo_utm_band{band}_filled.tif")
            run(["gdal_translate", "-b", str(band), albedo_utm, band_src])
            run(["gdal_fillnodata.py", "-q", band_src, band_filled])
            filled_bands.append(band_filled)
        if os.path.exists(filled_utm):
            os.remove(filled_utm)
        run(["gdal_merge.py", "-separate", "-o", filled_utm] + filled_bands)
        export_source = filled_utm

    # 8) Export
    if args.export in ("png", "both"):
        out_png = os.path.join(args.outdir, "albedo.png")
        run(["gdal_translate", *GDAL_GIS_ORDER, "-of", "PNG", export_source, out_png])
        print(f"Wrote {out_png}")

    if args.export in ("jpg", "both"):
        out_jpg = os.path.join(args.outdir, "albedo.jpg")
        run(["gdal_translate", *GDAL_GIS_ORDER, "-of", "JPEG", "-co", f"QUALITY={args.quality}", export_source, out_jpg])
        print(f"Wrote {out_jpg}")

    print("Done.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
