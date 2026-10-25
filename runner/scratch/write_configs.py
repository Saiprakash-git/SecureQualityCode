"""
write_configs.py
Reads task_manifest.json and writes run_config_pilot.json and run_config_main.json
"""
import json
from pathlib import Path

runner_dir = Path(r'F:\MajorProject-VII\runner')
manifest = json.loads((runner_dir / 'task_manifest.json').read_text(encoding='utf-8'))
selected = manifest['tasks']

# Build selection_id -> orig_task_number map
id_to_orig = {t['selection_id']: t['orig_task_number'] for t in selected}

# Pilot: 2-3 per stratum, totalling 20
per_stratum_pilot = {
    'A_memory_safety':       2,
    'B_injection_input':     4,
    'C_auth_access':         3,
    'D_crypto_random':       2,
    'E_concurrency_network': 2,
    'F_algorithms':          5,
    'G_quality_strings':     2,
}

pilot_sel_ids = []
for stratum, count in per_stratum_pilot.items():
    stratum_tasks = [t for t in selected if t['stratum'] == stratum]
    pilot_sel_ids.extend([t['selection_id'] for t in stratum_tasks[:count]])

pilot_sel_ids = sorted(pilot_sel_ids)
main_sel_ids  = sorted(t['selection_id'] for t in selected if t['selection_id'] not in set(pilot_sel_ids))

pilot_orig_ids = [id_to_orig[sid] for sid in pilot_sel_ids]
main_orig_ids  = [id_to_orig[sid] for sid in main_sel_ids]

print(f'Pilot: {len(pilot_orig_ids)} tasks  sel_ids={pilot_sel_ids}')
print(f'Main:  {len(main_orig_ids)} tasks')

# ── Model matrix ──────────────────────────────────────────────────────────────
MODELS = [
    {
        "label": "claude-sonnet-5.5",
        "provider": "openrouter",
        "model_id": "anthropic/claude-sonnet-4-5",
        "_note": "Update model_id when Claude Sonnet 5.5 is live on OpenRouter"
    },
    {
        "label": "gpt-6-luna",
        "provider": "openrouter",
        "model_id": "openai/gpt-4o",
        "_note": "Update model_id when GPT-6 Luna is live on OpenRouter"
    },
    {
        "label": "gemini-3.8-flash",
        "provider": "openrouter",
        "model_id": "google/gemini-flash-1.5",
        "_note": "Update model_id when Gemini 3.8 Flash is live on OpenRouter"
    },
    {
        "label": "codestral-2508",
        "provider": "openrouter",
        "model_id": "mistralai/codestral-2501",
        "_note": "Update model_id when Codestral 2508 is live on OpenRouter"
    },
    {
        "label": "llama-4-scout",
        "provider": "openrouter",
        "model_id": "meta-llama/llama-3.3-70b-instruct",
        "_note": "Update model_id when Llama 4 Scout is live on OpenRouter"
    },
]

BASE = {
    "languages": ["python", "java", "c", "cpp"],
    "temperature": 0.2,
    "top_p": 0.95,
    "max_tokens": 4096,
    "retry_max": 3,
    "retry_delay_seconds": 5,
    "sleep_between_requests_seconds": 2,
    "dataset_dirs": [
        "..\\Repo\\SecureQualityCode\\code-llm-evaluation-dataset-main\\code-llm-evaluation-dataset-main\\dataset\\promptDescription-0XX",
        "..\\Repo\\SecureQualityCode\\code-llm-evaluation-dataset-main\\code-llm-evaluation-dataset-main\\dataset\\promptDescription-1XX",
    ],
    "output_dir": "..\\output\\benchmark-90",
}

# ── Pilot config ──────────────────────────────────────────────────────────────
pilot_cfg = {
    **BASE,
    "run_id": "pilot-90task",
    "description": "Pilot: 20 of 90 selected tasks x 5 models x 4 languages",
    "phase": "pilot",
    "budget_usd_hard_limit": 8.0,
    "models": MODELS,
    "task_selection": {
        "mode": "orig_id_list",
        "_comment": "orig_task_ids = task_number values from original dataset. selection_ids = our stable 1-90 IDs.",
        "orig_task_ids": pilot_orig_ids,
        "selection_ids": pilot_sel_ids,
    },
}

(runner_dir / 'run_config_pilot.json').write_text(json.dumps(pilot_cfg, indent=2), encoding='utf-8')
print('Written: run_config_pilot.json')

# ── Main config ───────────────────────────────────────────────────────────────
main_cfg = {
    **BASE,
    "run_id": "main-90task",
    "description": "Main: remaining 70 of 90 selected tasks x 5 models x 4 languages",
    "phase": "main",
    "budget_usd_hard_limit": 40.0,
    "models": MODELS,
    "task_selection": {
        "mode": "orig_id_list",
        "_comment": "orig_task_ids = task_number values from original dataset. selection_ids = our stable 1-90 IDs.",
        "orig_task_ids": main_orig_ids,
        "selection_ids": main_sel_ids,
    },
}

(runner_dir / 'run_config_main.json').write_text(json.dumps(main_cfg, indent=2), encoding='utf-8')
print('Written: run_config_main.json')
print('Done.')
