#!/usr/bin/env python3
import argparse
import json
import os
import subprocess
import sys
import time
import urllib.error
import urllib.request
import subprocess
from collections import defaultdict
from datetime import datetime
from typing import Any, Optional

EARTH_SEARCH = "https://earth-search.aws.element84.com/v1"
PLANETARY_STAC = "https://planetarycomputer.microsoft.com/api/stac/v1"
GDAL_GIS_ORDER = ["--config", "OGR_CT_FORCE_TRADITIONAL_GIS_ORDER", "YES"]


def run(cmd: list[str]) -> None:
    print("+", " ".join(cmd), flush=True)
    subprocess.check_call(cmd)


def http_json(url: str, payload: dict, retries: int = 4, backoff: float = 1.5) -> dict: # pyright: ignore[reportUnknownParameterType]
    data = json.dumps(payload).encode("utf-8")
    req = urllib.request.Request(
        url, data=data, headers={"Content-Type": "application/json"}, method="POST"
    )
    attempt = 0
    while True:
        try:
            with urllib.request.urlopen(req) as resp:
                return json.loads(resp.read().decode("utf-8"))
        except urllib.error.HTTPError as err:
            attempt += 1
            if attempt > retries or err.code < 500:
                raise
            wait_s = backoff ** attempt
            print(f"STAC POST failed ({err.code}); retrying in {wait_s:.1f}s...", file=sys.stderr)
            time.sleep(wait_s)
        except urllib.error.URLError:
            attempt += 1
            if attempt > retries:
                raise
            wait_s = backoff ** attempt
            print(f"STAC POST failed (network); retrying in {wait_s:.1f}s...", file=sys.stderr)
            time.sleep(wait_s)


def http_get_json(url: str, retries: int = 4, backoff: float = 1.5) -> dict:
    attempt = 0
    while True:
        try:
            with urllib.request.urlopen(url) as resp:
                return json.loads(resp.read().decode("utf-8"))
        except urllib.error.HTTPError as err:
            attempt += 1
            if attempt > retries or err.code < 500:
                raise
            wait_s = backoff ** attempt
            print(f"STAC GET failed ({err.code}); retrying in {wait_s:.1f}s...", file=sys.stderr)
            time.sleep(wait_s)
        except urllib.error.URLError:
            attempt += 1
            if attempt > retries:
                raise
            wait_s = backoff ** attempt
            print(f"STAC GET failed (network); retrying in {wait_s:.1f}s...", file=sys.stderr)
            time.sleep(wait_s)


def flag_true(value: Any) -> bool:
    if isinstance(value, bool):
        return value
    if isinstance(value, (int, float)):
        return bool(value)
    if isinstance(value, str):
        return value.lower() in {"true", "1", "yes"}
    return False


def download_asset(href: str, out_path: str, requester_pays: bool = False, aws_sign: bool = False) -> None:
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    if href.startswith("s3://"):
        # Public bucket/object; no credentials needed
        cmd = ["aws", "s3", "cp"]
        if not requester_pays and not aws_sign:
            cmd.append("--no-sign-request")
        if requester_pays:
            cmd += ["--request-payer", "requester"]
        cmd += [href, out_path]
        run(cmd)
    else:
        # HTTPS fallback
        print(f"+ curl -L {href} -o {out_path}", flush=True)
        run(["curl", "-L", href, "-o", out_path])


def is_valid_raster(path: str) -> bool:
    try:
        if os.path.getsize(path) < 1024:
            return False
    except OSError:
        return False
    try:
        subprocess.check_output(["gdalinfo", path], stderr=subprocess.DEVNULL)
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False
    return True


def iso_dt(s: str) -> datetime:
    # e.g. 2025-10-28T19:14:07.929000Z
    return datetime.fromisoformat(s.replace("Z", "+00:00"))


def pick_asset(assets: dict[str, Any], preferred: Optional[str]) -> tuple[Optional[str], Optional[dict[str, Any]]]:
    if preferred:
        asset = assets.get(preferred)
        if asset:
            return preferred, asset

    for key in ("visual", "image"):
        asset = assets.get(key)
        if asset:
            return key, asset

    for key, asset in assets.items():
        roles = [r.lower() for r in asset.get("roles", []) if isinstance(r, str)]
        if "visual" in roles:
            return key, asset

    for key, asset in assets.items():
        roles = [r.lower() for r in asset.get("roles", []) if isinstance(r, str)]
        if "data" in roles or "analytic" in roles:
            return key, asset

    for key, asset in assets.items():
        return key, asset

    return None, None


def stac_url(base: str, path: str) -> str:
    return base.rstrip("/") + "/" + path.lstrip("/")


def stac_search(base: str, payload: dict, retries: int, backoff: float) -> dict:
    return http_json(stac_url(base, "search"), payload, retries=retries, backoff=backoff)


def stac_fetch(url: str, retries: int, backoff: float) -> dict:
    return http_get_json(url, retries=retries, backoff=backoff)


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Download STAC visual imagery (Sentinel-2 by default) over bbox, mosaic, clip, reproject, export."
    )
    ap.add_argument("--xmin", type=float, required=True)
    ap.add_argument("--ymin", type=float, required=True)
    ap.add_argument("--xmax", type=float, required=True)
    ap.add_argument("--ymax", type=float, required=True)
    ap.add_argument("--start", type=str, required=True, help="ISO8601 start, e.g. 2025-06-01T00:00:00Z")
    ap.add_argument("--end", type=str, required=True, help="ISO8601 end, e.g. 2025-10-31T23:59:59Z")
    ap.add_argument("--cloud-lt", type=float, default=5.0, help="Cloud cover threshold (percent). Default 5")
    ap.add_argument("--skip-cloud-filter", action="store_true", help="Skip the cloud-cover query (useful for collections without eo:cloud_cover).")
    ap.add_argument("--collection", type=str, default="sentinel-2-l2a", help="STAC collection to search")
    ap.add_argument("--asset", type=str, default="", help="Preferred STAC asset key (falls back to visual/data if unset)")
    ap.add_argument("--limit", type=int, default=50, help="How many candidates to fetch from STAC. Default 50")
    ap.add_argument("--max-pages", type=int, default=10, help="Max STAC pages to fetch when paging. Default 10")
    ap.add_argument("--max-items", type=int, default=500, help="Max STAC items to keep in memory. Default 500")
    ap.add_argument("--stac", choices=["earth-search", "planetary"], default="earth-search", help="STAC endpoint preset")
    ap.add_argument("--stac-fallback", choices=["planetary", "earth-search"], default="", help="Fallback STAC endpoint if the first fails")
    ap.add_argument("--stac-retries", type=int, default=4, help="STAC retry count for 5xx/network errors")
    ap.add_argument("--asset-stac", choices=["earth-search", "planetary"], default="", help="STAC endpoint to resolve asset hrefs (defaults to search endpoint)")
    ap.add_argument("--outdir", type=str, default="assets/scenery/sfo/imagery", help="Output directory")
    ap.add_argument("--export", choices=["png", "jpg", "both"], default="png")
    ap.add_argument("--quality", type=int, default=92, help="JPG quality if exporting jpg/both")
    ap.add_argument("--utm", type=str, default="EPSG:32610", help="Target projected CRS (Bay Area: EPSG:32610)")
    ap.add_argument("--tr", type=float, default=25.0, help="Target resolution in meters (Sentinel-2 visual ~10m). Default 25m for reasonable texture size.")
    ap.add_argument("--fill-nodata", action="store_true", help="Fill nodata gaps after reprojection (avoids black holes).")
    ap.add_argument("--aws-sign", action="store_true", help="Use signed AWS requests for s3:// assets (requires credentials).")
    ap.add_argument("--preflight", action="store_true", help="Report approximate coverage and exit without downloads.")
    ap.add_argument("--coverage-grid", type=int, default=64, help="Grid size for coverage estimate (NxN). Default 64.")
    args = ap.parse_args()

    bbox = [args.xmin, args.ymin, args.xmax, args.ymax]
    os.makedirs(args.outdir, exist_ok=True)
    dt = f"{args.start}/{args.end}"

    # 1) STAC search (with pagination)
    search_payload = {
        "collections": [args.collection],
        "bbox": bbox,
        "datetime": dt,
        "limit": args.limit,
    }
    if not args.skip_cloud_filter:
        search_payload["query"] = {"eo:cloud_cover": {"lt": args.cloud_lt}}

    base = EARTH_SEARCH if args.stac == "earth-search" else PLANETARY_STAC
    fallback = ""
    if args.stac_fallback:
        fallback = PLANETARY_STAC if args.stac_fallback == "planetary" else EARTH_SEARCH

    feats: list[dict[str, Any]] = []
    try:
        res = stac_search(base, search_payload, retries=args.stac_retries, backoff=1.5)
    except urllib.error.HTTPError as err:
        if fallback and err.code >= 500:
            print(f"Primary STAC failed ({err.code}); falling back to {fallback}", file=sys.stderr)
            res = stac_search(fallback, search_payload, retries=args.stac_retries, backoff=1.5)
            base = fallback
        else:
            raise
    except urllib.error.URLError:
        if fallback:
            print(f"Primary STAC failed (network); falling back to {fallback}", file=sys.stderr)
            res = stac_search(fallback, search_payload, retries=args.stac_retries, backoff=1.5)
            base = fallback
        else:
            raise
    feats.extend(res.get("features", []))
    next_link = None
    next_method = "GET"
    next_body: Optional[dict[str, Any]] = None
    for link in res.get("links", []):
        if link.get("rel") == "next":
            next_link = link.get("href")
            next_method = str(link.get("method", "GET")).upper()
            next_body = link.get("body") if isinstance(link.get("body"), dict) else None
            break

    page = 1
    while next_link and page < args.max_pages and len(feats) < args.max_items:
        if next_method == "POST":
            body = next_body or {}
            res = http_json(next_link, body, retries=args.stac_retries, backoff=1.5)
        else:
            res = stac_fetch(next_link, retries=args.stac_retries, backoff=1.5)
        feats.extend(res.get("features", []))
        next_link = None
        next_method = "GET"
        next_body = None
        for link in res.get("links", []):
            if link.get("rel") == "next":
                next_link = link.get("href")
                next_method = str(link.get("method", "GET")).upper()
                next_body = link.get("body") if isinstance(link.get("body"), dict) else None
                break
        page += 1

    if feats:
        feats = feats[: args.max_items]
    if not feats:
        print("No scenes found for that bbox/date/cloud threshold.", file=sys.stderr)
        return 2

    if args.preflight:
        grid = max(8, int(args.coverage_grid))
        xs = [args.xmin + (args.xmax - args.xmin) * (i + 0.5) / grid for i in range(grid)]
        ys = [args.ymin + (args.ymax - args.ymin) * (j + 0.5) / grid for j in range(grid)]
        covered = 0
        for y in ys:
            for x in xs:
                hit = False
                for f in feats:
                    fb = f.get("bbox")
                    if not fb or len(fb) < 4:
                        continue
                    if fb[0] <= x <= fb[2] and fb[1] <= y <= fb[3]:
                        hit = True
                        break
                if hit:
                    covered += 1
        total = grid * grid
        pct = (covered / total) * 100.0
        print(f"Preflight coverage (approx bbox hit-rate): {pct:.1f}%")
        print(f"Items scanned: {len(feats)} (pages <= {args.max_pages}, limit {args.limit})")
        return 0

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

    asset_base = base
    if args.asset_stac:
        asset_base = EARTH_SEARCH if args.asset_stac == "earth-search" else PLANETARY_STAC

    local_tifs = []
    for item in best_items:
        item_id = item["id"]
        item_url = stac_url(asset_base, f"collections/{args.collection}/items/{item_id}")
        full = stac_fetch(item_url, retries=args.stac_retries, backoff=1.5)

        assets = full.get("assets", {})
        asset_key, asset = pick_asset(assets, args.asset or None)
        if not asset:
            print(f"Skipping {item_id}: no suitable visual asset", file=sys.stderr)
            continue
        href = asset.get("href")
        if not href:
            print(f"Skipping {item_id}: asset '{asset_key}' missing href", file=sys.stderr)
            continue

        props = full.get("properties", {})
        requester_pays = flag_true(props.get("storage:requester_pays")) or flag_true(asset.get("storage:requester_pays"))

        out_tif = os.path.join(dl_dir, f"{item_id}_visual.tif")
        if not os.path.exists(out_tif):
            print(f"Downloading {item_id}")
            download_asset(href, out_tif, requester_pays=requester_pays, aws_sign=args.aws_sign)
            if not is_valid_raster(out_tif):
                print(f"Invalid download for {item_id}; retrying with Earth Search asset href.", file=sys.stderr)
                try:
                    fallback_item = stac_fetch(
                        stac_url(EARTH_SEARCH, f"collections/{args.collection}/items/{item_id}"),
                        retries=args.stac_retries,
                        backoff=1.5,
                    )
                    assets_fallback = fallback_item.get("assets", {})
                    asset_key_fallback, asset_fallback = pick_asset(assets_fallback, args.asset or None)
                    href_fallback = asset_fallback.get("href") if asset_fallback else None
                    if href_fallback:
                        if os.path.exists(out_tif):
                            os.remove(out_tif)
                        download_asset(href_fallback, out_tif, requester_pays=requester_pays, aws_sign=args.aws_sign)
                except urllib.error.HTTPError:
                    pass
        else:
            print(f"Already exists: {out_tif}")
        if not is_valid_raster(out_tif):
            print(f"Skipping {item_id}: invalid GeoTIFF download", file=sys.stderr)
            continue
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
