import json
import uuid
from dataclasses import asdict, dataclass, field
from datetime import datetime, timedelta, timezone
from enum import Enum
from typing import Any, Callable, Dict, List, Optional


class EventSeverity(str, Enum):
    INFO = "INFO"
    LOW = "LOW"
    MEDIUM = "MEDIUM"
    HIGH = "HIGH"
    CRITICAL = "CRITICAL"


class EventType(str, Enum):
    AUTH_SUCCESS = "AUTH_SUCCESS"
    AUTH_FAILURE = "AUTH_FAILURE"
    PRIVILEGE_ESCALATION = "PRIVILEGE_ESCALATION"
    UNAUTHORIZED_ACCESS = "UNAUTHORIZED_ACCESS"
    PORT_SCAN_DETECTED = "PORT_SCAN_DETECTED"
    DATA_EXFILTRATION = "DATA_EXFILTRATION"
    CONFIG_CHANGE = "CONFIG_CHANGE"


@dataclass
class SecurityEvent:
    event_id: str
    timestamp: datetime
    event_type: EventType
    severity: EventSeverity
    source_ip: str
    user_id: Optional[str]
    description: str
    metadata: Dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> Dict[str, Any]:
        data = asdict(self)
        data["timestamp"] = self.timestamp.isoformat()
        data["event_type"] = self.event_type.value
        data["severity"] = self.severity.value
        return data


@dataclass
class SecurityAlert:
    alert_id: str
    timestamp: datetime
    rule_name: str
    severity: EventSeverity
    description: str
    trigger_event_ids: List[str]

    def to_dict(self) -> Dict[str, Any]:
        data = asdict(self)
        data["timestamp"] = self.timestamp.isoformat()
        data["severity"] = self.severity.value
        return data


class DetectionRule:
    def __init__(
        self,
        rule_name: str,
        severity: EventSeverity,
        evaluator: Callable[[List[SecurityEvent]], List[SecurityAlert]],
    ):
        self.rule_name = rule_name
        self.severity = severity
        self.evaluator = evaluator

    def evaluate(self, events: List[SecurityEvent]) -> List[SecurityAlert]:
        return self.evaluator(events)


def build_brute_force_rule(
    max_failures: int = 5, window_seconds: int = 300
) -> DetectionRule:
    def evaluator(events: List[SecurityEvent]) -> List[SecurityAlert]:
        alerts: List[SecurityAlert] = []
        failures_by_ip: Dict[str, List[SecurityEvent]] = {}
        now = datetime.now(timezone.utc)
        window_start = now - timedelta(seconds=window_seconds)

        for event in events:
            if (
                event.event_type == EventType.AUTH_FAILURE
                and event.timestamp >= window_start
            ):
                failures_by_ip.setdefault(event.source_ip, []).append(event)

        for ip, ip_events in failures_by_ip.items():
            if len(ip_events) >= max_failures:
                alerts.append(
                    SecurityAlert(
                        alert_id=str(uuid.uuid4()),
                        timestamp=datetime.now(timezone.utc),
                        rule_name="Brute-Force Authentication Attempt",
                        severity=EventSeverity.HIGH,
                        description=(
                            f"IP {ip} accumulated {len(ip_events)} failed login "
                            f"attempts within {window_seconds} seconds."
                        ),
                        trigger_event_ids=[e.event_id for e in ip_events],
                    )
                )
        return alerts

    return DetectionRule(
        rule_name="Brute-Force Authentication Attempt",
        severity=EventSeverity.HIGH,
        evaluator=evaluator,
    )


def build_privilege_escalation_rule() -> DetectionRule:
    def evaluator(events: List[SecurityEvent]) -> List[SecurityAlert]:
        alerts: List[SecurityAlert] = []
        now = datetime.now(timezone.utc)
        recent_cutoff = now - timedelta(seconds=60)

        for event in events:
            if (
                event.event_type == EventType.PRIVILEGE_ESCALATION
                and event.timestamp >= recent_cutoff
            ):
                alerts.append(
                    SecurityAlert(
                        alert_id=str(uuid.uuid4()),
                        timestamp=datetime.now(timezone.utc),
                        rule_name="Critical Privilege Escalation",
                        severity=EventSeverity.CRITICAL,
                        description=(
                            f"User '{event.user_id}' performed privilege escalation from "
                            f"IP {event.source_ip}: {event.description}"
                        ),
                        trigger_event_ids=[event.event_id],
                    )
                )
        return alerts

    return DetectionRule(
        rule_name="Critical Privilege Escalation",
        severity=EventSeverity.CRITICAL,
        evaluator=evaluator,
    )


class SecurityMonitor:
    def __init__(self) -> None:
        self.events: List[SecurityEvent] = []
        self.alerts: List[SecurityAlert] = []
        self.rules: List[DetectionRule] = []

    def register_rule(self, rule: DetectionRule) -> None:
        self.rules.append(rule)

    def log_event(
        self,
        event_type: EventType,
        severity: EventSeverity,
        source_ip: str,
        user_id: Optional[str],
        description: str,
        metadata: Optional[Dict[str, Any]] = None,
        timestamp: Optional[datetime] = None,
    ) -> SecurityEvent:
        if timestamp is None:
            timestamp = datetime.now(timezone.utc)
        if metadata is None:
            metadata = {}

        event = SecurityEvent(
            event_id=str(uuid.uuid4()),
            timestamp=timestamp,
            event_type=event_type,
            severity=severity,
            source_ip=source_ip,
            user_id=user_id,
            description=description,
            metadata=metadata,
        )
        self.events.append(event)
        self._evaluate_event(event)
        return event

    def _evaluate_event(self, event: SecurityEvent) -> List[SecurityAlert]:
        generated_alerts: List[SecurityAlert] = []
        for rule in self.rules:
            new_alerts = rule.evaluate(self.events)
            for alert in new_alerts:
                already_recorded = any(
                    a.rule_name == alert.rule_name
                    and set(a.trigger_event_ids) == set(alert.trigger_event_ids)
                    for a in self.alerts
                )
                if not already_recorded:
                    self.alerts.append(alert)
                    generated_alerts.append(alert)
        return generated_alerts

    def run_all_rules(self) -> List[SecurityAlert]:
        current_alerts: List[SecurityAlert] = []
        for rule in self.rules:
            current_alerts.extend(rule.evaluate(self.events))
        return current_alerts

    def get_events(
        self,
        event_type: Optional[EventType] = None,
        severity: Optional[EventSeverity] = None,
        source_ip: Optional[str] = None,
    ) -> List[SecurityEvent]:
        filtered = self.events
        if event_type:
            filtered = [e for e in filtered if e.event_type == event_type]
        if severity:
            filtered = [e for e in filtered if e.severity == severity]
        if source_ip:
            filtered = [e for e in filtered if e.source_ip == source_ip]
        return filtered

    def get_alerts(
        self, severity: Optional[EventSeverity] = None
    ) -> List[SecurityAlert]:
        if severity:
            return [a for a in self.alerts if a.severity == severity]
        return list(self.alerts)

    def export_events_json(self) -> str:
        return json.dumps([e.to_dict() for e in self.events], indent=2)

    def export_alerts_json(self) -> str:
        return json.dumps([a.to_dict() for a in self.alerts], indent=2)


def main() -> None:
    monitor = SecurityMonitor()
    monitor.register_rule(build_brute_force_rule(max_failures=3, window_seconds=120))
    monitor.register_rule(build_privilege_escalation_rule())

    current_time = datetime.now(timezone.utc)

    monitor.log_event(
        event_type=EventType.AUTH_SUCCESS,
        severity=EventSeverity.INFO,
        source_ip="192.168.1.10",
        user_id="alice",
        description="User alice logged in successfully.",
        timestamp=current_time - timedelta(seconds=60),
    )

    for i in range(4):
        monitor.log_event(
            event_type=EventType.AUTH_FAILURE,
            severity=EventSeverity.LOW,
            source_ip="203.0.113.55",
            user_id="root",
            description="Failed password attempt for root.",
            timestamp=current_time - timedelta(seconds=40 - (i * 5)),
        )

    monitor.log_event(
        event_type=EventType.PRIVILEGE_ESCALATION,
        severity=EventSeverity.CRITICAL,
        source_ip="192.168.1.25",
        user_id="bob",
        description="User bob added to sudoers group without approval ticket.",
        timestamp=current_time,
    )

    print("=== SECURITY LOGS ===")
    print(monitor.export_events_json())
    print("\n=== SECURITY ALERTS GENERATED ===")
    print(monitor.export_alerts_json())


if __name__ == "__main__":
    main()