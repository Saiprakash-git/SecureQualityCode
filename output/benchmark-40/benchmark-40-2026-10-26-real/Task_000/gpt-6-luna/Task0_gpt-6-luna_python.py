from __future__ import annotations

import os
import threading
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Callable, Iterable


@dataclass
class ProcessingResult:
    path: Path
    value: Any = None
    error: Exception | None = None


# Shared across processor instances so the same resolved path is serialized
# even when submitted through different ConcurrentFileProcessor objects.
_file_locks: dict[str, list[Any]] = {}
_file_locks_guard = threading.Lock()


class ConcurrentFileProcessor:
    """Process files concurrently while serializing access to each individual file."""

    def __init__(
        self,
        processor: Callable[[Path], Any],
        max_workers: int | None = None,
    ) -> None:
        if not callable(processor):
            raise TypeError("processor must be callable")
        if max_workers is not None and max_workers < 1:
            raise ValueError("max_workers must be at least 1")

        self.processor = processor
        self.max_workers = max_workers

    def _process_one(self, path: Path) -> ProcessingResult:
        key = os.path.normcase(str(path.resolve()))

        with _file_locks_guard:
            entry = _file_locks.get(key)
            if entry is None:
                entry = [threading.Lock(), 0]
                _file_locks[key] = entry
            entry[1] += 1

        file_lock = entry[0]
        try:
            with file_lock:
                return ProcessingResult(path=path, value=self.processor(path))
        except Exception as exc:
            return ProcessingResult(path=path, error=exc)
        finally:
            with _file_locks_guard:
                entry[1] -= 1
                if entry[1] == 0 and _file_locks.get(key) is entry:
                    del _file_locks[key]

    def process_paths(self, paths: Iterable[str | os.PathLike[str]]) -> list[ProcessingResult]:
        normalized_paths = [Path(path) for path in paths]
        with ThreadPoolExecutor(max_workers=self.max_workers) as executor:
            return list(executor.map(self._process_one, normalized_paths))

    def process_directory(
        self,
        directory: str | os.PathLike[str],
        recursive: bool = False,
    ) -> list[ProcessingResult]:
        root = Path(directory)
        if not root.is_dir():
            raise NotADirectoryError(str(root))

        iterator = root.rglob("*") if recursive else root.iterdir()
        paths = sorted((path for path in iterator if path.is_file()), key=lambda p: str(p))
        return self.process_paths(paths)


def process_directory(
    directory: str | os.PathLike[str],
    processor: Callable[[Path], Any],
    max_workers: int | None = None,
    recursive: bool = False,
) -> list[ProcessingResult]:
    """Convenience function for processing all files in a directory."""
    return ConcurrentFileProcessor(processor, max_workers).process_directory(
        directory,
        recursive=recursive,
    )