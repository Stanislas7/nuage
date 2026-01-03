#!/usr/bin/env python3
import argparse
import json
import os
import shutil
import subprocess
import urllib.error
import urllib.request
import math
from datetime import datetime, timezone
from pathlib import Path


def run(cmd, dry_run=False):
    print("[scenery] " + cmd)
    if dry_run:
        return
    subprocess.run(cmd, shell=True, check=True)


def require_tool(name):
    if shutil.which(name) is None:
        raise RuntimeError(f"Missing required tool: {name}")


def gdal_raster_size(path: Path) -> tuple[int, int]:
    info = subprocess.check_output(f"gdalinfo -json \"{path}\"", shell=True, text=True)
    data = json.loads(info)
    size = data.get("size") or []
    if len(size) != 2:
        raise RuntimeError(f"Unable to read raster size for {path}")
    return int(size[0]), int(size[1])


def download_url(url: str, dest: Path, dry_run: bool) -> bool:
    print(f"[scenery] download {url}")
    if dry_run:
        return True
    try:
        dest.parent.mkdir(parents=True, exist_ok=True)
        urllib.request.urlretrieve(url, dest)
        return True
    except urllib.error.HTTPError as exc:
        print(f"[scenery] download failed ({exc.code}) for {url}")
        return False
    except Exception as exc:
        print(f"[scenery] download failed ({exc}) for {url}")
        return False


def ensure_list(value):
    if value is None:
        return []
    if isinstance(value, list):
        return value
    return [value]


def resolve_path(root: Path, path_str: str) -> Path:
    path = Path(path_str)
    if path.is_absolute():
        return path
    return root / path


def main():
    parser = argparse.ArgumentParser(description="Build a Nuage scenery pack.")
    parser.add_argument("--config", default="assets/scenery/regions/bay_area.json")
    parser.add_argument("--clean", action="store_true")
    parser.add_argument("--download", action="store_true")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[2]
    config_path = resolve_path(repo_root, args.config)
    if not config_path.exists():
        raise SystemExit(f"Config not found: {config_path}")

    config = json.loads(config_path.read_text())
    bbox = config["bbox"]
    sources = config["sources"]

    dem_src = resolve_path(repo_root, sources["dem"])
    landclass_src = resolve_path(repo_root, sources["landclass"])
    osm_src_value = sources.get("osm", "")
    osm_src = resolve_path(repo_root, osm_src_value) if osm_src_value else None
    airports_csv = resolve_path(repo_root, sources["airports"])
    runways_csv = resolve_path(repo_root, sources["runways"])
    landclass_map_value = config.get("landclassMap", "")
    landclass_map = resolve_path(repo_root, landclass_map_value) if landclass_map_value else None

    work_dir = resolve_path(repo_root, config["workDir"])
    output_pack = resolve_path(repo_root, config["outputPack"])
    tmp_compiled = work_dir / "compiled_tmp"
    runways_json = work_dir / "runways_source.json"

    terrainc = resolve_path(repo_root, config.get("terrainc", "build/terrainc"))
    ourairports = resolve_path(repo_root, config.get("ourairports_import", "build/ourairports_import"))

    require_tool("gdalwarp")
    require_tool("gdal_translate")

    if not terrainc.exists() or not ourairports.exists():
        raise SystemExit("Build tools not found. Build Nuage first to produce terrainc/ourairports_import.")
    if not airports_csv.exists() or not runways_csv.exists():
        raise SystemExit("Missing airports.csv or runways.csv.")

    if args.clean:
        if work_dir.exists():
            shutil.rmtree(work_dir)
        if output_pack.exists():
            shutil.rmtree(output_pack)

    work_dir.mkdir(parents=True, exist_ok=True)
    if tmp_compiled.exists() and not args.dry_run:
        shutil.rmtree(tmp_compiled)
    tmp_compiled.mkdir(parents=True, exist_ok=True)

    xmin = bbox["xmin"]
    ymin = bbox["ymin"]
    xmax = bbox["xmax"]
    ymax = bbox["ymax"]

    dem_clip = work_dir / "dem_clip.tif"
    landclass_clip = work_dir / "landclass_clip.tif"
    landclass_resampled = work_dir / "landclass_resampled.tif"
    landclass_gray = work_dir / "landclass_gray.tif"
    height_png = work_dir / "height.png"
    landclass_pgm = work_dir / "landclass.pgm"

    if args.download:
        downloads = config.get("downloads", {})
        dem_cfg = downloads.get("dem", {})
        wc_cfg = downloads.get("worldcover", {})
        osm_cfg = downloads.get("osm", {})

        if dem_cfg:
            if args.download or not dem_src.exists():
                require_tool("gdalbuildvrt")
                dem_dir = work_dir / "downloads" / "dem"
                dem_dir.mkdir(parents=True, exist_ok=True)
                tile_files = []

                def format_tile(lat, lon):
                    ns = "n" if lat >= 0 else "s"
                    ew = "e" if lon >= 0 else "w"
                    return f"{ns}{abs(lat):02d}{ew}{abs(lon):03d}"

                lat_min = int(bbox["ymin"] // 1)
                lat_max = int(bbox["ymax"] // 1)
                lon_min = int(bbox["xmin"] // 1)
                lon_max = int(bbox["xmax"] // 1)

                for lat in range(lat_min, lat_max + 1):
                    for lon in range(lon_min, lon_max + 1):
                        tile = format_tile(lat, lon)
                        base_tpls = ensure_list(dem_cfg.get("baseUrl", ""))
                        folder_tpls = ensure_list(dem_cfg.get("folderTemplate", ""))
                        file_tpls = ensure_list(dem_cfg.get("fileTemplate", "USGS_13_{tile}.tif"))

                        candidates = []
                        for base_tpl in base_tpls:
                            for folder_tpl in folder_tpls:
                                for file_tpl in file_tpls:
                                    candidates.append((base_tpl, folder_tpl, file_tpl))

                        downloaded = False
                        dest = None
                        for base_tpl, folder_tpl, file_tpl in candidates:
                            base = base_tpl.format(tile=tile)
                            folder = folder_tpl.format(tile=tile)
                            filename = file_tpl.format(tile=tile)
                            url = base + folder + filename
                            dest = dem_dir / filename
                            if dest.exists():
                                downloaded = True
                                break
                            if download_url(url, dest, args.dry_run):
                                downloaded = True
                                break

                        if not downloaded:
                            raise SystemExit(f"Failed to download DEM tile {tile}")
                        tile_files.append(dest)

                if not tile_files:
                    raise SystemExit("No DEM tiles resolved for download.")
                if not args.dry_run:
                    dem_src.parent.mkdir(parents=True, exist_ok=True)
                run("gdalbuildvrt -overwrite \"{}\" {}".format(
                    dem_src, " ".join(f"\"{p}\"" for p in tile_files)
                ), args.dry_run)
        elif not dem_src.exists():
            raise SystemExit("Missing DEM source and no download config.")

        if wc_cfg:
            if args.download or not landclass_src.exists():
                require_tool("gdalbuildvrt")
                wc_dir = work_dir / "downloads" / "worldcover"
                wc_dir.mkdir(parents=True, exist_ok=True)
                tile_files = []

                def format_tile(lat, lon):
                    ns = "N" if lat >= 0 else "S"
                    ew = "E" if lon >= 0 else "W"
                    return f"{ns}{abs(lat):02d}{ew}{abs(lon):03d}"

                lat_min = int(bbox["ymin"] // 1)
                lat_max = int(bbox["ymax"] // 1)
                lon_min = int(bbox["xmin"] // 1)
                lon_max = int(bbox["xmax"] // 1)

                base_tpl = wc_cfg.get("baseUrl", "")
                year = wc_cfg.get("year", 2021)
                version = wc_cfg.get("version", "v200")
                file_tpl = wc_cfg.get(
                    "fileTemplate",
                    "ESA_WorldCover_10m_{year}_{version}_{tile}_Map.tif"
                )

                tile_size = int(wc_cfg.get("tileSizeDeg", 3))
                tiles = set()
                for lat in range(lat_min, lat_max + 1):
                    for lon in range(lon_min, lon_max + 1):
                        base_lat = math.floor(lat / tile_size) * tile_size
                        base_lon = math.floor(lon / tile_size) * tile_size
                        tiles.add((base_lat, base_lon))

                for base_lat, base_lon in sorted(tiles):
                    tile = format_tile(base_lat, base_lon)
                    filename = file_tpl.format(tile=tile, year=year, version=version)
                    base = base_tpl.format(tile=tile, year=year, version=version)
                    url = base + filename
                    dest = wc_dir / filename
                    if not dest.exists():
                        if not download_url(url, dest, args.dry_run):
                            raise SystemExit(f"Failed to download WorldCover tile {tile}")
                    tile_files.append(dest)

                if not tile_files:
                    raise SystemExit("No landclass tiles resolved for download.")
                if not args.dry_run:
                    landclass_src.parent.mkdir(parents=True, exist_ok=True)
                run("gdalbuildvrt -overwrite \"{}\" {}".format(
                    landclass_src, " ".join(f"\"{p}\"" for p in tile_files)
                ), args.dry_run)
        elif not landclass_src.exists():
            raise SystemExit("Missing landclass source and no download config.")

        if osm_src and not osm_src.exists():
            osm_url = osm_cfg.get("geofabrik", "")
            if osm_url:
                if not download_url(osm_url, osm_src, args.dry_run):
                    raise SystemExit("Failed to download OSM data.")

    if not dem_src.exists():
        raise SystemExit(f"Missing DEM source: {dem_src}")
    if not landclass_src.exists():
        raise SystemExit(f"Missing landclass source: {landclass_src}")

    run(
        f"gdalwarp -t_srs EPSG:4326 -te {xmin} {ymin} {xmax} {ymax} -te_srs EPSG:4326 "
        f"-r bilinear -overwrite \"{dem_src}\" \"{dem_clip}\"",
        args.dry_run,
    )
    run(
        f"gdal_translate -of PNG -ot UInt16 -scale {config['heightMin']} {config['heightMax']} 0 65535 "
        f"\"{dem_clip}\" \"{height_png}\"",
        args.dry_run,
    )

    run(
        f"gdalwarp -t_srs EPSG:4326 -te {xmin} {ymin} {xmax} {ymax} -te_srs EPSG:4326 "
        f"-r near -overwrite \"{landclass_src}\" \"{landclass_clip}\"",
        args.dry_run,
    )
    landclass_max_dim = config.get("landclassMaxDim", 4096)
    landclass_source = landclass_clip
    if landclass_max_dim and not args.dry_run:
        w, h = gdal_raster_size(landclass_clip)
        max_dim = max(w, h)
        if max_dim > landclass_max_dim:
            scale = landclass_max_dim / max_dim
            out_w = max(1, int(w * scale))
            out_h = max(1, int(h * scale))
            run(
                f"gdalwarp -r near -ts {out_w} {out_h} -overwrite "
                f"\"{landclass_clip}\" \"{landclass_resampled}\"",
                args.dry_run,
            )
            landclass_source = landclass_resampled
    run(
        f"gdal_translate -of GTiff -ot Byte -b 1 -colorinterp gray -co PHOTOMETRIC=MINISBLACK "
        f"\"{landclass_source}\" \"{landclass_gray}\"",
        args.dry_run,
    )
    run(
        f"gdal_translate -of PNM -ot Byte -b 1 -a_nodata 0 "
        f"\"{landclass_gray}\" \"{landclass_pgm}\"",
        args.dry_run,
    )

    mask_res = config["maskResolution"]
    tile_size = config["tileSizeMeters"]
    grid_res = config["gridResolution"]
    runway_blend = config.get("runwayBlendMeters", 60.0)

    if landclass_map and not landclass_map.exists():
        raise SystemExit(f"Missing landclass map: {landclass_map}")

    landclass_args = f"--landclass \"{landclass_pgm}\" --mask-res {mask_res}"
    if landclass_map:
        landclass_args += f" --landclass-map \"{landclass_map}\""
    use_osm = False
    if osm_src:
        if osm_src.exists():
            use_osm = True
        else:
            print(f"[scenery] OSM source missing, skipping: {osm_src}")
    if use_osm and landclass_src.exists() and not config.get("useOsmWithLandclass", False):
        use_osm = False
    if use_osm:
        require_tool("osmium")
    bbox_args = f"--xmin {xmin} --ymin {ymin} --xmax {xmax} --ymax {ymax}"
    base_args = (
        f"\"{terrainc}\" --heightmap \"{height_png}\" "
        f"--height-min {config['heightMin']} --height-max {config['heightMax']} "
        f"--tile-size {tile_size} --grid {grid_res} {bbox_args} {landclass_args}"
    )
    if use_osm:
        base_args += f" --osm \"{osm_src}\""

    run(f"{base_args} --out \"{tmp_compiled}\"", args.dry_run)

    run(
        f"\"{ourairports}\" --airports \"{airports_csv}\" --runways \"{runways_csv}\" "
        f"--manifest \"{tmp_compiled / 'manifest.json'}\" --out \"{runways_json}\" "
        f"--min-lat {ymin} --min-lon {xmin} --max-lat {ymax} --max-lon {xmax}",
        args.dry_run,
    )

    if output_pack.exists() and not args.dry_run:
        shutil.rmtree(output_pack)
    output_pack.mkdir(parents=True, exist_ok=True)

    run(
        f"{base_args} --runways-json \"{runways_json}\" --runway-blend {runway_blend} --out \"{output_pack}\"",
        args.dry_run,
    )

    pack_meta = {
        "name": config["name"],
        "bbox": bbox,
        "createdUtc": datetime.now(timezone.utc).isoformat(),
        "sources": sources,
        "landclassMap": str(landclass_map) if landclass_map else "",
    }
    if not args.dry_run:
        (output_pack / "pack.json").write_text(json.dumps(pack_meta, indent=2))
    print(f"[scenery] pack built: {output_pack}")


if __name__ == "__main__":
    main()
