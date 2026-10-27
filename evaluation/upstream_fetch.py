#!/usr/bin/env python3
"""
upstream_fetch.py - Download the original functional tests and the original
model implementations (used only to read the canonical API signatures) for the
benchmark tasks, pinned to fixed commits.

Files land in evaluation/.upstream/<repo>/<path> (git-ignored: the upstream
repositories declare no license, so copies are not committed) with a manifest
of URLs and SHA-256 hashes. The files are data: nothing here executes them.

Usage: python upstream_fetch.py [--tasks 18,23]
"""

import argparse
import hashlib
import json
import re
import sys
import urllib.parse
import urllib.request
from pathlib import Path

EVAL_DIR = Path(__file__).resolve().parent
UPSTREAM_DIR = EVAL_DIR / ".upstream"
BENCHMARK_TASKS = [0, 3, 9, 12, 14, 15, 17, 18, 21, 23, 25, 28, 31, 44, 56, 66, 67, 68, 72, 73,
                   74, 76, 78, 82, 84, 86, 89, 91, 99, 101, 105, 113, 114, 124, 130, 132, 143,
                   145, 148, 149]

# language -> (repository, pinned commit, test path regex, implementation path regex).
# {t} is the task number; (?P<model>) is the original model folder.
REPOS = {
    "python": ("mohsystem/llm-generated-code-python", "8f438f770383783fc9fc62b4068f3f1b70e58e6b",
               r"tests/(?P<model>[^/]+)/Task{t}_[^/]+/[^/]+\.py",
               r"output/(?P<model>[^/]+)/Task{t}_[^/]+\.py"),
    "java": ("mohsystem/llm-generated-code-java", "70dbb6b13099714402cfc0083225faeb59c11067",
             r"src/test/java/(?P<model>[^/]+)/task{t}/[^/]+\.java",
             r"src/main/java/(?P<model>[^/]+)/task{t}/[^/]+\.java"),
    "c": ("mohsystem/llm_generated_code_c", "8e3568277c0f6d68e18df6d530ee4367aa9b7c86",
          r"tests/(?P<model>[^/]+)/Task{t}_[^/]*\.c",
          r"source/output/(?P<model>[^/]+)/c/Task{t}_[^/]+\.c"),
    "cpp": ("mohsystem/llm-generated-code-c-cpp", "0435da599ecae1596822e2773994ffb84f26dec5",
            r"tests/(?P<model>[^/]+)/Task{t}_[^/]*\.cpp",
            r"source/output/(?P<model>[^/]+)/cpp/Task{t}_[^/]+\.cpp"),
}


def fetch(url: str) -> bytes:
    with urllib.request.urlopen(url, timeout=60) as resp:
        return resp.read()


def repo_tree(repo: str, sha: str) -> list[str]:
    data = json.loads(fetch(f"https://api.github.com/repos/{repo}/git/trees/{sha}?recursive=1"))
    if data.get("truncated"):
        raise RuntimeError(f"Tree listing truncated for {repo}@{sha}")
    return [t["path"] for t in data["tree"] if t["type"] == "blob"]


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[1])
    parser.add_argument("--tasks", help="Comma-separated subset of the benchmark tasks.")
    args = parser.parse_args(argv)
    tasks = [int(t) for t in args.tasks.split(",")] if args.tasks else BENCHMARK_TASKS
    manifest = {"repos": {}, "files": []}
    for language, (repo, sha, test_rx, impl_rx) in REPOS.items():
        manifest["repos"][language] = {"repo": repo, "commit": sha}
        tree = repo_tree(repo, sha)
        for task in tasks:
            for role, rx in (("test", test_rx), ("implementation", impl_rx)):
                pattern = re.compile(rx.format(t=task) + "$")
                for path in tree:
                    match = pattern.match(path)
                    if not match:
                        continue
                    url = f"https://raw.githubusercontent.com/{repo}/{sha}/{urllib.parse.quote(path)}"
                    data = fetch(url)
                    dest = UPSTREAM_DIR / repo.split("/")[1] / path
                    dest.parent.mkdir(parents=True, exist_ok=True)
                    dest.write_bytes(data)
                    manifest["files"].append({
                        "language": language, "task_id": task, "role": role,
                        "original_model": match.group("model"), "repo": repo, "commit": sha,
                        "path": path, "local_path": dest.relative_to(EVAL_DIR).as_posix(),
                        "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest(),
                    })
        print(f"{language}: {sum(1 for f in manifest['files'] if f['language'] == language)} files")
    manifest["files"].sort(key=lambda f: (f["language"], f["task_id"], f["role"], f["path"]))
    (UPSTREAM_DIR / "manifest.json").write_text(json.dumps(manifest, indent=1) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
