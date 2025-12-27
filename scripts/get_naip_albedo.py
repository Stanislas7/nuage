#!/usr/bin/env python3
import argparse
import subprocess
import sys


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Download NAIP imagery via STAC and export as GeoTIFF/JPEG/PNG."
    )
    ap.add_argument("--xmin", type=float, required=True)
    ap.add_argument("--ymin", type=float, required=True)
    ap.add_argument("--xmax", type=float, required=True)
    ap.add_argument("--ymax", type=float, required=True)
    ap.add_argument("--start", type=str, required=True, help="ISO8601 start datetime")
    ap.add_argument("--end", type=str, required=True, help="ISO8601 end datetime")
    ap.add_argument("--asset", type=str, default="", help="Preferred STAC asset key (falls back to visual/data if unset)")
    ap.add_argument("--limit", type=int, default=50, help="How many candidates to fetch from STAC. Default 50")
    ap.add_argument("--outdir", type=str, default="assets/scenery/naip/imagery", help="Output directory")
    ap.add_argument("--export", choices=["png", "jpg", "both"], default="png")
    ap.add_argument("--quality", type=int, default=92, help="JPG quality if exporting jpg/both")
    ap.add_argument("--utm", type=str, default="auto", help="Target projected CRS")
    ap.add_argument("--tr", type=float, default=1.0, help="Target resolution in meters (NAIP is 1m)")
    ap.add_argument("--fill-nodata", action="store_true", help="Fill nodata gaps after reprojection (avoids black holes).")
    args = ap.parse_args()

    cmd = [
        sys.executable,
        "scripts/get_s2_albedo.py",
        "--xmin", str(args.xmin),
        "--ymin", str(args.ymin),
        "--xmax", str(args.xmax),
        "--ymax", str(args.ymax),
        "--start", args.start,
        "--end", args.end,
        "--collection", "naip",
        "--skip-cloud-filter",
        "--outdir", args.outdir,
        "--export", args.export,
        "--quality", str(args.quality),
        "--utm", args.utm,
        "--tr", str(args.tr),
        "--aws-sign",
    ]
    if args.asset:
        cmd += ["--asset", args.asset]
    if args.limit:
        cmd += ["--limit", str(args.limit)]
    if args.fill_nodata:
        cmd.append("--fill-nodata")

    print("+", " ".join(cmd), flush=True)
    subprocess.check_call(cmd)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
