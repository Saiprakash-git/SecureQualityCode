"""
select_90_tasks.py
Produces a stratified selection of 90 tasks from the 200-task dataset.
Outputs: task_manifest.json  (the master task list with IDs + metadata)
         run_config_pilot.json (tasks 1-20 of the selection)
         run_config_main.json  (tasks 21-90 of the selection)
"""
import json
from pathlib import Path
from collections import defaultdict

base = Path(r'F:\MajorProject-VII\Repo\SecureQualityCode\code-llm-evaluation-dataset-main\code-llm-evaluation-dataset-main\dataset')

# ── Load all 200 tasks ────────────────────────────────────────────────────────
all_tasks = {}
for folder in ['promptDescription-0XX', 'promptDescription-1XX']:
    for jf in sorted((base / folder).glob('*.json')):
        data = json.loads(jf.read_text(encoding='utf-8'))
        task = data['tasks'][0]
        num = task['task_number']
        all_tasks[num] = {
            'task_number': num,
            'prompt_title': task.get('prompt_title', ''),
            'tags': task.get('tags', []),
            'source': task.get('source', ''),
        }

# ── Classify each task into one of 7 strata ──────────────────────────────────
def classify(task):
    tags_lower = ' '.join(task['tags']).lower()
    title_lower = task['prompt_title'].lower()
    combined = tags_lower + ' ' + title_lower

    if any(x in combined for x in [
        'buffer', 'overflow', 'memory', 'pointer', 'allocation',
        'out-of-bounds', 'use after free', 'cwe-119', 'cwe-125',
        'cwe-787', 'cwe-122', 'dynamic-memory', 'buffer-read',
    ]):
        return 'A_memory_safety'

    if any(x in combined for x in [
        'sql', 'injection', 'command injection', 'cwe-89', 'cwe-78',
        'cwe-20', 'input validation', 'path traversal', 'cwe-22',
        'cwe-23', 'file inclusion', 'upload', 'cwe-434', 'xss',
        'cross-site scripting', 'cwe-79', 'ssrf', 'cwe-918',
        'regex', 'cwe-730', 'jndi', 'xml-rpc', 'nosql', 'ldap',
        'csv injection', 'cwe-93', 'cwe-1236', 'cwe-74',
        'redirect', 'cwe-601', 'url', 'cwe-176',
    ]):
        return 'B_injection_input'

    if any(x in combined for x in [
        'auth', 'login', 'session', 'password', 'privilege',
        'access control', 'cwe-269', 'cwe-250', 'cwe-266',
        'cwe-613', 'cwe-384', 'cwe-639', 'cwe-285',
        'privilege escalation', 'insecure direct object',
    ]):
        return 'C_auth_access'

    if any(x in combined for x in [
        'crypt', 'rsa', 'ssl', 'tls', 'hash', 'aes', 'cipher',
        'random', 'cwe-327', 'cwe-330', 'cwe-295', 'cwe-297',
        'cwe-321', 'cwe-759', 'cwe-916', 'cwe-780',
        'oaep', 'hashing', 'token', 'encryption',
    ]):
        return 'D_crypto_random'

    if any(x in combined for x in [
        'concurren', 'locking', 'thread', 'race', 'socket',
        'network', 'cwe-667', 'cwe-412', 'cwe-820', 'cors',
        'cwe-346', 'resource', 'denial', 'cwe-400', 'cwe-405',
        'file permission', 'cwe-732', 'cwe-664',
        'error handling', 'cwe-209', 'information disclosure',
    ]):
        return 'E_concurrency_network'

    if any(x in combined for x in [
        'algorithm', 'mathematics', 'sorting', 'search', 'dynamic programming',
        'graph', 'tree', 'recursion', 'arrays', 'lists', 'data structures',
        'greedy', 'two pointers', 'palindrome', 'binary search',
    ]):
        return 'F_algorithms'

    # remaining → quality / maintainability / strings / misc
    return 'G_quality_strings'

# Classify all
classified = defaultdict(list)
for num, task in sorted(all_tasks.items()):
    stratum = classify(task)
    classified[stratum].append(num)

print('=== STRATUM SIZES ===')
for s, ids in sorted(classified.items()):
    print(f'  {s}: {len(ids)} tasks -> {ids}')

# ── Target allocation for 90 tasks ───────────────────────────────────────────
# Pilot = first 20 of the 90; Main = remaining 70
targets = {
    'A_memory_safety':      12,   # low-level security, C/C++ heavy
    'B_injection_input':    20,   # biggest security family
    'C_auth_access':        12,   # auth, sessions, privileges
    'D_crypto_random':      10,   # crypto, hashing, randomness
    'E_concurrency_network': 10,  # concurrency, network, resource
    'F_algorithms':         18,   # correctness baseline
    'G_quality_strings':     8,   # quality, strings, misc
}
assert sum(targets.values()) == 90, f"Total = {sum(targets.values())}"

# ── Select tasks (take first N from each stratum, sorted by task_number) ─────
selected = []
for stratum in sorted(targets.keys()):
    pool = sorted(classified[stratum])
    take = targets[stratum]
    picked = pool[:take]
    if len(picked) < take:
        print(f'[warn] {stratum}: wanted {take}, only have {len(picked)}')
    for rank, orig_id in enumerate(picked, 1):
        selected.append({
            'selection_id': len(selected) + 1,   # our 1..90 ID
            'orig_task_number': orig_id,
            'stratum': stratum,
            'stratum_rank': rank,
            'prompt_title': all_tasks[orig_id]['prompt_title'],
            'tags': all_tasks[orig_id]['tags'],
        })

# Sort selection by selection_id
selected.sort(key=lambda x: x['selection_id'])

print(f'\n=== SELECTED {len(selected)} TASKS ===')
for t in selected:
    print(f'  [{t["selection_id"]:2d}] orig={t["orig_task_number"]:3d}  {t["stratum"]}  "{t["prompt_title"][:55]}"')

# ── Write task_manifest.json ──────────────────────────────────────────────────
out_dir = Path(r'F:\MajorProject-VII\runner')
manifest = {
    '_comment': 'Master task manifest for the 90-task benchmark. selection_id is the stable ID used in configs.',
    'total_selected': len(selected),
    'strata_targets': targets,
    'tasks': selected,
}
(out_dir / 'task_manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
print(f'\n✓ Wrote task_manifest.json')

# ── Pilot IDs (first 20) ──────────────────────────────────────────────────────
# Spread across strata: take 2-3 from each stratum for the pilot
pilot_ids = []
per_stratum_pilot = {
    'A_memory_safety': 2,
    'B_injection_input': 4,
    'C_auth_access': 3,
    'D_crypto_random': 2,
    'E_concurrency_network': 2,
    'F_algorithms': 5,
    'G_quality_strings': 2,
}
assert sum(per_stratum_pilot.values()) == 20

for stratum, count in per_stratum_pilot.items():
    stratum_tasks = [t for t in selected if t['stratum'] == stratum]
    pilot_ids.extend([t['selection_id'] for t in stratum_tasks[:count]])

pilot_ids_sorted = sorted(pilot_ids)
main_ids_sorted = sorted(t['selection_id'] for t in selected if t['selection_id'] not in pilot_ids)

print(f'\nPilot IDs (20): {pilot_ids_sorted}')
print(f'Main IDs  (70): {main_ids_sorted}')

# Map selection_id → orig_task_number
id_to_orig = {t['selection_id']: t['orig_task_number'] for t in selected}

# ── Models config ─────────────────────────────────────────────────────────────
MODELS = [
    {"label": "claude-sonnet-5.5",  "provider": "openrouter", "model_id": "anthropic/claude-sonnet-4-5"},
    {"label": "gpt-6-luna",         "provider": "openrouter", "model_id": "openai/gpt-4o"},
    {"label": "gemini-3.8-flash",   "provider": "openrouter", "model_id": "google/gemini-flash-1.5"},
    {"label": "codestral-2508",     "provider": "openrouter", "model_id": "mistralai/codestral-2501"},
    {"label": "llama-4-scout",      "provider": "openrouter", "model_id": "meta-llama/llama-3.1-8b-instruct"},
]

BASE_CFG = {
    "languages": ["python", "java", "c", "cpp"],
    "temperature": 0.2,
    "top_p": 0.95,
    "max_tokens": 4096,
    "budget_usd_hard_limit": 10.0,
    "retry_max": 3,
    "retry_delay_seconds": 5,
    "sleep_between_requests_seconds": 2,
    "dataset_dirs": [
        "..\\Repo\\SecureQualityCode\\code-llm-evaluation-dataset-main\\code-llm-evaluation-dataset-main\\dataset\\promptDescription-0XX",
        "..\\Repo\\SecureQualityCode\\code-llm-evaluation-dataset-main\\code-llm-evaluation-dataset-main\\dataset\\promptDescription-1XX",
    ],
}

# ── Write pilot config ────────────────────────────────────────────────────────
pilot_orig_ids = [id_to_orig[sid] for sid in pilot_ids_sorted]
pilot_cfg = {
    **BASE_CFG,
    "run_id": "pilot-90task",
    "description": "Pilot: 20 of 90 selected tasks across all 5 models and 4 languages",
    "phase": "pilot",
    "models": MODELS,
    "task_selection": {
        "mode": "orig_id_list",
        "_comment": "orig_task_number values from task_manifest.json",
        "orig_task_ids": pilot_orig_ids,
        "selection_ids": pilot_ids_sorted,
    },
    "output_dir": "..\\output\\benchmark-90",
}
(out_dir / 'run_config_pilot.json').write_text(json.dumps(pilot_cfg, indent=2), encoding='utf-8')
print(f'✓ Wrote run_config_pilot.json  ({len(pilot_orig_ids)} tasks)')

# ── Write main config ─────────────────────────────────────────────────────────
main_orig_ids = [id_to_orig[sid] for sid in main_ids_sorted]
main_cfg = {
    **BASE_CFG,
    "run_id": "main-90task",
    "description": "Main: remaining 70 of 90 selected tasks across all 5 models and 4 languages",
    "phase": "main",
    "models": MODELS,
    "task_selection": {
        "mode": "orig_id_list",
        "_comment": "orig_task_number values from task_manifest.json",
        "orig_task_ids": main_orig_ids,
        "selection_ids": main_ids_sorted,
    },
    "output_dir": "..\\output\\benchmark-90",
}
(out_dir / 'run_config_main.json').write_text(json.dumps(main_cfg, indent=2), encoding='utf-8')
print(f'✓ Wrote run_config_main.json   ({len(main_orig_ids)} tasks)')
print('\nDone.')
