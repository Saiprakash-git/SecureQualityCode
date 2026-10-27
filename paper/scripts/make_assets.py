#!/usr/bin/env python3
"""
make_assets.py - Generate every number, table and figure used in the paper
from the verified evaluation outputs (read-only). Nothing in the paper's
results is typed by hand: the text uses the macros in generated/numbers.tex.

Inputs (never modified):
  evaluation/results/benchmark-40-final-consolidated/{summary.json, combined_per_combination.csv}
  evaluation/results/benchmark-40-final-scans/{findings.csv, scan_manifest.json}
  evaluation/results/SNAPSHOT_benchmark-40-final.json
  evaluation/functional/{oracle_review.json, adapter_review.json}
  runner/run_config_benchmark_40task.json
  paper/data/triage_ai_proposed.csv   (AI-proposed labels, not human-confirmed)
Outputs: paper/generated/*.tex, paper/figures/*.pdf

Usage: python paper/scripts/make_assets.py
"""

import csv
import json
import re
from collections import Counter, defaultdict
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = Path(__file__).resolve().parents[2]
RES = ROOT / "evaluation" / "results"
CONS = RES / "benchmark-40-final-consolidated-v2"  # v2: Cppcheck/KLOC uses C/C++ NLOC; adds CCD and paired gate-1 stats
GEN = ROOT / "paper" / "generated"
FIG = ROOT / "paper" / "figures"
PREVIEW = None  # PNG previews for visual checking: --preview DIR
LANGS = ["python", "java", "c", "cpp"]
LANG_NAME = {"python": "Python", "java": "Java", "c": "C", "cpp": "C++"}
MODELS = ["claude-haiku-5-5", "gpt-6-luna", "gemini-3-8-flash", "mistral-large-4", "qwen3-8-flash"]
MODEL_NAME = {"claude-haiku-5-5": "Claude Haiku 5.5", "gpt-6-luna": "GPT-6 Luna", "gemini-3-8-flash": "Gemini 3.8 Flash",
              "mistral-large-4": "Mistral Large 4", "qwen3-8-flash": "Qwen3.8 Flash"}
MACRO = {"claude-haiku-5-5": "Haiku", "gpt-6-luna": "Luna", "gemini-3-8-flash": "Gemini",
         "mistral-large-4": "Mistral", "qwen3-8-flash": "Qwen"}
LANG_MACRO = {"python": "Py", "java": "Java", "c": "C", "cpp": "Cpp"}
# Validated categorical palette (dataviz reference palette, slots 1-5, light mode; all checks pass,
# contrast WARN handled by direct labels + table view + hatching).
COLOR = dict(zip(MODELS, ["#2a78d6", "#eb6834", "#1baf7a", "#eda100", "#e87ba4"]))
HATCH = dict(zip(MODELS, ["", "//", "..", "\\\\", "xx"]))
INK, INK2, MUTED, SURFACE = "#0b0b0b", "#52514e", "#898781", "#fcfcfb"

plt.rcParams.update({"font.family": "serif", "font.size": 7.5, "axes.edgecolor": MUTED, "axes.labelcolor": INK2,
                     "xtick.color": INK2, "ytick.color": INK2, "axes.linewidth": 0.6, "figure.dpi": 300,
                     "pdf.fonttype": 42, "axes.spines.top": False, "axes.spines.right": False})


def load_csv(p):
    with open(p, encoding="utf-8", newline="") as fh:
        return list(csv.DictReader(fh))


def pct(r, d=1):
    return f"{100 * r:.{d}f}\\%"


def frac(k, n):
    return f"{k}/{n}"


macros = {}


def m(name, value):
    if not re.fullmatch(r"[A-Za-z]+", name):
        raise ValueError(name)
    if name in macros and macros[name] != str(value):
        raise ValueError(f"macro {name} redefined")
    macros[name] = str(value)


def main():
    GEN.mkdir(parents=True, exist_ok=True)
    FIG.mkdir(parents=True, exist_ok=True)
    S = json.loads((CONS / "summary.json").read_text(encoding="utf-8"))
    rows = load_csv(CONS / "combined_per_combination.csv")
    snap = json.loads((RES / "SNAPSHOT_benchmark-40-final.json").read_text(encoding="utf-8"))
    scan_meta = json.loads((RES / "benchmark-40-final-scans" / "scan_manifest.json").read_text(encoding="utf-8"))
    findings = load_csv(RES / "benchmark-40-final-scans" / "findings.csv")
    orev = json.loads((ROOT / "evaluation/functional/oracle_review.json").read_text(encoding="utf-8"))
    arev = json.loads((ROOT / "evaluation/functional/adapter_review.json").read_text(encoding="utf-8"))
    cfg = json.loads((ROOT / "runner/run_config_benchmark_40task.json").read_text(encoding="utf-8"))
    triage = load_csv(ROOT / "paper/data/triage_ai_proposed.csv")
    O = S["overall"]

    # ---- study-level numbers
    m("nPlanned", O["planned"]); m("nArtifacts", O["artifacts"]); m("nTruncated", O["truncated"])
    m("nFailedGen", O["failed_generation"]); m("nTasks", len({r["task_id"] for r in rows}))
    m("snapshotId", snap["snapshot_id"][:12])
    g = O["gate1_counts"]
    m("nGOnePass", g.get("pass", 0)); m("nGOneIntegrity", g.get("fail_integrity", 0))
    m("nGOneSyntax", g.get("fail_syntax", 0)); m("nGOneCompile", g.get("fail_compile", 0))
    m("nGOneBlocked", g.get("blocked_dependency", 0)); m("nGOneNoArtifact", g.get("no_artifact", 0))
    m("pGOnePass", pct(O["gate1_pass"]["rate"]))
    m("ciGOnePass", f"[{100 * O['gate1_pass']['ci95'][0]:.1f}, {100 * O['gate1_pass']['ci95'][1]:.1f}]")
    m("temperature", cfg["temperature"]); m("topP", cfg["top_p"]); m("maxTokens", cfg["max_tokens"]); m("maxAttempts", cfg["max_attempts"])
    trunc_and_built = sum(1 for r in rows if r["generation_status"] == "generated_truncated")
    m("costTotal", f"{O['cost_usd']:.2f}")
    m("nOutputTokens", f"{O['output_tokens']:,}".replace(",", "{,}"))

    # ---- why generation failed (inventory validation status of the last attempt)
    inv = json.loads((RES / "benchmark-40-final-gate1" / "inventory.json").read_text(encoding="utf-8"))["rows"]
    failed = [r for r in inv if r["status"] == "failed"]
    fc = Counter(r["validation_status"] for r in failed)
    m("nFailEmpty", fc.get("empty_response", 0)); m("nFailRefusal", fc.get("refusal", 0))
    m("nFailInvalid", fc.get("invalid_syntax", 0) + fc.get("invalid_source", 0) + fc.get("heuristic_rejected", 0))
    m("nFailEmptyQwen", sum(1 for r in failed if r["validation_status"] == "empty_response" and r["model_label"] == "qwen3-8-flash"))
    refused = Counter(r["task_id"] for r in failed if r["validation_status"] == "refusal")
    m("refusalTasks", ", ".join(str(t) for t in sorted(refused)))

    # ---- per model / per language
    for mod in MODELS:
        s, k = S["by_model"][mod], MACRO[mod]
        m(f"gOne{k}", pct(s["gate1_pass"]["rate"])); m(f"cost{k}", f"{s['cost_usd']:.3f}")
        m(f"secMH{k}", pct(s["security_medhigh_finding"]["rate"])); m(f"trunc{k}", s["truncated"])
        m(f"failGen{k}", s["failed_generation"]); m(f"art{k}", s["artifacts"])
        # from unrounded totals (summary's cost_per_gate1_pass is pre-rounded to 6 decimals)
        m(f"costPerPass{k}", f"{1000 * s['cost_usd'] / s['gate1_pass']['k']:.2f}")
    for lang in LANGS:
        s, k = S["by_language"][lang], LANG_MACRO[lang]
        m(f"gOne{k}", pct(s["gate1_pass"]["rate"])); m(f"secMH{k}", pct(s["security_medhigh_finding"]["rate"]))
    cost_ratio = S["by_model"]["mistral-large-4"]["cost_usd"] / S["by_model"]["gpt-6-luna"]["cost_usd"]
    m("costRatioMistralLuna", f"{cost_ratio:.0f}")

    # ---- functional funnel
    funnel = {}
    for lang in LANGS + ["all"]:
        sel = [r for r in rows if lang == "all" or r["language"] == lang]
        funnel[lang] = {
            "planned": len(sel),
            "gate1": sum(r["gate1_status"] == "pass" for r in sel),
            "oracle": sum(r["gate1_status"] == "pass" and r["oracle_status"] == "approved" for r in sel),
            "adapter": sum(r["gate1_status"] == "pass" and r["oracle_status"] == "approved" and r["adapter_status"] == "approved" for r in sel),
            "executed": sum(r["functional_status"] in ("passed", "failed", "error", "timeout") for r in sel),
            "passed": sum(r["functional_status"] == "passed" for r in sel),
        }
    F = funnel["all"]
    m("nFuncExecuted", F["executed"]); m("nFuncPassed", F["passed"]); m("nFuncOracleOk", F["oracle"])
    m("pFuncCoverage", pct(F["executed"] / F["planned"])); m("pFuncCoverageGOne", pct(F["executed"] / F["gate1"]))
    m("nFuncTasks", len({r["task_id"] for r in rows if r["functional_status"] == "passed"}))
    fx = [r for r in rows if r["functional_status"] in ("passed", "failed", "error", "timeout")]
    m("nFuncFailed", sum(r["functional_status"] == "failed" for r in fx))
    from_eval = __import__("importlib").import_module
    import sys
    sys.path.insert(0, str(ROOT / "evaluation"))
    from evaluate import wilson
    lo, hi = wilson(F["passed"], F["executed"])
    m("ciFuncPass", f"[{100 * lo:.1f}, 100.0]" if hi == 1.0 else f"[{100 * lo:.1f}, {100 * hi:.1f}]")

    # ---- oracle review
    def reason(note):
        n = note.lower()
        if "wrong expected" in n: return "wrong expected values"
        if "different task" in n: return "test for a different task"
        if "inconsistent" in n or "contradicts" in n: return "self-inconsistent / contradicts task"
        if any(w in n for w in ("server", "network", "libpqxx", "downloads")): return "needs network or external service"
        return "tests unspecified behaviour"
    o_by = defaultdict(Counter)
    for key, v in orev.items():
        lang = key.split("/")[1]
        o_by[lang]["approved" if v["approved"] else reason(v["notes"])] += 1
    m("nOracleProposed", len(orev)); m("nOracleApproved", sum(v["approved"] for v in orev.values()))
    m("nOracleRejected", sum(not v["approved"] for v in orev.values()))
    m("nOracleWrong", sum(c["wrong expected values"] for c in o_by.values()))
    m("nAdapterReviewed", len(arev)); m("nAdapterApproved", sum(v["approved"] for v in arev.values()))
    m("nAdapterRejected", sum(not v["approved"] for v in arev.values()))
    m("nAdapterPostExec", sum(1 for v in arev.values() if "after execution" in v["notes"]))
    with (GEN / "tab_oracles.tex").open("w", encoding="utf-8") as fh:
        reasons = ["approved", "tests unspecified behaviour", "needs network or external service", "wrong expected values",
                   "self-inconsistent / contradicts task", "test for a different task"]
        fh.write("\\begin{tabular}{@{}l" + "r" * 5 + "@{}}\n\\toprule\n")
        fh.write("Outcome & " + " & ".join(LANG_NAME[l] for l in LANGS) + " & All \\\\\n\\midrule\n")
        for rs in reasons:
            vals = [o_by[l][rs] for l in LANGS]
            fh.write(f"{rs[0].upper() + rs[1:]} & " + " & ".join(str(v) for v in vals) + f" & {sum(vals)} \\\\\n")
            if rs == "approved":
                fh.write("\\midrule\n\\multicolumn{6}{@{}l}{\\emph{Rejected because the test \\dots}} \\\\\n")
        tot = [sum(o_by[l].values()) for l in LANGS]
        fh.write("\\midrule\nProposed references & " + " & ".join(str(t) for t in tot) + f" & {sum(tot)} \\\\\n\\bottomrule\n\\end{{tabular}}\n")

    # ---- Table: models, generation and cost
    with (GEN / "tab_models.tex").open("w", encoding="utf-8") as fh:
        fh.write("\\begin{tabular}{@{}lrrrrr@{}}\n\\toprule\n")
        fh.write("Model & Artifacts & Trunc. & Failed & Cost (USD) & Output tokens \\\\\n\\midrule\n")
        num = lambda n: f"{n:,}".replace(",", "{,}")
        for mod in MODELS:
            s = S["by_model"][mod]
            fh.write(f"{MODEL_NAME[mod]} & {s['artifacts']}/160 & {s['truncated']} & {s['failed_generation']} & "
                     f"{s['cost_usd']:.3f} & {num(s['output_tokens'])} \\\\\n")
        fh.write(f"\\midrule\nAll & {O['artifacts']}/800 & {O['truncated']} & {O['failed_generation']} & "
                 f"{O['cost_usd']:.3f} & {num(O['output_tokens'])} \\\\\n")
        fh.write("\\bottomrule\n\\end{tabular}\n")
    with (GEN / "tab_modelids.tex").open("w", encoding="utf-8") as fh:
        fh.write("\\begin{tabular}{@{}ll@{}}\n\\toprule\nModel & OpenRouter identifier \\\\\n\\midrule\n")
        for mc in cfg["models"]:
            fh.write(f"{MODEL_NAME[mc['label']]} & \\texttt{{{mc['model_id']}}} \\\\\n")
        fh.write("\\bottomrule\n\\end{tabular}\n")

    # ---- Table: gate 1 by model x language
    def cell(r):
        return f"{100 * r['rate']:.1f}" if r["n"] else "--"
    with (GEN / "tab_gate1.tex").open("w", encoding="utf-8") as fh:
        fh.write("\\begin{tabular}{@{}lrrrrr@{}}\n\\toprule\n")
        fh.write("Model & " + " & ".join(LANG_NAME[l] for l in LANGS) + " & All [95\\% CI] \\\\\n\\midrule\n")
        for mod in MODELS:
            s = S["by_model"][mod]["gate1_pass"]
            fh.write(f"{MODEL_NAME[mod]} & " + " & ".join(cell(S["by_model_language"][mod][l]["gate1_pass"]) for l in LANGS)
                     + f" & {100 * s['rate']:.1f} [{100 * s['ci95'][0]:.1f}, {100 * s['ci95'][1]:.1f}] \\\\\n")
        fh.write("\\midrule\nAll & " + " & ".join(cell(S["by_language"][l]["gate1_pass"]) for l in LANGS)
                 + f" & {100 * O['gate1_pass']['rate']:.1f} [{100 * O['gate1_pass']['ci95'][0]:.1f}, {100 * O['gate1_pass']['ci95'][1]:.1f}] \\\\\n")
        fh.write("\\bottomrule\n\\end{tabular}\n")
    # gate 1 outcome breakdown by language (all models)
    with (GEN / "tab_gate1_outcomes.tex").open("w", encoding="utf-8") as fh:
        cats = [("pass", "Pass"), ("fail_integrity", "Incomplete/malformed"), ("fail_syntax", "Syntax error"),
                ("fail_compile", "Compile error"), ("blocked_dependency", "Blocked (missing library)"), ("no_artifact", "No artifact")]
        fh.write("\\begin{tabular}{@{}l" + "r" * 5 + "@{}}\n\\toprule\nGate 1 outcome & " + " & ".join(LANG_NAME[l] for l in LANGS) + " & All \\\\\n\\midrule\n")
        for c, name in cats:
            vals = [S["by_language"][l]["gate1_counts"].get(c, 0) for l in LANGS]
            fh.write(f"{name} & " + " & ".join(map(str, vals)) + f" & {sum(vals)} \\\\\n")
        fh.write("\\bottomrule\n\\end{tabular}\n")
    m("nTruncatedGOne", sum(1 for r in rows if r["generation_status"] == "generated_truncated" and r["gate1_status"] == "fail_integrity"))
    blocked_deps = Counter(d for r in rows if r["gate1_status"] == "blocked_dependency" for d in r["missing_dependencies"].split(";") if d)
    esc = lambda s: s.replace("_", "\\_")
    m("blockedDeps", ", ".join("\\texttt{" + esc(d) + "}" for d, _ in blocked_deps.most_common()))

    # ---- Table: functional funnel
    with (GEN / "tab_functional.tex").open("w", encoding="utf-8") as fh:
        fh.write("\\begin{tabular}{@{}l" + "r" * 5 + "@{}}\n\\toprule\nStage & " + " & ".join(LANG_NAME[l] for l in LANGS) + " & All \\\\\n\\midrule\n")
        for key, name in [("planned", "Planned combinations"), ("gate1", "Passed gate 1"), ("oracle", "\\quad with an approved oracle"),
                          ("adapter", "\\quad and an approved adapter"), ("executed", "Executed in sandbox"), ("passed", "Passed all checks")]:
            fh.write(f"{name} & " + " & ".join(str(funnel[l][key]) for l in LANGS) + f" & {funnel['all'][key]} \\\\\n")
        fh.write("\\bottomrule\n\\end{tabular}\n")
    with (GEN / "tab_functional_models.tex").open("w", encoding="utf-8") as fh:
        fh.write("\\begin{tabular}{@{}l" + "r" * 5 + "@{}}\n\\toprule\nModel & " + " & ".join(LANG_NAME[l] for l in LANGS) + " & All \\\\\n\\midrule\n")
        for mod in MODELS:
            vals = []
            for l in LANGS:
                s = S["by_model_language"][mod][l]
                vals.append(f"{s['functional_pass_of_executed']['k']}/{s['functional_executed']}")
            s = S["by_model"][mod]
            fh.write(f"{MODEL_NAME[mod]} & " + " & ".join(vals) + f" & {s['functional_pass_of_executed']['k']}/{s['functional_executed']} \\\\\n")
        fh.write("\\bottomrule\n\\end{tabular}\n")

    # ---- Table: security (potential findings)
    with (GEN / "tab_security.tex").open("w", encoding="utf-8") as fh:
        fh.write("\\begin{tabular}{@{}lrrrrrrr@{}}\n\\toprule\n")
        fh.write("Model & $n$ & $\\geq$1 med./high & $\\geq$1 high & /KLOC & Semgrep & Bandit & Flawf. \\\\\n\\midrule\n")
        for mod in MODELS:
            s = S["by_model"][mod]; t = s["findings_by_tool"]; mh = s["security_medhigh_finding"]; hh = s["security_high_finding"]
            fh.write(f"{MODEL_NAME[mod]} & {s['scanned_gate1_pass']} & {100 * mh['rate']:.1f} [{100 * mh['ci95'][0]:.0f}, {100 * mh['ci95'][1]:.0f}] & "
                     f"{100 * hh['rate']:.1f} & {s['security_medhigh_per_kloc']:.1f} & {t['semgrep']} & {t['bandit']} & {t['flawfinder']} \\\\\n")
        s = O; t = s["findings_by_tool"]; mh = s["security_medhigh_finding"]; hh = s["security_high_finding"]
        fh.write(f"\\midrule\nAll & {s['scanned_gate1_pass']} & {100 * mh['rate']:.1f} [{100 * mh['ci95'][0]:.0f}, {100 * mh['ci95'][1]:.0f}] & "
                 f"{100 * hh['rate']:.1f} & {s['security_medhigh_per_kloc']:.1f} & {t['semgrep']} & {t['bandit']} & {t['flawfinder']} \\\\\n")
        fh.write("\\bottomrule\n\\end{tabular}\n")
    with (GEN / "tab_security_lang.tex").open("w", encoding="utf-8") as fh:
        fh.write("\\begin{tabular}{@{}lrrrr@{}}\n\\toprule\nModel & " + " & ".join(LANG_NAME[l] for l in LANGS) + " \\\\\n\\midrule\n")
        for mod in MODELS:
            fh.write(f"{MODEL_NAME[mod]} & " + " & ".join(cell(S["by_model_language"][mod][l]["security_medhigh_finding"]) for l in LANGS) + " \\\\\n")
        fh.write("\\midrule\nAll & " + " & ".join(cell(S["by_language"][l]["security_medhigh_finding"]) for l in LANGS) + " \\\\\n")
        fh.write("\\bottomrule\n\\end{tabular}\n")
    top = S["top_cwes_artifacts"][:8]
    m("topCwes", ", ".join(f"{c} ({n})" for c, n in top))
    m("nFindingsTotal", scan_meta["findings"]); m("nSemgrepErrors", scan_meta["semgrep_errors"])
    for tool in ("semgrep", "bandit", "flawfinder", "cppcheck"):
        m(f"nFind{tool.capitalize()}", scan_meta["findings_by_tool"].get(tool, 0))
    g1f = [f for f in findings if f["gate1_status"] == "pass"]
    m("nFindGOne", len(g1f)); m("nFindGOneSec", sum(f["kind"] == "security" for f in g1f))
    m("nFindGOneLow", sum(f["kind"] == "security" and f["severity"] == "low" for f in g1f))
    m("nFindGOneMedHigh", sum(f["kind"] == "security" and f["severity"] in ("medium", "high") for f in g1f))
    m("nScannedArtifacts", scan_meta["artifacts_scanned"])
    vers = {l.split()[0]: " ".join(l.split()[1:]) for l in scan_meta["versions"] if not re.match(r"^[0-9a-f]{64}", l)}
    m("verSemgrep", vers.get("semgrep", "?")); m("verBandit", vers.get("bandit", "?").replace("bandit ", ""))
    m("verFlawfinder", vers.get("flawfinder", "?")); m("verLizard", vers.get("lizard", "?"))
    m("verCppcheck", vers.get("cppcheck", "?").replace("Cppcheck ", ""))

    # ---- Table: quality by model (v2: CCD added; Cppcheck density over C/C++ NLOC only)
    q = {(r["model"], r["language"]): r for r in load_csv(CONS / "quality_by_model_language.csv")}
    with (GEN / "tab_quality.tex").open("w", encoding="utf-8") as fh:
        fh.write("\\begin{tabular}{@{}lrrrrr@{}}\n\\toprule\nModel & NLOC & CCN & CCD & Fn.\\ CCN$>$10 & Cppcheck/KLOC \\\\\n\\midrule\n")
        for mod in MODELS + ["all"]:
            r = q[(mod, "all")]
            if mod == "all":
                fh.write("\\midrule\n")
            fh.write(f"{MODEL_NAME.get(mod, 'All')} & {float(r['mean_nloc']):.1f} & {float(r['mean_avg_ccn']):.2f} & "
                     f"{float(r['mean_ccd']):.3f} & {r['functions_ccn_gt10']} & {float(r['cppcheck_per_kloc_c_cpp']):.2f} \\\\\n")
        fh.write("\\bottomrule\n\\end{tabular}\n")
    # ---- Table: quality by model x language (mean CCN | mean CCD; Cppcheck/KLOC for C, C++)
    with (GEN / "tab_quality_lang.tex").open("w", encoding="utf-8") as fh:
        fh.write("\\begin{tabular}{@{}l" + "r" * 6 + "@{}}\n\\toprule\n")
        fh.write(" & \\multicolumn{4}{c}{Mean CCN / mean CCD} & \\multicolumn{2}{c}{Cppcheck/KLOC} \\\\\n"
                 "\\cmidrule(lr){2-5}\\cmidrule(l){6-7}\nModel & Python & Java & C & C++ & C & C++ \\\\\n\\midrule\n")
        for mod in MODELS + ["all"]:
            if mod == "all":
                fh.write("\\midrule\n")
            cells = [f"{float(q[(mod, l)]['mean_avg_ccn']):.2f}/{float(q[(mod, l)]['mean_ccd']):.2f}" for l in LANGS]
            cells += [f"{float(q[(mod, l)]['cppcheck_per_kloc_c_cpp']):.1f}" for l in ("c", "cpp")]
            fh.write(f"{MODEL_NAME.get(mod, 'All')} & " + " & ".join(cells) + " \\\\\n")
        fh.write("\\bottomrule\n\\end{tabular}\n")
    m("cppcheckMistral", f"{float(q[('mistral-large-4', 'all')]['cppcheck_per_kloc_c_cpp']):.1f}")
    m("cppcheckLuna", f"{float(q[('gpt-6-luna', 'all')]['cppcheck_per_kloc_c_cpp']):.1f}")
    m("cppcheckAll", f"{float(q[('all', 'all')]['cppcheck_per_kloc_c_cpp']):.1f}")
    m("ccdAll", f"{float(q[('all', 'all')]['mean_ccd']):.3f}")
    m("nNoFunctions", int(q[("all", "all")]["gate1_pass_artifacts"]) - int(q[("all", "all")]["artifacts_with_functions"]))

    # ---- Table: paired gate-1 comparisons (exact McNemar, Holm) from paired_gate1.csv
    pairs = load_csv(CONS / "paired_gate1.csv")
    with (GEN / "tab_paired_gate1.tex").open("w", encoding="utf-8") as fh:
        fh.write("\\begin{tabular}{@{}llrrrrr@{}}\n\\toprule\n"
                 "A & B & A only & B only & $\\Delta$ (pp) [95\\% CI] & $p$ & $p_\\mathrm{Holm}$ \\\\\n\\midrule\n")
        short = {"claude-haiku-5-5": "Haiku", "gpt-6-luna": "Luna", "gemini-3-8-flash": "Gemini",
                 "mistral-large-4": "Mistral", "qwen3-8-flash": "Qwen"}
        def pfmt(x):
            x = float(x)
            if x < 1e-3:
                mant, exp = f"{x:.1e}".split("e")
                return f"${mant}\\times10^{{{int(exp)}}}$"
            return f"{x:.3f}"
        for p in sorted(pairs, key=lambda p: float(p["p_exact_mcnemar"])):
            sig = "$^{*}$" if p["significant_holm_0.05"] == "True" else ""
            fh.write(f"{short[p['model_a']]} & {short[p['model_b']]} & {p['a_only']} & {p['b_only']} & "
                     f"{100 * float(p['diff_a_minus_b']):+.1f} [{100 * float(p['diff_ci_lo']):+.1f}, {100 * float(p['diff_ci_hi']):+.1f}] & "
                     f"{pfmt(p['p_exact_mcnemar'])} & {pfmt(p['p_holm'])}{sig} \\\\\n")
        fh.write("\\bottomrule\n\\end{tabular}\n")
    m("nPairsSig", sum(p["significant_holm_0.05"] == "True" for p in pairs)); m("nPairs", len(pairs))
    m("nPairCells", pairs[0]["cells"])
    pl = {(p["model_a"], p["model_b"]): p for p in pairs}
    m("pHolmLunaGemini", f"{float(pl[('gemini-3-8-flash', 'gpt-6-luna')]['p_holm']):.2f}")
    m("pHolmLunaMistral", f"{float(pl[('gpt-6-luna', 'mistral-large-4')]['p_holm']):.3f}")
    m("pHolmGeminiMistral", f"{float(pl[('gemini-3-8-flash', 'mistral-large-4')]['p_holm']):.2f}")
    m("pRawGeminiMistral", f"{float(pl[('gemini-3-8-flash', 'mistral-large-4')]['p_exact_mcnemar']):.3f}")
    m("pHolmHaikuMistral", f"{float(pl[('claude-haiku-5-5', 'mistral-large-4')]['p_holm']):.2f}")

    # ---- Table: triage (AI-proposed)
    labs = ["likely_true_positive", "needs_human_context", "likely_false_positive"]
    tri = defaultdict(Counter)
    for t in triage:
        tri[t["tool"]][t["ai_proposed_label"]] += 1
    with (GEN / "tab_triage.tex").open("w", encoding="utf-8") as fh:
        fh.write("\\begin{tabular}{@{}lrrrr@{}}\n\\toprule\nTool & Likely TP & Needs context & Likely FP & Sampled \\\\\n\\midrule\n")
        for tool in ("semgrep", "bandit", "flawfinder"):
            vals = [tri[tool][l] for l in labs]
            fh.write(f"{tool.capitalize()} & " + " & ".join(map(str, vals)) + f" & {sum(vals)} \\\\\n")
        tot = [sum(tri[t][l] for t in tri) for l in labs]
        fh.write("\\midrule\nAll & " + " & ".join(map(str, tot)) + f" & {sum(tot)} \\\\\n\\bottomrule\n\\end{{tabular}}\n")
    m("nTriage", len(triage)); m("nTriageTP", tot[0]); m("nTriageNC", tot[1]); m("nTriageFP", tot[2])
    m("nTriageHigh", sum(t["severity"] == "high" for t in triage)); m("nTriageMedium", sum(t["severity"] == "medium" for t in triage))
    m("nFlawfinderFP", tri["flawfinder"]["likely_false_positive"]); m("nFlawfinderSampled", sum(tri["flawfinder"].values()))

    # ---- Figures
    def save(fig, name):
        fig.savefig(FIG / f"{name}.pdf", bbox_inches="tight", facecolor="white")
        if PREVIEW:
            fig.savefig(PREVIEW / f"{name}.png", bbox_inches="tight", facecolor="white", dpi=200)
        plt.close(fig)

    # Fig: gate 1 pass rate by language, grouped by model, with Wilson CIs
    fig, ax = plt.subplots(figsize=(3.45, 1.95))
    w = 0.15
    for i, mod in enumerate(MODELS):
        xs, ys, lo_e, hi_e = [], [], [], []
        for j, l in enumerate(LANGS):
            r = S["by_model_language"][mod][l]["gate1_pass"]
            xs.append(j + (i - 2) * w); ys.append(100 * r["rate"])
            lo_e.append(100 * (r["rate"] - r["ci95"][0])); hi_e.append(100 * (r["ci95"][1] - r["rate"]))
        ax.bar(xs, ys, width=w - 0.02, color=COLOR[mod], hatch=HATCH[mod], edgecolor="white", linewidth=0.4, label=MODEL_NAME[mod])
        ax.errorbar(xs, ys, yerr=[lo_e, hi_e], fmt="none", ecolor=INK2, elinewidth=0.5, capsize=0)
    ax.set_xticks(range(4)); ax.set_xticklabels([LANG_NAME[l] for l in LANGS])
    ax.set_ylim(0, 105); ax.set_ylabel("Gate 1 pass (%)")
    ax.yaxis.grid(True, color="#e6e5e1", linewidth=0.5); ax.set_axisbelow(True)
    ax.legend(ncol=3, fontsize=6, frameon=False, loc="upper center", bbox_to_anchor=(0.5, 1.33), handlelength=1.2, columnspacing=0.8)
    save(fig, "fig_gate1")

    # Fig: outcome composition per model (stacked horizontal bars)
    cats = [("pass", "Pass", "#2a78d6", ""), ("fail_integrity", "Incomplete/malformed", "#86b6ef", "//"),
            ("fail_syntax+fail_compile", "Syntax/compile error", "#52514e", ""), ("blocked_dependency", "Blocked (library)", "#c3c2b7", ".."),
            ("no_artifact", "No artifact", "#e34948", "xx")]
    fig, ax = plt.subplots(figsize=(3.45, 1.6))
    for i, mod in enumerate(MODELS):
        gc = S["by_model"][mod]["gate1_counts"]; left = 0
        for key, name, col, hat in cats:
            v = sum(gc.get(k, 0) for k in key.split("+"))
            ax.barh(i, v, left=left, color=col, hatch=hat, edgecolor="white", linewidth=0.6, height=0.62, label=name if i == 0 else None)
            if v >= 9:
                ax.text(left + v / 2, i, str(v), ha="center", va="center", fontsize=6,
                        color="white" if col in ("#2a78d6", "#52514e", "#e34948") else INK)
            left += v
    ax.set_yticks(range(len(MODELS))); ax.set_yticklabels([MODEL_NAME[x] for x in MODELS]); ax.invert_yaxis()
    ax.set_xlim(0, 160); ax.set_xlabel("Combinations (of 160 per model)")
    ax.legend(ncol=3, fontsize=5.8, frameon=False, loc="upper center", bbox_to_anchor=(0.42, 1.36), handlelength=1.2, columnspacing=0.7)
    save(fig, "fig_outcomes")

    # Fig: cost vs gate 1 pass (one scale: cost on log x, pass on y)
    fig, ax = plt.subplots(figsize=(3.45, 1.85))
    for mod in MODELS:
        s = S["by_model"][mod]; r = s["gate1_pass"]
        ax.errorbar([s["cost_usd"]], [100 * r["rate"]], yerr=[[100 * (r["rate"] - r["ci95"][0])], [100 * (r["ci95"][1] - r["rate"])]],
                    fmt="o", color=COLOR[mod], markersize=5, markeredgecolor="white", markeredgewidth=0.8, elinewidth=0.6, capsize=0)
        ax.annotate(MODEL_NAME[mod], (s["cost_usd"], 100 * r["rate"]), textcoords="offset points", xytext=(5, -2), fontsize=6, color=INK)
    ax.set_xscale("log"); ax.set_xlim(0.07, 6); ax.set_ylim(50, 95)
    from matplotlib.ticker import FixedLocator, FixedFormatter, NullLocator
    ticks = [0.1, 0.2, 0.5, 1, 2, 5]
    ax.xaxis.set_major_locator(FixedLocator(ticks)); ax.xaxis.set_major_formatter(FixedFormatter([f"${t:g}" for t in ticks]))
    ax.xaxis.set_minor_locator(NullLocator())
    ax.set_xlabel("Generation cost for 160 combinations (USD, log scale)"); ax.set_ylabel("Gate 1 pass (%)")
    ax.yaxis.grid(True, color="#e6e5e1", linewidth=0.5); ax.set_axisbelow(True)
    save(fig, "fig_cost")

    # Fig: potential medium/high finding rate by language and model
    fig, ax = plt.subplots(figsize=(3.45, 1.85))
    for i, mod in enumerate(MODELS):
        xs = [j + (i - 2) * w for j in range(4)]
        ys = [100 * S["by_model_language"][mod][l]["security_medhigh_finding"]["rate"] for l in LANGS]
        ax.bar(xs, ys, width=w - 0.02, color=COLOR[mod], hatch=HATCH[mod], edgecolor="white", linewidth=0.4, label=MODEL_NAME[mod])
    for j, l in enumerate(LANGS):
        ax.text(j, 100 * S["by_language"][l]["security_medhigh_finding"]["rate"] + 14 if l == "c" else
                100 * max(S["by_model_language"][x][l]["security_medhigh_finding"]["rate"] for x in MODELS) + 4,
                f"all: {100 * S['by_language'][l]['security_medhigh_finding']['rate']:.0f}%", ha="center", fontsize=6, color=INK2)
    ax.set_xticks(range(4)); ax.set_xticklabels([LANG_NAME[l] for l in LANGS]); ax.set_ylim(0, 80)
    ax.set_ylabel("Artifacts with $\\geq$1 med./high\npotential finding (%)")
    ax.yaxis.grid(True, color="#e6e5e1", linewidth=0.5); ax.set_axisbelow(True)
    ax.legend(ncol=3, fontsize=6, frameon=False, loc="upper center", bbox_to_anchor=(0.5, 1.33), handlelength=1.2, columnspacing=0.8)
    save(fig, "fig_security")

    # ---- Secondary: cohort comparison with the base study's published outputs (C/C++/Python)
    BC = RES / "base-40-comparison-v1"
    cc = {(r["analysis"], r["language"], r["measure"]): r for r in load_csv(BC / "cohort_comparison.csv")}
    cj = json.loads((BC / "comparison.json").read_text(encoding="utf-8"))

    def dpp(r):  # unrounded difference from comparison.json (the CSV is rounded to 4 dp)
        d = cj["analyses"][r["analysis"]][r["language"]][f"diff_{r['measure']}"]
        return f"${100 * d['diff']:+.1f}$~pp [${100 * d['lo']:+.1f}$, ${100 * d['hi']:+.1f}$]"
    with (GEN / "tab_base_cohort.tex").open("w", encoding="utf-8") as fh:
        fh.write("\\begin{tabular}{@{}lrrrr@{}}\n\\toprule\n"
                 "& Gate 1 / planned & Gate 1 / artifact & $\\geq$1 med./high & mean NLOC \\\\\n\\midrule\n")
        for lang, nm in (("c", "C"), ("cpp", "C++"), ("python", "Python"), ("all", "All")):
            if lang == "all":
                fh.write("\\midrule\n")
            cells = [f"{100 * float(cc[('primary', lang, x)]['ours_value']):.1f} / {100 * float(cc[('primary', lang, x)]['base_value']):.1f}"
                     for x in ("gate1_pass_planned", "gate1_pass_of_artifacts", "sec_medhigh_of_pass")]
            cells.append(f"{float(cc[('primary', lang, 'mean_nloc')]['ours_value']):.1f} / {float(cc[('primary', lang, 'mean_nloc')]['base_value']):.1f}")
            fh.write(f"{nm} & " + " & ".join(cells) + " \\\\\n")
        fh.write("\\bottomrule\n\\end{tabular}\n")
    m("baseNFiles", cj["base_files"]); m("baseNIdentical", cj["base_identical"]); m("baseNEscape", cj["base_escape_pattern"])
    m("baseNEdited", cj["base_files"] - cj["base_identical"])
    pa = cj["analyses"]["primary"]["all"]
    m("baseNBlocked", pa["base"]["gate1_counts"]["blocked_dependency"]); m("oursNBlockedCCpp", pa["ours"]["gate1_counts"]["blocked_dependency"])
    m("oursNNoArtifactCCpp", pa["ours"]["gate1_counts"]["no_artifact"]); m("oursNIntegrityCCpp", pa["ours"]["gate1_counts"]["fail_integrity"])
    m("cohDiffPlanned", dpp(cc[("primary", "all", "gate1_pass_planned")]))
    m("cohDiffArtifact", dpp(cc[("primary", "all", "gate1_pass_of_artifacts")]))
    m("cohDiffMedHigh", dpp(cc[("primary", "all", "sec_medhigh_of_pass")]))
    m("cohDiffMedHighPy", dpp(cc[("primary", "python", "sec_medhigh_of_pass")]))
    for tag, an in (("SOne", "S1_identical"), ("STwo", "S2_no_escape"), ("SThree", "S3_blocked_excl"), ("SFour", "S4_symmetric_integrity")):
        m(f"cohDiffArtifact{tag}", dpp(cc[(an, "all", "gate1_pass_of_artifacts")]))
    m("cohDiffMedHighSOne", dpp(cc[("S1_identical", "all", "sec_medhigh_of_pass")]))
    m("cohDiffMedHighSTwo", dpp(cc[("S2_no_escape", "all", "sec_medhigh_of_pass")]))
    pc = cj["analyses"]["primary"]["paired_cells"]["all"]
    m("cohCells", pc["cells"]); m("cohCellsBaseHigher", pc["base_higher"]); m("cohCellsOursHigher", pc["ours_higher"])
    m("cohCellsEqual", pc["equal"])
    mant, ex = f"{pc['p']:.1e}".split("e")
    m("cohWilcoxonP", f"${mant}\\times10^{{{int(ex)}}}$")
    m("cohNlocOurs", f"{float(cc[('primary', 'all', 'mean_nloc')]['ours_value']):.1f}")
    m("cohNlocBase", f"{float(cc[('primary', 'all', 'mean_nloc')]['base_value']):.1f}")
    m("cohCppcheckOurs", f"{float(cc[('primary', 'all', 'cppcheck_per_kloc_c_cpp')]['ours_value']):.1f}")
    m("cohCppcheckBase", f"{float(cc[('primary', 'all', 'cppcheck_per_kloc_c_cpp')]['base_value']):.1f}")

    # ---- Supplementary: pilot-10 disjoint-task check (reported separately, never pooled)
    pj = json.loads((RES / "pilot-10-summary-v1/summary.json").read_text(encoding="utf-8"))
    po = pj["overall"]
    m("pilotPlanned", po["planned"]); m("pilotGOnePass", pct(po["gate1_pass"]["rate"]))
    m("pilotGOnePassArtifact", pct(po["gate1_pass_of_artifacts"]["rate"]))
    for l, tag in (("python", "Py"), ("java", "Java"), ("c", "C"), ("cpp", "Cpp")):
        m(f"pilotGOne{tag}", pct(pj["by_language"][l]["gate1_pass"]["rate"]))
    m("pilotMedHighC", pct(pj["by_language"]["c"]["sec_medhigh"]["rate"]))
    m("pilotMedHigh", pct(po["sec_medhigh"]["rate"]))

    with (GEN / "numbers.tex").open("w", encoding="utf-8") as fh:
        fh.write("% Generated by paper/scripts/make_assets.py from evaluation/results (do not edit by hand)\n")
        for k, v in sorted(macros.items()):
            fh.write(f"\\newcommand{{\\{k}}}{{{v}}}\n")
    print(f"{len(macros)} macros, tables and figures written")


if __name__ == "__main__":
    import sys
    if "--preview" in sys.argv:
        PREVIEW = Path(sys.argv[sys.argv.index("--preview") + 1])
        PREVIEW.mkdir(parents=True, exist_ok=True)
    main()
