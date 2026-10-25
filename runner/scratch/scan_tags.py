import json
from pathlib import Path
from collections import Counter

base = Path(r'F:\MajorProject-VII\Repo\SecureQualityCode\code-llm-evaluation-dataset-main\code-llm-evaluation-dataset-main\dataset')
tags_map = {}
all_tags = []

for folder in ['promptDescription-0XX', 'promptDescription-1XX']:
    for jf in sorted((base / folder).glob('*.json')):
        data = json.loads(jf.read_text(encoding='utf-8'))
        task = data['tasks'][0]
        num = task['task_number']
        tags = task.get('tags', [])
        title = task.get('prompt_title', '')
        source = task.get('source', '')
        tags_map[num] = {'tags': tags, 'title': title, 'source': source}
        all_tags.extend(tags)

print('=== TAG FREQUENCY ===')
for tag, count in Counter(all_tags).most_common():
    print(f'  {tag}: {count}')

print(f'\nTOTAL TASKS: {len(tags_map)}')

print('\n=== SAMPLE TASKS (first 30 with titles and tags) ===')
for num in sorted(tags_map)[:30]:
    t = tags_map[num]
    print(f'  Task {num:3d}: {t["title"][:50]:<52} tags={t["tags"]}')

print('\n=== SAMPLE TASKS 100-130 ===')
for num in sorted(tags_map):
    if 100 <= num <= 130:
        t = tags_map[num]
        print(f'  Task {num:3d}: {t["title"][:50]:<52} tags={t["tags"]}')
