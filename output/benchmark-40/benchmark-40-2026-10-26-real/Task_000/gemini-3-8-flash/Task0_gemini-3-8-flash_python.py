import os
import shutil
import tempfile
import threading
import hashlib
from concurrent.futures import ThreadPoolExecutor, as_completed
from contextlib import contextmanager
from pathlib import Path
from typing import Callable, Dict, List, Optional, Tuple


class FileLockManager:
    """Thread-safe manager for granular, per-file locking to prevent race conditions."""

    def __init__(self):
        self._locks: Dict[Path, threading.Lock] = {}
        self._master_lock = threading.Lock()

    def get_lock(self, file_path: Path) -> threading.Lock:
        resolved_path = file_path.resolve()
        with self._master_lock:
            if resolved_path not in self._locks:
                self._locks[resolved_path] = threading.Lock()
            return self._locks[resolved_path]

    @contextmanager
    def acquire(self, file_path: Path):
        lock = self.get_lock(file_path)
        lock.acquire()
        try:
            yield
        finally:
            lock.release()


def default_chunk_processor(chunk: bytes) -> bytes:
    """Example chunk transformation: converts text chunks to uppercase."""
    return chunk.upper()


def process_single_file(
    file_path: Path,
    output_dir: Path,
    lock_manager: FileLockManager,
    chunk_size: int = 1024 * 1024,
    transform_fn: Optional[Callable[[bytes], bytes]] = None,
) -> Dict[str, object]:
    """
    Safely processes a single file in chunks using locking to guarantee
    that no other thread modifies or reads it concurrently.
    """
    if transform_fn is None:
        transform_fn = default_chunk_processor

    resolved_path = file_path.resolve()
    dest_path = output_dir / file_path.name
    temp_dest = output_dir / f".tmp_{file_path.name}_{threading.get_ident()}"

    total_bytes = 0
    hasher = hashlib.sha256()

    with lock_manager.acquire(resolved_path):
        if not file_path.exists():
            return {
                "file": str(file_path),
                "status": "error",
                "reason": "File not found",
            }

        try:
            with open(file_path, "rb") as src, open(temp_dest, "wb") as dst:
                while True:
                    chunk = src.read(chunk_size)
                    if not chunk:
                        break
                    total_bytes += len(chunk)
                    hasher.update(chunk)
                    processed_chunk = transform_fn(chunk)
                    dst.write(processed_chunk)

            os.replace(temp_dest, dest_path)
        except Exception as exc:
            if temp_dest.exists():
                temp_dest.unlink()
            return {
                "file": str(file_path),
                "status": "failed",
                "error": str(exc),
            }

    return {
        "file": str(file_path),
        "output_file": str(dest_path),
        "status": "success",
        "bytes_processed": total_bytes,
        "sha256": hasher.hexdigest(),
        "processed_by_thread": threading.current_thread().name,
    }


def process_directory_concurrently(
    input_dir: str,
    output_dir: str,
    max_workers: int = 4,
    chunk_size: int = 1024 * 1024,
    transform_fn: Optional[Callable[[bytes], bytes]] = None,
) -> List[Dict[str, object]]:
    """
    Orchestrates concurrent processing of all regular files in input_dir,
    distributing them across a worker pool with per-file exclusivity.
    """
    in_path = Path(input_dir)
    out_path = Path(output_dir)

    out_path.mkdir(parents=True, exist_ok=True)

    files = [f for f in in_path.iterdir() if f.is_file() and not f.name.startswith(".")]
    lock_manager = FileLockManager()
    results = []

    with ThreadPoolExecutor(max_workers=max_workers, thread_name_prefix="FileWorker") as executor:
        future_to_file = {
            executor.submit(
                process_single_file,
                file_path=file_path,
                output_dir=out_path,
                lock_manager=lock_manager,
                chunk_size=chunk_size,
                transform_fn=transform_fn,
            ): file_path
            for file_path in files
        }

        for future in as_completed(future_to_file):
            results.append(future.result())

    return results


def run_demonstration() -> bool:
    """Self-contained verification demonstrating concurrent processing and data integrity."""
    temp_dir = tempfile.mkdtemp(prefix="concurrent_file_proc_")
    try:
        in_dir = Path(temp_dir) / "input"
        out_dir = Path(temp_dir) / "output"
        in_dir.mkdir()
        out_dir.mkdir()

        test_data = [
            ("large_file_1.txt", b"abcdefghijklmnopqrstuvwxyz" * 50000),
            ("large_file_2.txt", b"0123456789" * 100000),
            ("large_file_3.txt", b"concurrent-processing-safety-" * 40000),
            ("large_file_4.txt", b"data integrity and no deadlocks " * 35000),
        ]

        for name, content in test_data:
            with open(in_dir / name, "wb") as f:
                f.write(content)

        results = process_directory_concurrently(
            input_dir=str(in_dir),
            output_dir=str(out_dir),
            max_workers=4,
            chunk_size=64 * 1024,
        )

        all_success = all(r.get("status") == "success" for r in results)
        files_match = len(results) == len(test_data)

        for name, original in test_data:
            out_file = out_dir / name
            if not out_file.exists():
                return False
            with open(out_file, "rb") as f:
                processed_content = f.read()
            if processed_content != default_chunk_processor(original):
                return False

        return all_success and files_match
    finally:
        shutil.rmtree(temp_dir, ignore_errors=True)


if __name__ == "__main__":
    success = run_demonstration()
    print(f"Concurrent file processing executed successfully: {success}")