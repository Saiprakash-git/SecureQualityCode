import argparse
import hashlib
import json
import os
import sys
import threading
from concurrent.futures import ThreadPoolExecutor, as_completed
from contextlib import contextmanager


class FileLockManager:
    """
    Provides one exclusive lock per file path.

    Deadlock avoidance strategy:
      * Each worker acquires at most one file lock at a time and never
        requests a second lock while holding one, so a circular wait cannot form.
      * The internal registry guard is held only long enough to look up or
        create a per-file lock; it is never held while a file lock is awaited.
      * Acquisition uses a timeout so a stalled file cannot block workers forever.
    """

    def __init__(self):
        self._locks = {}
        self._registry_guard = threading.Lock()

    def _get_lock(self, path):
        key = os.path.abspath(path)
        with self._registry_guard:
            lock = self._locks.get(key)
            if lock is None:
                lock = threading.Lock()
                self._locks[key] = lock
            return lock

    @contextmanager
    def exclusive(self, path, timeout=None):
        lock = self._get_lock(path)
        acquired = lock.acquire(timeout=timeout) if timeout is not None else lock.acquire()
        if not acquired:
            raise TimeoutError(f"Timed out waiting for lock on {path}")
        try:
            yield
        finally:
            lock.release()


def process_file(path, chunk_size=1024 * 1024):
    """
    Default file processor: computes size, line/word counts and SHA-256 digest.
    Reads the file in chunks so that large files do not need to fit in memory.
    """
    sha = hashlib.sha256()
    size = 0
    lines = 0
    words = 0
    in_word = False
    with open(path, "rb") as fh:
        while True:
            chunk = fh.read(chunk_size)
            if not chunk:
                break
            size += len(chunk)
            sha.update(chunk)
            lines += chunk.count(b"\n")
            for byte in chunk:
                is_space = byte in b" \t\r\n\f\v"
                if is_space:
                    in_word = False
                elif not in_word:
                    in_word = True
                    words += 1
    return {
        "size_bytes": size,
        "lines": lines,
        "words": words,
        "sha256": sha.hexdigest(),
    }


def _process_with_lock(lock_manager, path, processor, lock_timeout):
    with lock_manager.exclusive(path, timeout=lock_timeout):
        return processor(path)


def list_files(directory):
    if not os.path.isdir(directory):
        raise NotADirectoryError(f"Not a directory: {directory}")
    return sorted(
        os.path.join(directory, name)
        for name in os.listdir(directory)
        if os.path.isfile(os.path.join(directory, name))
    )


def process_directory(directory, max_workers=4, lock_timeout=30.0, processor=process_file):
    """
    Processes every regular file in `directory` concurrently.
    Each file is handled by exactly one thread at a time via FileLockManager.
    Returns a dict mapping file path -> result dict (or {"error": message}).
    """
    if max_workers < 1:
        raise ValueError("max_workers must be at least 1")

    files = list_files(directory)
    lock_manager = FileLockManager()
    results = {}

    with ThreadPoolExecutor(max_workers=max_workers) as executor:
        futures = {
            executor.submit(_process_with_lock, lock_manager, path, processor, lock_timeout): path
            for path in files
        }
        for future in as_completed(futures):
            path = futures[future]
            try:
                results[path] = future.result()
            except Exception as exc:
                results[path] = {"error": f"{type(exc).__name__}: {exc}"}

    return results


def main(argv=None):
    parser = argparse.ArgumentParser(description="Concurrently process files in a directory.")
    parser.add_argument("directory", help="Directory containing files to process")
    parser.add_argument("-w", "--workers", type=int, default=4, help="Number of worker threads")
    parser.add_argument("-t", "--timeout", type=float, default=30.0,
                        help="Seconds to wait for a file lock before failing")
    args = parser.parse_args(argv)

    try:
        results = process_directory(args.directory, args.workers, args.timeout)
    except (NotADirectoryError, ValueError) as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1

    print(json.dumps(results, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    sys.exit(main())