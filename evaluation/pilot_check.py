#!/usr/bin/env python3
"""Supplementary disjoint-task check on the pilot-10 outputs (tasks 24, 37, 38, 39,
41, 42, 43, 46, 51, 104; same models, settings and prompt as benchmark-40).

The pilot was used to validate the generation runner, so it is reported separately
and never pooled with the 40-task study. Same code and configuration as the main
evaluation: gate 1 from evaluate.py (results/pilot-10-gate1-v1), scans from scan.py.
This script
  1. freezes a snapshot of the pilot artifacts (hashes + ledger hashes),
  2. runs scan.py on it,
  3. summarises with the consolidation-v2 definitions and cross-checks every
     per-artifact value against scan.py's artifact_scan_summary.csv.
No API calls; nothing is executed.

Usage: python pilot_check.py
"""
import csv
import hashlib
import json
from collections import Counter
from pathlib import Path

import scan
from consolidate import _wilson_raw, lizard_by_artifact

EVAL = Path(__file__).resolve().parent
ROOT = EVAL.parent
G1 = EVAL / "results/pilot-10-gate1-v1"
SNAP = EVAL / "results/SNAPSHOT_pilot-10.json"
SCANS = EVAL / "results/pilot-10-scans-v1"
OUT = EVAL / "results/pilot-10-summary-v1"
MODELS = ["claude-haiku-5-5", "gpt-6-luna", "gemini-3-8-flash", "mistral-large-4", "qwen3-8-flash"]
LANGS = ["python", "java", "c", "cpp"]


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def freeze():
    inv = json.loads((G1 / "inventory.json").read_text(encoding="utf-8"))
    arts = [{"combo_index": r["combo_index"], "source_path": r["source_path"], "sha256": r["source_sha256"]}
            for r in inv["rows"] if r["status"] in ("generated", "generated_truncated")]
    bad = [a["source_path"] for a in arts if sha(ROOT / a["source_path"]) != a["sha256"]]
    ledgers = [{"run_dir": i["run_dir"], "ledger_rows": i["ledger_rows"], "ledger_sha256": i["ledger_sha256"]}
               for i in inv["meta"]["inputs"]]
    changed = [i["run_dir"] for i in ledgers if sha(ROOT / i["run_dir"] / "cost_ledger.jsonl") != i["ledger_sha256"]]
    if changed:
        raise SystemExit(f"ledgers changed since gate 1: {changed}")
    snap = {"snapshot_id": hashlib.sha256("".join(a["sha256"] for a in arts).encode()).hexdigest(),
            "planned": len(inv["rows"]), "artifacts": len(arts), "status_counts": inv["meta"]["status_counts"],
            "ledgers": ledgers, "hash_mismatches": bad, "source": "evaluation/results/pilot-10-gate1-v1/inventory.json",
            "artifact_list": arts}
    if bad:
        raise SystemExit(f"artifacts changed since gate 1: {bad}")
    SNAP.write_text(json.dumps(snap, indent=1) + "\n", encoding="utf-8")
    return snap


def rate(k, n):
    lo, hi = _wilson_raw(k, n) if n else (None, None)
    return {"k": k, "n": n, "rate": k / n if n else None, "lo": lo, "hi": hi}


def fmt(r):
    return f"{r['k']}/{r['n']} = {100 * r['rate']:.1f}% [{100 * r['lo']:.1f}, {100 * r['hi']:.1f}]" if r["n"] else "n/a"


def main():
    if OUT.exists() and any(OUT.iterdir()):
        raise SystemExit(f"Output folder already has files: {OUT}")
    snap = json.loads(SNAP.read_text(encoding="utf-8")) if SNAP.exists() else freeze()
    if not (SCANS / "scan_manifest.json").exists():
        rc = scan.main(["--snapshot", str(SNAP), "--gate1", str(G1), "--out", str(SCANS)])
        if rc:
            raise SystemExit(rc)
    inv = {r["combo_index"]: r for r in json.loads((G1 / "inventory.json").read_text(encoding="utf-8"))["rows"]}
    g1 = {r["combo_index"]: r for r in json.loads((G1 / "gate1_results.json").read_text(encoding="utf-8"))}
    summ = {int(r["combo_index"]): r for r in csv.DictReader(open(SCANS / "artifact_scan_summary.csv", encoding="utf-8"))}
    liz = lizard_by_artifact(SCANS / "raw/lizard.csv")
    mh = Counter()
    for f in csv.DictReader(open(SCANS / "findings.csv", encoding="utf-8")):
        if f["kind"] == "security" and f["severity"] in ("high", "medium"):
            mh[int(f["combo_index"])] += 1
    # cross-check: medium/high counts and NLOC recomputed from raw files vs scan.py's per-artifact summary
    mism = [c for c, s in summ.items() if int(s["sec_high"]) + int(s["sec_medium"]) != mh[c] or int(s["nloc"] or 0) != liz.get(c, (0, 0, 0))[1]]

    rows = []
    for c, r in sorted(inv.items()):
        s = summ.get(c, {})
        sum_ccn, fn_nloc, nf = liz.get(c, (0, 0, 0))
        rows.append({"combo_index": c, "task_id": r["task_id"], "language": r["language"], "model": r["model_label"],
                     "generation_status": r["status"], "gate1_status": g1[c]["gate1_status"],
                     "nloc": fn_nloc, "sum_ccn": sum_ccn, "functions": nf,
                     "avg_ccn": s.get("avg_ccn", ""), "medhigh_findings": mh[c],
                     "cppcheck_findings": int(s.get("cppcheck_findings") or 0)})

    def stats(sel):
        p = [r for r in sel if r["gate1_status"] == "pass"]
        arts = [r for r in sel if r["generation_status"] in ("generated", "generated_truncated")]
        fn = [r for r in p if r["functions"]]
        cc = [r for r in p if r["language"] in ("c", "cpp")]
        nl, ncc = sum(r["nloc"] for r in p), sum(r["nloc"] for r in cc)
        return {"planned": len(sel), "artifacts": len(arts), "gate1_counts": dict(Counter(r["gate1_status"] for r in sel)),
                "gate1_pass": rate(len(p), len(sel)), "gate1_pass_of_artifacts": rate(len(p), len(arts)),
                "sec_medhigh": rate(sum(r["medhigh_findings"] > 0 for r in p), len(p)),
                "mean_nloc": nl / len(p) if p else None,
                "mean_avg_ccn": sum(r["sum_ccn"] / r["functions"] for r in fn) / len(fn) if fn else None,
                "mean_ccd": sum(r["sum_ccn"] / r["nloc"] for r in fn if r["nloc"]) / len(fn) if fn else None,
                "cppcheck_per_kloc_c_cpp": 1000 * sum(r["cppcheck_findings"] for r in cc) / ncc if ncc else None}

    res = {"snapshot_id": snap["snapshot_id"], "note": "pilot-10, disjoint from benchmark-40; reported separately, never pooled",
           "cross_check_mismatches": mism, "overall": stats(rows),
           "by_model": {m: stats([r for r in rows if r["model"] == m]) for m in MODELS},
           "by_language": {l: stats([r for r in rows if r["language"] == l]) for l in LANGS}}
    OUT.mkdir(parents=True, exist_ok=True)
    with (OUT / "pilot_rows.csv").open("w", encoding="utf-8", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=list(rows[0]), lineterminator="\n")
        w.writeheader()
        w.writerows(rows)
    (OUT / "summary.json").write_text(json.dumps(res, indent=1) + "\n", encoding="utf-8")

    o = res["overall"]
    md = ["# Pilot-10: supplementary disjoint-task check", "",
          "Tasks 24, 37, 38, 39, 41, 42, 43, 46, 51, 104 (all from the security strata A-D); same five models, settings and prompt "
          "as benchmark-40. The pilot runs were used to validate the generation runner, so these results are reported separately and "
          "never pooled with the 40-task study. Same gate 1 code and scanner configuration as the main evaluation. "
          "Functional tests were not run (no oracle review for these tasks). Scanner hits are potential findings, not confirmed vulnerabilities.", "",
          f"Snapshot `{snap['snapshot_id'][:12]}`: {snap['planned']} planned, {snap['artifacts']} artifacts, status {snap['status_counts']} "
          "(the one `incomplete` combination is a Qwen3.8 Flash request that ended in IncompleteRead; it counts as not passing). "
          f"Cross-check of per-artifact scanner values against raw output: {len(mism)} mismatches.", "",
          f"Gate 1 outcomes: {o['gate1_counts']}", "",
          "| group | gate 1 pass / planned | gate 1 pass / artifact | >=1 medium/high potential finding (of passes) | mean NLOC | mean CCN | mean CCD | Cppcheck/KLOC (C/C++) |",
          "|---|---|---|---|---|---|---|---|"]

    def line(name, s):
        f2 = lambda x, d: "n/a" if x is None else f"{x:.{d}f}"  # noqa: E731
        return (f"| {name} | {fmt(s['gate1_pass'])} | {fmt(s['gate1_pass_of_artifacts'])} | {fmt(s['sec_medhigh'])} | "
                f"{f2(s['mean_nloc'], 1)} | {f2(s['mean_avg_ccn'], 2)} | {f2(s['mean_ccd'], 3)} | {f2(s['cppcheck_per_kloc_c_cpp'], 2)} |")
    md.append(line("**All**", o))
    md += [line(m, res["by_model"][m]) for m in MODELS]
    md += [line(l, res["by_language"][l]) for l in LANGS]
    md += ["", "Cell sizes are small (40 combinations per model, 50 per language); intervals are wide. "
           "Not comparable one-to-one with benchmark-40: the pilot contains only security-strata tasks.", ""]
    (OUT / "RESULTS.md").write_text("\n".join(md), encoding="utf-8", newline="\n")
    print(json.dumps({"gate1": o["gate1_counts"], "pass": fmt(o["gate1_pass"]), "per_artifact": fmt(o["gate1_pass_of_artifacts"]),
                      "medhigh": fmt(o["sec_medhigh"]), "mismatches": len(mism)}, indent=1))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
