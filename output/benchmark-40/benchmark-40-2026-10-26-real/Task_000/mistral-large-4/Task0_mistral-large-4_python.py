import os
import threading
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path
import time

def process_file(filepath, file_locks, file_locks_guard, processed_files, processed_files_guard):
    filepath_str = str(filepath)

    with file_locks_guard:
        if filepath_str not in file_locks:
            file_locks[filepath_str] = threading.Lock()
        file_lock = file_locks[filepath_str]

    with file_lock:
        with processed_files_guard:
            if filepath_str in processed_files:
                return f"Skipped (already processed): {filepath_str}"
            processed_files.add(filepath_str)

        try:
            time.sleep(0.1)

            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read(1024)

            result = f"Processed {filepath_str}: {len(content)} bytes read"
            return result

        except Exception as e:
            return f"Error processing {filepath_str}: {str(e)}"

        finally:
            with processed_files_guard:
                processed_files.discard(filepath_str)

def process_directory_concurrently(directory_path, max_workers=4):
    directory = Path(directory_path)

    if not directory.exists() or not directory.is_dir():
        return [f"Error: {directory_path} is not a valid directory"]

    files = [f for f in directory.iterdir() if f.is_file()]

    if not files:
        return ["No files found in directory"]

    file_locks = {}
    file_locks_guard = threading.Lock()
    processed_files = set()
    processed_files_guard = threading.Lock()

    results = []

    with ThreadPoolExecutor(max_workers=max_workers) as executor:
        future_to_file = {
            executor.submit(
                process_file,
                filepath,
                file_locks,
                file_locks_guard,
                processed_files,
                processed_files_guard
            ): filepath
            for filepath in files
        }

        for future in as_completed(future_to_file):
            filepath = future_to_file[future]
            try:
                result = future.result()
                results.append(result)
            except Exception as e:
                results.append(f"Exception for {filepath}: {str(e)}")

    return results

def main():
    import sys
    directory = sys.argv[1] if len(sys.argv) > 1 else '.'
    max_workers = int(sys.argv[2]) if len(sys.argv) > 2 else 4

    results = process_directory_concurrently(directory, max_workers)
    for result in results:
        print(result)
    return results

if __name__ == '__main__':
    main()