#!/usr/bin/env python3
"""
scan.py - Static security and quality scans over the frozen artifact snapshot.

Runs sandbox/run_scanners.sh inside the eval-sandbox WSL distro (static tools
only; no generated code is executed) with one configuration for all models and
languages, keeps the raw tool output, and normalises findings.

Security (potential findings, not confirmed vulnerabilities):
  semgrep (all languages; pinned cwe-top-25 + security-audit packs),
  bandit (Python), flawfinder (C/C++).
Reliability/quality: cppcheck (C/C++), lizard (size and complexity, all).

Usage:
  python scan.py --snapshot results/SNAPSHOT_benchmark-40-final.json
                 --gate1 results/benchmark-40-final-gate1 --out results/benchmark-40-final-scans
"""

import argparse
import csv
import io
import json
import re
import subprocess
import tarfile
import xml.etree.ElementTree as ET
from collections import Counter, defaultdict
from pathlib import Path

EVAL_DIR = Path(__file__).resolve().parent
ROOT = EVAL_DIR.parent
EXT = {"python": "py", "java": "java", "c": "c", "cpp": "cpp"}
FINDING_FIELDS = ["combo_index", "task_id", "language", "model_label", "gate1_status", "tool", "kind",
                  "rule_id", "severity", "cwe", "line", "message"]
ARTIFACT_FIELDS = ["combo_index", "task_id", "language", "model_label", "gate1_status", "nloc", "functions",
                   "avg_ccn", "max_ccn", "functions_ccn_gt10", "sec_findings", "sec_high", "sec_medium", "sec_low",
                   "sec_per_kloc", "semgrep", "bandit", "flawfinder", "cppcheck_findings", "cppcheck_errors",
                   "cwes", "any_security_finding", "any_high_security_finding"]


def cwe_ids(values) -> list[str]:
    out = []
    for v in values if isinstance(values, list) else [values]:
        for m in re.findall(r"CWE-(\d+)", str(v)):
            out.append(f"CWE-{m}")
    return sorted(set(out))


def combo_of(path: str) -> int | None:
    m = re.search(r"(?:^|/)(\d{4})\.(py|java|c|cpp)$", path.replace("\\", "/"))
    return int(m.group(1)) if m else None


def parse_semgrep(data: dict) -> list[dict]:
    sev = {"ERROR": "high", "WARNING": "medium", "INFO": "low"}
    out = []
    for r in data.get("results", []):
        meta = r.get("extra", {}).get("metadata", {})
        out.append({"combo_index": combo_of(r["path"]), "tool": "semgrep", "kind": "security",
                    "rule_id": r.get("check_id", "").split(".")[-1],
                    "severity": sev.get(r.get("extra", {}).get("severity", ""), "low"),
                    "cwe": ";".join(cwe_ids(meta.get("cwe", []))), "line": r.get("start", {}).get("line"),
                    "message": " ".join(r.get("extra", {}).get("message", "").split())[:200]})
    return out


def parse_bandit(data: dict) -> list[dict]:
    out = []
    for r in data.get("results", []):
        cwe = r.get("issue_cwe", {}).get("id")
        out.append({"combo_index": combo_of(r["filename"]), "tool": "bandit", "kind": "security",
                    "rule_id": f"{r.get('test_id')}:{r.get('test_name')}",
                    "severity": r.get("issue_severity", "LOW").lower(),
                    "cwe": f"CWE-{cwe}" if cwe else "", "line": r.get("line_number"),
                    "message": f"[confidence {r.get('issue_confidence', '').lower()}] " + " ".join(r.get("issue_text", "").split())[:180]})
    return out


def parse_flawfinder(text: str) -> list[dict]:
    out = []
    for r in csv.DictReader(io.StringIO(text)):
        level = int(r.get("Level") or 0)
        out.append({"combo_index": combo_of(r.get("File", "")), "tool": "flawfinder", "kind": "security",
                    "rule_id": f"{r.get('Category')}:{r.get('Name')}",
                    "severity": "high" if level >= 4 else "medium" if level >= 2 else "low",
                    "cwe": ";".join(cwe_ids(r.get("CWEs", ""))), "line": r.get("Line"),
                    "message": f"[level {level}] " + " ".join((r.get("Warning") or "").split())[:180]})
    return out


def parse_cppcheck(text: str) -> list[dict]:
    if not text.strip():
        return []
    sev = {"error": "high", "warning": "medium"}
    out = []
    for e in ET.fromstring(text).iter("error"):
        loc = e.find("location")
        if loc is None or e.get("id") in ("missingInclude", "missingIncludeSystem", "checkersReport",
                                          "toomanyconfigs", "normalCheckLevelMaxBranches"):
            continue
        out.append({"combo_index": combo_of(loc.get("file", "")), "tool": "cppcheck", "kind": "reliability_quality",
                    "rule_id": e.get("id"), "severity": sev.get(e.get("severity"), "low"),
                    "cwe": f"CWE-{e.get('cwe')}" if e.get("cwe") else "", "line": loc.get("line"),
                    "message": f"[{e.get('severity')}] " + " ".join((e.get("msg") or "").split())[:180]})
    return out


def parse_lizard(text: str) -> dict:
    """Per artifact: nloc, functions, CCN statistics. lizard --csv columns:
    NLOC, CCN, token, PARAM, length, location, file, function, long_name, start, end."""
    per = defaultdict(lambda: {"nloc": 0, "ccn": []})
    for row in csv.reader(io.StringIO(text)):
        if len(row) < 7 or not row[0].isdigit():
            continue
        c = combo_of(row[6])
        if c is None:
            continue
        per[c]["nloc"] += int(row[0])
        per[c]["ccn"].append(int(row[1]))
    return per


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[1])
    ap.add_argument("--snapshot", required=True)
    ap.add_argument("--gate1", required=True)
    ap.add_argument("--out", required=True)
    args = ap.parse_args(argv)
    snap = json.loads(Path(args.snapshot).read_text(encoding="utf-8"))
    inv = {r["combo_index"]: r for r in json.loads((Path(args.gate1) / "inventory.json").read_text(encoding="utf-8"))["rows"]}
    gate1 = {r["combo_index"]: r for r in json.loads((Path(args.gate1) / "gate1_results.json").read_text(encoding="utf-8"))}
    out = Path(args.out).resolve()
    if out.exists() and any(out.iterdir()):
        raise SystemExit(f"Output folder already has files: {out}")
    (out / "raw").mkdir(parents=True, exist_ok=True)

    buf = io.BytesIO()
    import hashlib
    with tarfile.open(fileobj=buf, mode="w") as tar:
        for a in snap["artifact_list"]:
            data = (ROOT / a["source_path"]).read_bytes()
            if hashlib.sha256(data).hexdigest() != a["sha256"]:
                raise SystemExit(f"artifact changed since the snapshot: {a['source_path']}")
            lang = inv[a["combo_index"]]["language"]
            info = tarfile.TarInfo(f"src/{lang}/{a['combo_index']:04d}.{EXT[lang]}")
            info.size, info.mode = len(data), 0o644
            tar.addfile(info, io.BytesIO(data))
        for rule in sorted((EVAL_DIR / "scanners" / "rules").glob("*.yml")):
            tar.add(rule, arcname=f"rules/{rule.name}")
    proc = subprocess.run(["wsl.exe", "-d", "eval-sandbox", "--", "/opt/eval-sandbox/run_scanners.sh"],
                          input=buf.getvalue(), capture_output=True, timeout=7200)
    if proc.returncode != 0 or not proc.stdout:
        raise SystemExit(f"scanner run failed: rc={proc.returncode} {proc.stderr[:500]!r}")
    with tarfile.open(fileobj=io.BytesIO(proc.stdout)) as tar:
        for m in tar.getmembers():
            if m.isfile():
                (out / "raw" / Path(m.name).name).write_bytes(tar.extractfile(m).read())

    raw = out / "raw"
    findings = []
    findings += parse_semgrep(json.loads((raw / "semgrep.json").read_text(encoding="utf-8")))
    if (raw / "bandit.json").stat().st_size:
        findings += parse_bandit(json.loads((raw / "bandit.json").read_text(encoding="utf-8")))
    findings += parse_flawfinder((raw / "flawfinder.csv").read_text(encoding="utf-8", errors="replace"))
    for f in ("cppcheck_c.xml", "cppcheck_cpp.xml"):
        findings += parse_cppcheck((raw / f).read_text(encoding="utf-8", errors="replace"))
    unmapped = [f for f in findings if f["combo_index"] is None]
    findings = [f for f in findings if f["combo_index"] is not None]
    for f in findings:
        r = inv[f["combo_index"]]
        f.update({"task_id": r["task_id"], "language": r["language"], "model_label": r["model_label"],
                  "gate1_status": gate1[f["combo_index"]]["gate1_status"]})
    findings.sort(key=lambda f: (f["combo_index"], f["tool"], int(f["line"] or 0), f["rule_id"]))
    with (out / "findings.csv").open("w", encoding="utf-8", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=FINDING_FIELDS, lineterminator="\n")
        w.writeheader()
        w.writerows({k: f.get(k, "") for k in FINDING_FIELDS} for f in findings)

    liz = parse_lizard((raw / "lizard.csv").read_text(encoding="utf-8", errors="replace"))
    by = defaultdict(list)
    for f in findings:
        by[f["combo_index"]].append(f)
    rows = []
    for a in snap["artifact_list"]:
        c = a["combo_index"]
        r, fs = inv[c], by.get(c, [])
        sec = [f for f in fs if f["kind"] == "security"]
        sev = Counter(f["severity"] for f in sec)
        tools = Counter(f["tool"] for f in fs)
        ccn = liz[c]["ccn"] if c in liz else []
        nloc = liz[c]["nloc"] if c in liz else 0
        rows.append({
            "combo_index": c, "task_id": r["task_id"], "language": r["language"], "model_label": r["model_label"],
            "gate1_status": gate1[c]["gate1_status"], "nloc": nloc, "functions": len(ccn),
            "avg_ccn": round(sum(ccn) / len(ccn), 2) if ccn else "", "max_ccn": max(ccn) if ccn else "",
            "functions_ccn_gt10": sum(1 for x in ccn if x > 10),
            "sec_findings": len(sec), "sec_high": sev["high"], "sec_medium": sev["medium"], "sec_low": sev["low"],
            "sec_per_kloc": round(1000 * len(sec) / nloc, 2) if nloc else "",
            "semgrep": tools["semgrep"], "bandit": tools["bandit"], "flawfinder": tools["flawfinder"],
            "cppcheck_findings": tools["cppcheck"],
            "cppcheck_errors": sum(1 for f in fs if f["tool"] == "cppcheck" and f["severity"] == "high"),
            "cwes": ";".join(sorted({c2 for f in sec for c2 in f["cwe"].split(";") if c2})),
            "any_security_finding": bool(sec), "any_high_security_finding": sev["high"] > 0,
        })
    with (out / "artifact_scan_summary.csv").open("w", encoding="utf-8", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=ARTIFACT_FIELDS, lineterminator="\n")
        w.writeheader()
        w.writerows(rows)
    errors = json.loads((raw / "semgrep.json").read_text(encoding="utf-8")).get("errors", [])
    meta = {"snapshot_id": snap["snapshot_id"], "artifacts_scanned": len(rows), "findings": len(findings),
            "unmapped_findings": len(unmapped), "semgrep_errors": len(errors),
            "semgrep_error_types": dict(Counter(e.get("type", "?") if isinstance(e.get("type"), str) else str(e.get("type"))[:40] for e in errors)),
            "versions": (raw / "versions.txt").read_text(encoding="utf-8").splitlines(),
            "config": {"semgrep": "--config cwe-top-25.yml --config security-audit.yml --timeout 30 --metrics=off",
                       "bandit": "-r src/python (all severities/confidences)", "flawfinder": "--minlevel=0",
                       "cppcheck": "--enable=warning,style,performance,portability (c17 / c++17)",
                       "lizard": "-l python -l java -l cpp"},
            "severity_mapping": {"semgrep": "ERROR=high WARNING=medium INFO=low",
                                 "bandit": "issue_severity", "flawfinder": "level>=4 high, 2-3 medium, 0-1 low",
                                 "cppcheck": "error=high warning=medium other=low (reliability/quality, not security)"},
            "findings_by_tool": dict(Counter(f["tool"] for f in findings))}
    (out / "scan_manifest.json").write_text(json.dumps(meta, indent=1) + "\n", encoding="utf-8")
    print(json.dumps({k: meta[k] for k in ("artifacts_scanned", "findings", "unmapped_findings", "semgrep_errors",
                                           "findings_by_tool")}, indent=1))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
