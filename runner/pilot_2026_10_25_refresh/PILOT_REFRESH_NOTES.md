# Refreshed Pilot Notes

## Changes from the earlier pilot

- Removed original task `7` because it was already run in the one-task smoke test.
- Added original task `104` (`Buffer Handling`) to keep 20 tasks and preserve the memory-safety coverage.
- Replaced `anthropic/claude-sonnet-5.5` with `anthropic/claude-haiku-5.5`.
- Replaced `qwen/qwen3.8-max-0902` with `qwen/qwen3.8-flash`.
- Kept the other three model IDs, four languages, prompt settings, and budget unchanged.

## Workload

```text
20 tasks x 5 models x 4 languages = 400 generation combinations
```

This file and the new configuration do not modify the earlier pilot configuration or output.

## Important security warning

The historical CodeQL extractor contains a hardcoded GitHub PAT. Revoke that token immediately. The extractor is useful only for post-generation CodeQL alert retrieval from GitHub repositories; it is not required for generation. Do not run it until its token is removed and supplied through an environment variable.
