import secrets
import threading

_generated_ids = set()
_lock = threading.Lock()


def generate_session_id(existing_ids=()):
    """Generate a secure random session ID unique to this process and existing_ids."""
    known_ids = set(existing_ids)

    with _lock:
        while True:
            session_id = secrets.token_urlsafe(32)
            if session_id not in known_ids and session_id not in _generated_ids:
                _generated_ids.add(session_id)
                return session_id