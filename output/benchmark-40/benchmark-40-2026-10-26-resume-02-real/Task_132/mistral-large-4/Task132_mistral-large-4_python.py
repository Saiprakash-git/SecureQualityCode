import logging
import json
import os
import re
import time
from datetime import datetime, timedelta
from collections import defaultdict, deque
from typing import Dict, List, Optional, Any, Callable
from dataclasses import dataclass, asdict
from enum import Enum
import hashlib

class EventSeverity(Enum):
    INFO = "INFO"
    WARNING = "WARNING"
    CRITICAL = "CRITICAL"
    HIGH = "HIGH"

class EventCategory(Enum):
    AUTHENTICATION = "AUTHENTICATION"
    AUTHORIZATION = "AUTHORIZATION"
    FILE_ACCESS = "FILE_ACCESS"
    NETWORK = "NETWORK"
    PRIVILEGE_ESCALATION = "PRIVILEGE_ESCALATION"
    SYSTEM = "SYSTEM"
    APPLICATION = "APPLICATION"

@dataclass
class SecurityEvent:
    timestamp: str
    event_id: str
    category: str
    severity: str
    source: str
    user: Optional[str]
    action: str
    resource: Optional[str]
    ip_address