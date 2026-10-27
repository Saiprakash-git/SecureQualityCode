#!/usr/bin/env python3
"""Run our unchanged gate 1 and scan pipeline over the base paper's PUBLISHED
C, C++ and Python outputs for the 40 benchmark tasks (BASE_COMPARISON_PROTOCOL.md).

- Inputs: the 600 files listed in base_version_classes.csv (dataset/output).
  They are hashed, copied byte-for-byte into OUT/artifacts (set read-only) and
  never edited. The originals are re-hashed at the end.
- Gate 1: evaluate.py's own prepare_toolchain / evaluate_artifact with the
  toolchain section of eval_config_benchmark_40.json (same flags, timeouts).
- Scans: scan.py main() with a snapshot of the frozen copies (same sandbox
  script, tool versions and pinned rules).
Nothing is executed; gate 1 and the scanners only parse/compile.

Usage: python base_compare/run_base_pipeline.py --out results/base-40-published-v1
"""
import argparse
import csv
import datetime
import hashlib
import json
import os
import shutil
import stat
import sys
from pathlib import Path

EVAL_DIR = Path(__file__).resolve().parents[1]
ROOT = EVAL_DIR.parent
sys.path.insert(0, str(EVAL_DIR))
import evaluate  # noqa: E402
import scan  # noqa: E402


def sha(p: Path) -> str:
    return hashlib.sha256(p.read_bytes()).hexdigest()


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", required=True)
    ap.add_argument("--workers", type=int, default=4)
    args = ap.parse_args()
    out = Path(args.out).resolve()
    if out.exists() and any(out.iterdir()):
        raise SystemExit(f"Output folder already has files: {out}")
    rows = list(csv.DictReader((EVAL_DIR / "base_compare/base_version_classes.csv").open(encoding="utf-8")))
    rows.sort(key=lambda r: (r["model"], int(r["task_id"]), r["language"]))

    inv, artifacts, orig_hash = [], [], {}
    for i, r in enumerate(rows):
        src = ROOT / r["published"]
        h = sha(src)
        orig_hash[r["published"]] = h
        dst = out / "artifacts" / r["language"] / r["model"] / src.name
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(src, dst)
        os.chmod(dst, stat.S_IREAD)
        if sha(dst) != h:
            raise SystemExit(f"copy mismatch: {src}")
        rel = dst.relative_to(ROOT).as_posix()
        inv.append({"combo_index": i, "task_id": int(r["task_id"]), "language": r["language"],
                    "model_label": r["model"], "status": evaluate.GENERATED, "truncated": False,
                    "source_path": rel, "published_path": r["published"], "base_class": r["class"],
                    "source_sha256": h})
        artifacts.append({"combo_index": i, "source_path": rel, "sha256": h})
    snap_id = hashlib.sha256("".join(a["sha256"] for a in artifacts).encode()).hexdigest()
    snapshot = {"snapshot_id": snap_id, "description": "Base paper published outputs (dataset/output), 40 tasks, C/C++/Python",
                "created_at": datetime.datetime.now().isoformat(timespec="seconds"), "artifact_list": artifacts}
    evaluate.write_json(out / "SNAPSHOT_base-40-published.json", snapshot)

    gdir = out / "gate1"
    gdir.mkdir()
    evaluate.write_json(gdir / "inventory.json", {"rows": inv})
    cfg = evaluate.load_eval_config(EVAL_DIR / "eval_config_benchmark_40.json")
    tc = evaluate.prepare_toolchain(cfg)
    diag = gdir / "diagnostics"
    diag.mkdir()
    for lang, name in (("c", "w.c"), ("cpp", "w.cpp")):
        evaluate.check_c_family("int f(void){return 0;}\n", lang, name, tc)
    from concurrent.futures import ThreadPoolExecutor
    with ThreadPoolExecutor(max_workers=args.workers) as pool:
        gate1 = list(pool.map(lambda r: evaluate.evaluate_artifact(r, ROOT, tc, {}, diag), inv))
    gate1.sort(key=lambda r: r["combo_index"])
    evaluate.write_csv(gdir / "gate1_results.csv", evaluate.GATE1_FIELDS, gate1)
    evaluate.write_json(gdir / "gate1_results.json", gate1)
    evaluate.write_json(gdir / "eval_manifest.json", {
        "evaluator_version": evaluate.EVALUATOR_VERSION, "evaluator_sha256": sha(Path(evaluate.__file__)),
        "driver_sha256": sha(Path(__file__)), "toolchain_config": cfg["toolchain"], "versions": tc["versions"],
        "snapshot_id": snap_id, "generated_at": datetime.datetime.now().isoformat(timespec="seconds")})

    rc = scan.main(["--snapshot", str(out / "SNAPSHOT_base-40-published.json"), "--gate1", str(gdir),
                    "--out", str(out / "scans")])
    changed = [p for p, h in orig_hash.items() if sha(ROOT / p) != h]
    print(f"gate1: {dict(__import__('collections').Counter(g['gate1_status'] for g in gate1))}")
    print(f"published originals changed: {len(changed)}")
    return rc or (1 if changed else 0)


if __name__ == "__main__":
    raise SystemExit(main())
