#!/usr/bin/env python3
import argparse
import shutil
from pathlib import Path


def resolve_path(root: Path, path_str: str) -> Path:
    path = Path(path_str)
    if path.is_absolute():
        return path
    return root / path


def main():
    parser = argparse.ArgumentParser(description="Manage Nuage scenery packs.")
    sub = parser.add_subparsers(dest="cmd", required=True)

    list_cmd = sub.add_parser("list", help="List available packs")

    activate_cmd = sub.add_parser("activate", help="Activate a pack")
    activate_cmd.add_argument("--pack", required=True, help="Pack path or pack name under assets/scenery/packs")
    activate_cmd.add_argument("--copy", action="store_true", help="Copy pack instead of symlink")

    args = parser.parse_args()
    repo_root = Path(__file__).resolve().parents[2]
    packs_root = repo_root / "assets" / "scenery" / "packs"
    active_path = repo_root / "assets" / "scenery" / "active"

    if args.cmd == "list":
        if not packs_root.exists():
            print("No packs directory found.")
            return
        for path in sorted(packs_root.iterdir()):
            if path.is_dir():
                print(path.name)
        return

    if args.cmd == "activate":
        pack_path = resolve_path(packs_root, args.pack)
        if not pack_path.exists():
            pack_path = resolve_path(repo_root, args.pack)
        if not pack_path.exists():
            raise SystemExit(f"Pack not found: {args.pack}")

        if active_path.exists() or active_path.is_symlink():
            if active_path.is_dir() and not active_path.is_symlink():
                shutil.rmtree(active_path)
            else:
                active_path.unlink()

        if args.copy:
            shutil.copytree(pack_path, active_path)
        else:
            active_path.symlink_to(pack_path, target_is_directory=True)
        print(f"Activated pack: {pack_path}")


if __name__ == "__main__":
    main()
