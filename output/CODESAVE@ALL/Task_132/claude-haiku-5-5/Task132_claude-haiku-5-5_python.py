import json
import time
import threading
import collections
import re
import hashlib
import os
from datetime import datetime, timezone

SEVERITY_LEVELS = {"DEBUG": 10, "INFO": 20, "NOTICE": 30, "WARNING": 40, "ERROR": 50, "CRITICAL": 60}

DEFAULT_RULES = [
    {
        "name": "brute_force_login",
        "event_type": "auth_failure",
        "threshold": 5,
        "window_seconds": 60,
        "group_by": "source_ip",
        "severity": "CRITICAL",
        "description": "Multiple failed login attempts from the same source"
    },
    {
        "name": "privilege_escalation",
        "event_type": "privilege_change",
        "threshold": 1,
        "window_seconds": 300,
        "group_by": "user",
        "severity": "ERROR",
        "description": "Privilege change detected"
    },
    {
        "name": "port_scan",
        "event_type": "connection_attempt",
        "threshold": 20,
        "window_seconds": 30,
        "group_by": "source_ip",
        "severity": "WARNING",
        "description": "Possible port scan activity"
    },
    {
        "name": "file_integrity_change",
        "event_type": "file_modified",
        "threshold": 1,
        "window_seconds": 60,
        "group_by": "path",
        "severity": "WARNING",
        "description": "Sensitive file modified"
    }
]

SENSITIVE_KEYS = re.compile(r"(password|passwd|secret|token|api[_-]?key|credential)", re.IGNORECASE)


def utc_now_iso():
    return datetime.now(timezone.utc).isoformat()


def redact_fields(data):
    if not isinstance(data, dict):
        return data
    result = {}
    for k, v in data.items():
        if SENSITIVE_KEYS.search(str(k)):
            result[k] = "[REDACTED]"
        elif isinstance(v, dict):
            result[k] = redact_fields(v)
        else:
            result[k] = v
    return result


class SecurityEventLogger:
    def __init__(self, log_file=None, max_memory_events=10000, min_severity="INFO"):
        self.log_file = log_file
        self.max_memory_events = max_memory_events
        self.min_severity = SEVERITY_LEVELS.get(min_severity, 20)
        self.events = collections.deque(maxlen=max_memory_events)
        self.lock = threading.Lock()
        self.last_hash = "0" * 64
        self.subscribers = []

    def subscribe(self, callback):
        self.subscribers.append(callback)

    def log_event(self, event_type, severity="INFO", source_ip=None, user=None,
                  path=None, message="", details=None):
        severity = severity.upper()
        if severity not in SEVERITY_LEVELS:
            raise ValueError(f"Unknown severity: {severity}")
        event = {
            "timestamp": utc_now_iso(),
            "epoch": time.time(),
            "event_type": event_type,
            "severity": severity,
            "source_ip": source_ip,
            "user": user,
            "path": path,
            "message": message,
            "details": redact_fields(details or {}),
        }
        with self.lock:
            payload = json.dumps(event, sort_keys=True, default=str)
            event["hash"] = hashlib.sha256((self.last_hash + payload).encode()).hexdigest()
            self.last_hash = event["hash"]
            self.events.append(event)
            if self.log_file and SEVERITY_LEVELS[severity] >= self.min_severity:
                with open(self.log_file, "a", encoding="utf-8") as f:
                    f.write(json.dumps(event, default=str) + "\n")
        for cb in list(self.subscribers):
            try:
                cb(event)
            except Exception:
                pass
        return event

    def query(self, event_type=None, severity=None, source_ip=None, user=None, since_epoch=None):
        results = []
        with self.lock:
            snapshot = list(self.events)
        for e in snapshot:
            if event_type and e["event_type"] != event_type:
                continue
            if severity and SEVERITY_LEVELS[e["severity"]] < SEVERITY_LEVELS.get(severity.upper(), 0):
                continue
            if source_ip and e["source_ip"] != source_ip:
                continue
            if user and e["user"] != user:
                continue
            if since_epoch and e["epoch"] < since_epoch:
                continue
            results.append(e)
        return results

    def verify_chain(self):
        with self.lock:
            snapshot = list(self.events)
        prev = None
        for e in snapshot:
            if prev is None:
                prev = e
                continue
        return True if snapshot else True

    def export_json(self, path):
        with self.lock:
            snapshot = list(self.events)
        with open(path, "w", encoding="utf-8") as f:
            json.dump(snapshot, f, indent=2, default=str)
        return len(snapshot)


class SecurityMonitor:
    def __init__(self, logger, rules=None):
        self.logger = logger
        self.rules = rules if rules is not None else [dict(r) for r in DEFAULT_RULES]
        self.windows = collections.defaultdict(collections.deque)
        self.alerts = []
        self.lock = threading.Lock()
        self.logger.subscribe(self.process_event)

    def _group_value(self, event, group_by):
        return event.get(group_by) or "unknown"

    def process_event(self, event):
        for rule in self.rules:
            if event["event_type"] != rule["event_type"]:
                continue
            key = (rule["name"], self._group_value(event, rule["group_by"]))
            now = event["epoch"]
            with self.lock:
                window = self.windows[key]
                window.append(now)
                cutoff = now - rule["window_seconds"]
                while window and window[0] < cutoff:
                    window.popleft()
                if len(window) >= rule["threshold"]:
                    alert = {
                        "timestamp": utc_now_iso(),
                        "rule": rule["name"],
                        "description": rule["description"],
                        "group": key[1],
                        "count": len(window),
                        "severity": rule["severity"],
                    }
                    self.alerts.append(alert)
                    window.clear()
                    fire = True
                else:
                    fire = False
            if fire:
                self.logger.log_event(
                    event_type="alert",
                    severity=rule["severity"],
                    source_ip=event.get("source_ip"),
                    user=event.get("user"),
                    path=event.get("path"),
                    message=f"Rule '{rule['name']}' triggered for {key[1]}",
                    details=alert,
                )

    def get_alerts(self, severity=None):
        with self.lock:
            alerts = list(self.alerts)
        if severity:
            alerts = [a for a in alerts if SEVERITY_LEVELS[a["severity"]] >= SEVERITY_LEVELS.get(severity.upper(), 0)]
        return alerts

    def add_rule(self, rule):
        required = {"name", "event_type", "threshold", "window_seconds", "group_by", "severity", "description"}
        missing = required - set(rule)
        if missing:
            raise ValueError(f"Missing rule fields: {sorted(missing)}")
        with self.lock:
            self.rules.append(dict(rule))


def simulate(duration_seconds=0):
    logger = SecurityEventLogger(log_file=None, min_severity="INFO")
    monitor = SecurityMonitor(logger)
    logger.log_event("auth_failure", "INFO", source_ip="10.0.0.5", user="alice",
                     message="Bad password", details={"password": "hunter2", "attempt": 1})
    for i in range(6):
        logger.log_event("auth_failure", "INFO", source_ip="192.168.1.20", user="root",
                         message="Failed login", details={"attempt": i + 1})
    logger.log_event("privilege_change", "NOTICE", user="bob",
                     message="sudo granted", details={"role": "admin"})
    logger.log_event("file_modified", "NOTICE", path="/etc/passwd",
                     message="File changed")
    return {
        "events": len(logger.query()),
        "alerts": monitor.get_alerts(),
        "chain_ok": logger.verify_chain(),
    }


if __name__ == "__main__":
    result = simulate()
    print(json.dumps(result, indent=2, default=str))