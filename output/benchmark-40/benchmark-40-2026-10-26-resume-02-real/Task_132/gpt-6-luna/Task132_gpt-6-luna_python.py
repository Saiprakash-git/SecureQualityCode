#!/usr/bin/env python3
"""Record security events as JSON Lines and alert on suspicious activity."""

from __future__ import annotations

import argparse
import json
import os
import sys
import threading
import time
from collections import defaultdict, deque
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Mapping


SENSITIVE_KEYS = {
    "password",
    "passwd",
    "secret",
    "token",
    "access_token",
    "refresh_token",
    "api_key",
    "authorization",
    "cookie",
}


def utc_now() -> str:
    """Return the current UTC time in ISO 8601 format."""
    return datetime.now(timezone.utc).isoformat()


def redact(value: Any, sensitive_keys: set[str] | None = None) -> Any:
    """Recursively redact values whose keys commonly contain credentials."""
    keys = sensitive_keys if sensitive_keys is not None else SENSITIVE_KEYS
    if isinstance(value, Mapping):
        return {
            str(key): (
                "[REDACTED]"
                if str(key).lower() in keys
                else redact(item, keys)
            )
            for key, item in value.items()
        }
    if isinstance(value, list):
        return [redact(item, keys) for item in value]
    if isinstance(value, tuple):
        return [redact(item, keys) for item in value]
    return value


class SecurityMonitor:
    """Validate, persist, and monitor security-related events."""

    IMMEDIATE_ALERT_TYPES = {
        "malware_detected",
        "privilege_escalation",
        "unauthorized_access",
        "account_compromised",
    }

    def __init__(
        self,
        log_path: str | os.PathLike[str],
        failure_threshold: int = 5,
        failure_window_seconds: int = 300,
    ) -> None:
        if failure_threshold < 1:
            raise ValueError("failure_threshold must be at least 1")
        if failure_window_seconds < 1:
            raise ValueError("failure_window_seconds must be at least 1")

        self.log_path = Path(log_path)
        self.failure_threshold = failure_threshold
        self.failure_window_seconds = failure_window_seconds
        self._failures: dict[str, deque[float]] = defaultdict(deque)
        self._lock = threading.Lock()

    def record(self, event: Mapping[str, Any]) -> dict[str, Any]:
        """Validate and record one event; return the event and generated alerts."""
        if not isinstance(event, Mapping):
            raise TypeError("event must be a mapping")

        event_type = event.get("event_type")
        if not isinstance(event_type, str) or not event_type.strip():
            raise ValueError("event_type must be a non-empty string")

        normalized = redact(dict(event))
        normalized["event_type"] = event_type.strip().lower()
        normalized.setdefault("timestamp", utc_now())
        normalized.setdefault("severity", "info")

        if not isinstance(normalized["timestamp"], str):
            raise ValueError("timestamp must be a string")
        if not isinstance(normalized["severity"], str):
            raise ValueError("severity must be a string")

        alerts = self._detect_alerts(normalized)
        result = {"event": normalized, "alerts": alerts}

        with self._lock:
            self._append_json_line(result)

        return result

    def _detect_alerts(self, event: dict[str, Any]) -> list[dict[str, Any]]:
        alerts: list[dict[str, Any]] = []
        event_type = event["event_type"]

        if event_type in self.IMMEDIATE_ALERT_TYPES:
            alerts.append(
                {
                    "timestamp": utc_now(),
                    "severity": "high",
                    "rule": "immediate_security_event",
                    "message": f"Security event requires immediate attention: {event_type}",
                    "source": event.get("source"),
                }
            )

        if event_type in {"login_failure", "authentication_failure"}:
            source = str(event.get("source", "unknown"))
            now = time.monotonic()
            failures = self._failures[source]
            failures.append(now)

            cutoff = now - self.failure_window_seconds
            while failures and failures[0] < cutoff:
                failures.popleft()

            if len(failures) >= self.failure_threshold:
                alerts.append(
                    {
                        "timestamp": utc_now(),
                        "severity": "high",
                        "rule": "repeated_authentication_failures",
                        "message": (
                            f"{len(failures)} authentication failures from "
                            f"{source} within {self.failure_window_seconds} seconds"
                        ),
                        "source": source,
                        "count": len(failures),
                    }
                )

        return alerts

    def _append_json_line(self, record: dict[str, Any]) -> None:
        self.log_path.parent.mkdir(parents=True, exist_ok=True)
        line = json.dumps(record, ensure_ascii=False, separators=(",", ":")) + "\n"

        file_descriptor = os.open(
            self.log_path,
            os.O_WRONLY | os.O_CREAT | os.O_APPEND,
            0o600,
        )
        try:
            os.fchmod(file_descriptor, 0o600)
            with os.fdopen(file_descriptor, "a", encoding="utf-8") as log_file:
                log_file.write(line)
                log_file.flush()
                os.fsync(log_file.fileno())
        except Exception:
            try:
                os.close(file_descriptor)
            except OSError:
                pass
            raise


def process_json_line(
    line: str,
    monitor: SecurityMonitor,
) -> dict[str, Any]:
    """Parse a JSON event and pass it to the monitor."""
    event = json.loads(line)
    if not isinstance(event, Mapping):
        raise ValueError("each input line must contain a JSON object")
    return monitor.record(event)


def build_argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Record and monitor security events supplied as JSON Lines."
    )
    parser.add_argument(
        "--log",
        default="security-events.jsonl",
        help="JSON Lines output file (default: security-events.jsonl)",
    )
    parser.add_argument(
        "--failure-threshold",
        type=int,
        default=5,
        help="authentication failures needed to trigger an alert (default: 5)",
    )
    parser.add_argument(
        "--failure-window",
        type=int,
        default=300,
        help="authentication failure window in seconds (default: 300)",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_argument_parser().parse_args(argv)
    try:
        monitor = SecurityMonitor(
            log_path=args.log,
            failure_threshold=args.failure_threshold,
            failure_window_seconds=args.failure_window,
        )
    except (ValueError, OSError) as error:
        print(f"configuration error: {error}", file=sys.stderr)
        return 2

    for line_number, line in enumerate(sys.stdin, start=1):
        if not line.strip():
            continue
        try:
            result = process_json_line(line, monitor)
            for alert in result["alerts"]:
                print(json.dumps(alert, ensure_ascii=False), file=sys.stderr)
        except (json.JSONDecodeError, TypeError, ValueError, OSError) as error:
            print(f"line {line_number}: {error}", file=sys.stderr)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())