#!/usr/bin/env python3
"""Verify the numbers in paper/generated/ (numbers.tex macros and tab_*.tex
cells) against independent recomputation from row-level sources:
combined_per_combination.csv (v2), paired_gate1.csv and
quality_by_model_language.csv (both checked against raw data by
evaluation/verify_consolidated_v2.py), findings.csv, inventory.json, the
review files and the AI-proposed triage CSV. summary.json is not used.
Prints every check that fails and a count of checks; exit code 1 on failure."""
import csv
import json
import math
import re
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
RES = ROOT / "evaluation" / "results"
V2 = RES / "benchmark-40-final-consolidated-v2"
GEN = ROOT / "paper" / "generated"
sys.path.insert(0, str(ROOT / "evaluation"))
from evaluate import wilson  # noqa: E402  (Wilson interval, unit-tested in evaluation/tests)

MODELS = ["claude-haiku-5-5", "gpt-6-luna", "gemini-3-8-flash", "mistral-large-4", "qwen3-8-flash"]
NAME = {"claude-haiku-5-5": "Claude Haiku 5.5", "gpt-6-luna": "GPT-6 Luna", "gemini-3-8-flash": "Gemini 3.8 Flash",
        "mistral-large-4": "Mistral Large 4", "qwen3-8-flash": "Qwen3.8 Flash"}
MAC = {"claude-haiku-5-5": "Haiku", "gpt-6-luna": "Luna", "gemini-3-8-flash": "Gemini", "mistral-large-4": "Mistral",
       "qwen3-8-flash": "Qwen"}
LANGS = ["python", "java", "c", "cpp"]
LM = {"python": "Py", "java": "Java", "c": "C", "cpp": "Cpp"}
fails, n = [], 0


def rd(p):
    with open(p, encoding="utf-8", newline="") as fh:
        return list(csv.DictReader(fh))


macros = dict(re.findall(r"\\newcommand\{\\(\w+)\}\{(.*)\}", (GEN / "numbers.tex").read_text(encoding="utf-8")))


def eq(label, got, want):
    global n
    n += 1
    if str(got) != str(want):
        fails.append(f"{label}: paper has {got!r}, source gives {want!r}")


checked = set()


def macro(name, want):
    checked.add(name)
    eq(f"\\{name}", macros.get(name), want)


def pct(k, nn, d=1):
    return f"{100 * k / nn:.{d}f}\\%"


rows = rd(V2 / "combined_per_combination.csv")
g1 = [r for r in rows if r["gate1_status"] == "pass"]
arts = [r for r in rows if r["generation_status"] in ("generated", "generated_truncated")]

# -- study level
macro("nPlanned", len(rows)); macro("nArtifacts", len(arts))
macro("nTruncated", sum(r["generation_status"] == "generated_truncated" for r in rows))
macro("nFailedGen", len(rows) - len(arts)); macro("nGOnePass", len(g1)); macro("pGOnePass", pct(len(g1), len(rows)))
lo, hi = wilson(len(g1), len(rows)); macro("ciGOnePass", f"[{100 * lo:.1f}, {100 * hi:.1f}]")
gc = Counter(r["gate1_status"] for r in rows)
for k, mname in (("fail_integrity", "nGOneIntegrity"), ("fail_syntax", "nGOneSyntax"), ("fail_compile", "nGOneCompile"),
                 ("blocked_dependency", "nGOneBlocked"), ("no_artifact", "nGOneNoArtifact")):
    macro(mname, gc[k])
macro("costTotal", f"{sum(float(r['cost_usd_total'] or 0) for r in rows):.2f}")

# -- per model / language and the model tables
tab_models = (GEN / "tab_models.tex").read_text(encoding="utf-8")
tab_gate1 = (GEN / "tab_gate1.tex").read_text(encoding="utf-8")
for m in MODELS:
    sel = [r for r in rows if r["model_label"] == m]
    ok = [r for r in sel if r["gate1_status"] == "pass"]
    a = [r for r in sel if r["generation_status"] in ("generated", "generated_truncated")]
    cost = sum(float(r["cost_usd_total"] or 0) for r in sel)
    trunc = sum(r["generation_status"] == "generated_truncated" for r in sel)
    macro(f"gOne{MAC[m]}", pct(len(ok), len(sel))); macro(f"cost{MAC[m]}", f"{cost:.3f}")
    macro(f"trunc{MAC[m]}", trunc); macro(f"failGen{MAC[m]}", len(sel) - len(a)); macro(f"art{MAC[m]}", len(a))
    macro(f"costPerPass{MAC[m]}", f"{1000 * cost / len(ok):.2f}")
    tokens = f"{sum(int(r['output_tokens'] or 0) for r in sel):,}".replace(",", "{,}")
    eq(f"tab_models {m}", f"{NAME[m]} & {len(a)}/160 & {trunc} & {len(sel) - len(a)} & {cost:.3f} & {tokens} \\\\" in tab_models, True)
    cells = []
    for l in LANGS:
        c = [r for r in sel if r["language"] == l]
        cells.append(f"{100 * sum(r['gate1_status'] == 'pass' for r in c) / len(c):.1f}")
    lo, hi = wilson(len(ok), len(sel))
    eq(f"tab_gate1 {m}", f"{NAME[m]} & " + " & ".join(cells) + f" & {100 * len(ok) / len(sel):.1f} [{100 * lo:.1f}, {100 * hi:.1f}] \\\\" in tab_gate1, True)
    mh = [r for r in g1 if r["model_label"] == m and int(r["sec_high"] or 0) + int(r["sec_medium"] or 0) > 0]
    macro(f"secMH{MAC[m]}", pct(len(mh), len(ok)))
for l in LANGS:
    sel = [r for r in rows if r["language"] == l]
    ok = [r for r in sel if r["gate1_status"] == "pass"]
    macro(f"gOne{LM[l]}", pct(len(ok), len(sel)))
    mh = [r for r in ok if int(r["sec_high"] or 0) + int(r["sec_medium"] or 0) > 0]
    macro(f"secMH{LM[l]}", pct(len(mh), len(ok)))
costs = {m: sum(float(r["cost_usd_total"] or 0) for r in rows if r["model_label"] == m) for m in MODELS}
macro("costRatioMistralLuna", f"{costs['mistral-large-4'] / costs['gpt-6-luna']:.0f}")

# -- functional funnel
ex = [r for r in rows if r["functional_status"] in ("passed", "failed", "error", "timeout")]
macro("nFuncExecuted", len(ex)); macro("nFuncPassed", sum(r["functional_status"] == "passed" for r in ex))
macro("nFuncFailed", sum(r["functional_status"] == "failed" for r in ex))
macro("nFuncOracleOk", sum(r["gate1_status"] == "pass" and r["oracle_status"] == "approved" for r in rows))
macro("pFuncCoverage", pct(len(ex), len(rows))); macro("pFuncCoverageGOne", pct(len(ex), len(g1)))
macro("nFuncTasks", len({r["task_id"] for r in ex}))
tab_func = (GEN / "tab_functional.tex").read_text(encoding="utf-8")
for key, label, pred in (("planned", "Planned combinations", lambda r: True),
                         ("gate1", "Passed gate 1", lambda r: r["gate1_status"] == "pass"),
                         ("executed", "Executed in sandbox", lambda r: r in ex),
                         ("passed", "Passed all checks", lambda r: r["functional_status"] == "passed")):
    vals = [sum(1 for r in rows if r["language"] == l and pred(r)) for l in LANGS]
    eq(f"tab_functional {key}", f"{label} & " + " & ".join(map(str, vals)) + f" & {sum(vals)} \\\\" in tab_func, True)

# -- reviews
orev = json.loads((ROOT / "evaluation/functional/oracle_review.json").read_text(encoding="utf-8"))
arev = json.loads((ROOT / "evaluation/functional/adapter_review.json").read_text(encoding="utf-8"))
macro("nOracleProposed", len(orev)); macro("nOracleApproved", sum(v["approved"] for v in orev.values()))
macro("nOracleRejected", sum(not v["approved"] for v in orev.values()))
macro("nOracleWrong", sum("WRONG expected" in v["notes"] for v in orev.values()))
macro("nAdapterReviewed", len(arev)); macro("nAdapterApproved", sum(v["approved"] for v in arev.values()))
macro("nAdapterPostExec", sum("after execution" in v["notes"] for v in arev.values()))

# -- findings and triage
fnd = rd(RES / "benchmark-40-final-scans/findings.csv")
tool = Counter(f["tool"] for f in fnd)
macro("nFindingsTotal", len(fnd))
for t in ("semgrep", "bandit", "flawfinder", "cppcheck"):
    macro(f"nFind{t.capitalize()}", tool[t])
gf = [f for f in fnd if f["gate1_status"] == "pass" and f["kind"] == "security"]
macro("nFindGOneSec", len(gf)); macro("nFindGOneMedHigh", sum(f["severity"] in ("medium", "high") for f in gf))
tri = rd(ROOT / "paper/data/triage_ai_proposed.csv")
tc = Counter(t["ai_proposed_label"] for t in tri)
macro("nTriage", len(tri)); macro("nTriageTP", tc["likely_true_positive"]); macro("nTriageNC", tc["needs_human_context"])
macro("nTriageFP", tc["likely_false_positive"])
eq("triage all rows AI-proposed (not human-confirmed)", all(t["status"] == "AI-proposed; NOT human-confirmed" and not t["human_label"] for t in tri), True)

# -- quality and paired tables
q = {(r["model"], r["language"]): r for r in rd(V2 / "quality_by_model_language.csv")}
tab_q = (GEN / "tab_quality.tex").read_text(encoding="utf-8")
for m in MODELS + ["all"]:
    r = q[(m, "all")]
    line = (f"{NAME.get(m, 'All')} & {float(r['mean_nloc']):.1f} & {float(r['mean_avg_ccn']):.2f} & {float(r['mean_ccd']):.3f} & "
            f"{r['functions_ccn_gt10']} & {float(r['cppcheck_per_kloc_c_cpp']):.2f} \\\\")
    eq(f"tab_quality {m}", line in tab_q, True)
tab_ql = (GEN / "tab_quality_lang.tex").read_text(encoding="utf-8")
for m in MODELS + ["all"]:
    cells = [f"{float(q[(m, l)]['mean_avg_ccn']):.2f}/{float(q[(m, l)]['mean_ccd']):.2f}" for l in LANGS]
    cells += [f"{float(q[(m, l)]['cppcheck_per_kloc_c_cpp']):.1f}" for l in ("c", "cpp")]
    eq(f"tab_quality_lang {m}", f"{NAME.get(m, 'All')} & " + " & ".join(cells) + " \\\\" in tab_ql, True)
macro("cppcheckMistral", f"{float(q[('mistral-large-4', 'all')]['cppcheck_per_kloc_c_cpp']):.1f}")
pairs = rd(V2 / "paired_gate1.csv")
tab_p = (GEN / "tab_paired_gate1.tex").read_text(encoding="utf-8")
short = {"claude-haiku-5-5": "Haiku", "gpt-6-luna": "Luna", "gemini-3-8-flash": "Gemini", "mistral-large-4": "Mistral", "qwen3-8-flash": "Qwen"}
for p in pairs:
    prefix = (f"{short[p['model_a']]} & {short[p['model_b']]} & {p['a_only']} & {p['b_only']} & "
              f"{100 * float(p['diff_a_minus_b']):+.1f} [{100 * float(p['diff_ci_lo']):+.1f}, {100 * float(p['diff_ci_hi']):+.1f}]")
    eq(f"tab_paired {p['model_a']}-{p['model_b']}", prefix in tab_p, True)
    eq(f"tab_paired {p['model_a']}-{p['model_b']} star", ("$^{*}$" in [l for l in tab_p.splitlines() if l.startswith(prefix)][0]),
       p["significant_holm_0.05"] == "True")
macro("nPairsSig", sum(p["significant_holm_0.05"] == "True" for p in pairs))

# -- remaining macros, each from its own source
cfg = json.loads((ROOT / "runner/run_config_benchmark_40task.json").read_text(encoding="utf-8"))
for k, mname in (("temperature", "temperature"), ("top_p", "topP"), ("max_tokens", "maxTokens"), ("max_attempts", "maxAttempts")):
    macro(mname, cfg[k])
macro("nTasks", len({r["task_id"] for r in rows}))
macro("nOutputTokens", f"{sum(int(r['output_tokens'] or 0) for r in rows):,}".replace(",", "{,}"))
macro("nTruncatedGOne", sum(r["generation_status"] == "generated_truncated" and r["gate1_status"] == "fail_integrity" for r in rows))
macro("nAdapterRejected", sum(not v["approved"] for v in arev.values()))
inv = json.loads((RES / "benchmark-40-final-gate1/inventory.json").read_text(encoding="utf-8"))["rows"]
failed = [r for r in inv if r["status"] == "failed"]
fv = Counter(r["validation_status"] for r in failed)
macro("nFailEmpty", fv["empty_response"]); macro("nFailRefusal", fv["refusal"])
macro("nFailInvalid", fv["invalid_syntax"] + fv["invalid_source"] + fv["heuristic_rejected"])
macro("nFailEmptyQwen", sum(r["validation_status"] == "empty_response" and r["model_label"] == "qwen3-8-flash" for r in failed))
macro("refusalTasks", ", ".join(str(t) for t in sorted({r["task_id"] for r in failed if r["validation_status"] == "refusal"})))
deps = Counter(d for r in rows if r["gate1_status"] == "blocked_dependency" for d in r["missing_dependencies"].split(";") if d)
macro("blockedDeps", ", ".join("\\texttt{" + d.replace("_", "\\_") + "}" for d, _ in deps.most_common()))
g1f = [f for f in fnd if f["gate1_status"] == "pass"]
macro("nFindGOne", len(g1f)); macro("nFindGOneLow", sum(f["kind"] == "security" and f["severity"] == "low" for f in g1f))
cwe = Counter(c for r in g1 for c in (r["cwes"] or "").split(";") if c)
macro("topCwes", ", ".join(f"{c} ({k})" for c, k in sorted(cwe.items(), key=lambda x: -x[1])[:8]))
macro("nTriageHigh", sum(t["severity"] == "high" for t in tri)); macro("nTriageMedium", sum(t["severity"] == "medium" for t in tri))
macro("nFlawfinderSampled", sum(t["tool"] == "flawfinder" for t in tri))
macro("nFlawfinderFP", sum(t["tool"] == "flawfinder" and t["ai_proposed_label"] == "likely_false_positive" for t in tri))
smeta = json.loads((RES / "benchmark-40-final-scans/scan_manifest.json").read_text(encoding="utf-8"))
macro("nScannedArtifacts", smeta["artifacts_scanned"]); macro("nSemgrepErrors", smeta["semgrep_errors"])
vers = (RES / "benchmark-40-final-scans/raw/versions.txt").read_text(encoding="utf-8")
for mname, rx in (("verSemgrep", r"^semgrep (\S+)"), ("verBandit", r"^bandit bandit (\S+)"), ("verFlawfinder", r"^flawfinder (\S+)"),
                  ("verLizard", r"^lizard (\S+)"), ("verCppcheck", r"^cppcheck Cppcheck (\S+)")):
    macro(mname, re.search(rx, vers, re.M).group(1))
snap = json.loads((RES / "SNAPSHOT_benchmark-40-final.json").read_text(encoding="utf-8"))
macro("snapshotId", snap["snapshot_id"][:12])
npass = sum(r["functional_status"] == "passed" for r in ex)
lo, hi = wilson(npass, len(ex))
macro("ciFuncPass", f"[{100 * lo:.1f}, 100.0]" if hi == 1.0 else f"[{100 * lo:.1f}, {100 * hi:.1f}]")
macro("nNoFunctions", int(q[("all", "all")]["gate1_pass_artifacts"]) - int(q[("all", "all")]["artifacts_with_functions"]))
macro("ccdAll", f"{float(q[('all', 'all')]['mean_ccd']):.3f}")
macro("cppcheckAll", f"{float(q[('all', 'all')]['cppcheck_per_kloc_c_cpp']):.1f}")
macro("cppcheckLuna", f"{float(q[('gpt-6-luna', 'all')]['cppcheck_per_kloc_c_cpp']):.1f}")
macro("nPairs", len(pairs)); macro("nPairCells", pairs[0]["cells"])
pl = {(p["model_a"], p["model_b"]): p for p in pairs}
macro("pHolmLunaGemini", f"{float(pl[('gemini-3-8-flash', 'gpt-6-luna')]['p_holm']):.2f}")
macro("pHolmLunaMistral", f"{float(pl[('gpt-6-luna', 'mistral-large-4')]['p_holm']):.3f}")
macro("pHolmGeminiMistral", f"{float(pl[('gemini-3-8-flash', 'mistral-large-4')]['p_holm']):.2f}")
macro("pRawGeminiMistral", f"{float(pl[('gemini-3-8-flash', 'mistral-large-4')]['p_exact_mcnemar']):.3f}")
macro("pHolmHaikuMistral", f"{float(pl[('claude-haiku-5-5', 'mistral-large-4')]['p_holm']):.2f}")
# prose claims about the paired results (RQ1 paragraph)
sig = {(p["model_a"], p["model_b"]): p["significant_holm_0.05"] == "True" for p in pairs}
for m in MODELS[:-1]:
    a, b = sorted([m, "qwen3-8-flash"])
    eq(f"claim: {m} beats Qwen (significant)", sig[(a, b)] and float(pl[(a, b)]["diff_a_minus_b"]) * (1 if a == m else -1) > 0, True)
for winner in ("gpt-6-luna", "gemini-3-8-flash"):
    a, b = sorted([winner, "claude-haiku-5-5"])
    eq(f"claim: {winner} beats Haiku", sig[(a, b)] and float(pl[(a, b)]["diff_a_minus_b"]) * (1 if a == winner else -1) > 0, True)
eq("claim: Luna beats Mistral", sig[("gpt-6-luna", "mistral-large-4")] and float(pl[("gpt-6-luna", "mistral-large-4")]["diff_a_minus_b"]) > 0, True)
eq("claim: C has highest mean CCN for every model",
   all(max(LANGS, key=lambda l: float(q[(m, l)]["mean_avg_ccn"])) == "c" for m in MODELS), True)
eq("claim: Mistral highest Cppcheck density",
   max(MODELS, key=lambda m: float(q[(m, "all")]["cppcheck_per_kloc_c_cpp"])) == "mistral-large-4", True)

# -- secondary cohort comparison: recomputed from cohort_rows.csv (row level; itself checked
#    against raw gate-1/scanner output by evaluation/base_compare/verify_base_comparison.py)
from scipy.stats import wilcoxon  # noqa: E402

crow = rd(RES / "base-40-comparison-v1/cohort_rows.csv")


def wil(k, nn, z=1.959963984540054):
    p = k / nn
    c = (p + z * z / (2 * nn)) / (1 + z * z / nn)
    h = z * math.sqrt(p * (1 - p) / nn + z * z / (4 * nn * nn)) / (1 + z * z / nn)
    return p, c - h, c + h


def coh(an, lang, coh_):
    key = "gate1_symmetric" if an == "SFour" else "gate1_status"
    sel = [r for r in crow if r["cohort"] == coh_ and (lang == "all" or r["language"] == lang)]
    if coh_ == "base" and an == "SOne":
        sel = [r for r in sel if r["base_class"] == "identical"]
    if coh_ == "base" and an == "STwo":
        sel = [r for r in sel if r["escape_pattern"] != "True"]
    if an == "SThree":
        sel = [r for r in sel if r[key] != "blocked_dependency"]
    ps = [r for r in sel if r[key] == "pass"]
    return {"planned": (len(ps), len(sel)), "artifact": (len(ps), sum(r["has_artifact"] == "True" for r in sel)),
            "medhigh": (sum(int(r["sec_high"]) + int(r["sec_medium"]) > 0 for r in ps), len(ps)), "pass": ps}


def dpp2(an, lang, meas):
    (k1, n1), (k2, n2) = coh(an, lang, "ours")[meas], coh(an, lang, "base")[meas]
    p1, l1, u1 = wil(k1, n1); p2, l2, u2 = wil(k2, n2)
    d = p1 - p2
    lo = d - math.sqrt((p1 - l1) ** 2 + (u2 - p2) ** 2); hi = d + math.sqrt((u1 - p1) ** 2 + (p2 - l2) ** 2)
    return d, lo, hi, f"${100 * d:+.1f}$~pp [${100 * lo:+.1f}$, ${100 * hi:+.1f}$]"


base_r = [r for r in crow if r["cohort"] == "base"]
macro("baseNFiles", len(base_r)); macro("baseNIdentical", sum(r["base_class"] == "identical" for r in base_r))
macro("baseNEscape", sum(r["escape_pattern"] == "True" for r in base_r))
macro("baseNEdited", sum(r["base_class"] != "identical" for r in base_r))
macro("baseNBlocked", sum(r["gate1_status"] == "blocked_dependency" for r in base_r))
ours_r = [r for r in crow if r["cohort"] == "ours"]
macro("oursNBlockedCCpp", sum(r["gate1_status"] == "blocked_dependency" for r in ours_r))
macro("oursNNoArtifactCCpp", sum(r["gate1_status"] == "no_artifact" for r in ours_r))
macro("oursNIntegrityCCpp", sum(r["gate1_status"] == "fail_integrity" for r in ours_r))
macro("cohDiffPlanned", dpp2("", "all", "planned")[3]); macro("cohDiffArtifact", dpp2("", "all", "artifact")[3])
macro("cohDiffMedHigh", dpp2("", "all", "medhigh")[3]); macro("cohDiffMedHighPy", dpp2("", "python", "medhigh")[3])
for tag in ("SOne", "STwo", "SThree", "SFour"):
    macro(f"cohDiffArtifact{tag}", dpp2(tag, "all", "artifact")[3])
macro("cohDiffMedHighSOne", dpp2("SOne", "all", "medhigh")[3]); macro("cohDiffMedHighSTwo", dpp2("STwo", "all", "medhigh")[3])
cellc = {}
for r in crow:
    k = (r["task_id"], r["language"])
    cellc.setdefault(k, [0, 0])[r["cohort"] == "base"] += r["gate1_status"] == "pass"
dd = [v[0] - v[1] for v in cellc.values()]
macro("cohCells", len(dd)); macro("cohCellsBaseHigher", sum(x < 0 for x in dd)); macro("cohCellsOursHigher", sum(x > 0 for x in dd))
macro("cohCellsEqual", sum(x == 0 for x in dd))
mant, ex = f"{wilcoxon(sorted(dd), zero_method='wilcox').pvalue:.1e}".split("e")
macro("cohWilcoxonP", f"${mant}\\times10^{{{int(ex)}}}$")


def mean_nloc(an, lang, c_):
    ps = coh(an, lang, c_)["pass"]
    return sum(int(r["nloc"]) for r in ps) / len(ps)


def cpk(c_):
    ps = [r for r in coh("", "all", c_)["pass"] if r["language"] in ("c", "cpp")]
    return 1000 * sum(int(r["cppcheck_findings"]) for r in ps) / sum(int(r["nloc"]) for r in ps)


macro("cohNlocOurs", f"{mean_nloc('', 'all', 'ours'):.1f}"); macro("cohNlocBase", f"{mean_nloc('', 'all', 'base'):.1f}")
macro("cohCppcheckOurs", f"{cpk('ours'):.1f}"); macro("cohCppcheckBase", f"{cpk('base'):.1f}")
tab = (GEN / "tab_base_cohort.tex").read_text(encoding="utf-8")
for lang, nm in (("c", "C"), ("cpp", "C++"), ("python", "Python"), ("all", "All")):
    cells = []
    for meas in ("planned", "artifact", "medhigh"):
        (k1, n1), (k2, n2) = coh("", lang, "ours")[meas], coh("", lang, "base")[meas]
        cells.append(f"{100 * k1 / n1:.1f} / {100 * k2 / n2:.1f}")
    cells.append(f"{mean_nloc('', lang, 'ours'):.1f} / {mean_nloc('', lang, 'base'):.1f}")
    line = f"{nm} & " + " & ".join(cells) + " \\\\"
    eq(f"tab_base_cohort row {nm}", line in tab.splitlines(), True)
# prose claims (secondary-analysis subsection)
inc0 = lambda t: dpp2(t, "all", "artifact")[1] < 0 < dpp2(t, "all", "artifact")[2]  # noqa: E731
eq("claim: per-artifact gate-1 CI includes 0 (primary, S2, S4)", all(inc0(t) for t in ("", "STwo", "SFour")), True)
eq("claim: per-artifact gate-1 CI excludes 0, base higher (S1, S3)",
   all(dpp2(t, "all", "artifact")[2] < 0 for t in ("SOne", "SThree")), True)
eq("claim: per-planned gate-1 CI below 0", dpp2("", "all", "planned")[2] < 0, True)
eq("claim: med/high CI includes 0 overall (primary, S1, S2)",
   all(dpp2(t, "all", "medhigh")[1] < 0 < dpp2(t, "all", "medhigh")[2] for t in ("", "SOne", "STwo")), True)
g1raw = [r for r in json.loads((RES / "benchmark-40-final-gate1/gate1_results.json").read_text(encoding="utf-8"))
         if r["language"] != "java" and r["gate1_status"] == "fail_integrity"]
eq("claim: our C/C++/Python integrity failures are mostly truncations",
   sum("truncated_finish_length" in r["integrity_failures"] for r in g1raw) > len(g1raw) / 2, True)
eq("claim: Python med/high CI below 0", dpp2("", "python", "medhigh")[2] < 0, True)

# -- pilot-10 supplementary check, recomputed from pilot_rows.csv
prow = rd(RES / "pilot-10-summary-v1/pilot_rows.csv")


def prate(sel, art=False):
    ps = [r for r in sel if r["gate1_status"] == "pass"]
    den = [r for r in sel if r["generation_status"] in ("generated", "generated_truncated")] if art else sel
    return pct(len(ps), len(den))


macro("pilotPlanned", len(prow)); macro("pilotGOnePass", prate(prow)); macro("pilotGOnePassArtifact", prate(prow, True))
for l, tag in (("python", "Py"), ("java", "Java"), ("c", "C"), ("cpp", "Cpp")):
    macro(f"pilotGOne{tag}", prate([r for r in prow if r["language"] == l]))
pc_ = [r for r in prow if r["language"] == "c" and r["gate1_status"] == "pass"]
macro("pilotMedHighC", pct(sum(int(r["medhigh_findings"]) > 0 for r in pc_), len(pc_)))
pp_ = [r for r in prow if r["gate1_status"] == "pass"]
macro("pilotMedHigh", pct(sum(int(r["medhigh_findings"]) > 0 for r in pp_), len(pp_)))
lg = {l: [r for r in prow if r["language"] == l] for l in ("python", "java", "c", "cpp")}
eq("claim: pilot language ordering Python > Java > C++ > C (gate 1)",
   [sum(r["gate1_status"] == "pass" for r in lg[l]) for l in ("python", "java", "cpp", "c")] ==
   sorted([sum(r["gate1_status"] == "pass" for r in lg[l]) for l in lg], reverse=True), True)
eq("claim: pilot C has the highest med/high share",
   max(lg, key=lambda l: (lambda ps: sum(int(r["medhigh_findings"]) > 0 for r in ps) / len(ps))(
       [r for r in lg[l] if r["gate1_status"] == "pass"])) == "c", True)

print(f"{n} checks, {len(fails)} failed")
for f in fails:
    print("  FAIL", f)
unchecked = sorted(set(macros) - checked)
print(f"macros verified: {len(checked)} of {len(macros)}; not covered: {unchecked}")
sys.exit(1 if fails or unchecked else 0)
