import subprocess
import sys

def validate_command(command):
    if not isinstance(command, str):
        return False, "Command must be a string"
    if not command.strip():
        return False, "Command cannot be empty"
    if len(command) > 1000:
        return False, "Command too long (max 1000 characters)"
    dangerous = ['rm -rf /', 'rm -rf /*', ':(){ :|:& };:', 'mkfs.', 'dd if=/dev/zero', '> /dev/sda', 'mv / /dev/null']
    for pattern in dangerous:
        if pattern in command:
            return False, f"Dangerous pattern detected: {pattern}"
    return True, "Valid"

def execute_command(command):
    try:
        result = subprocess.run(
            command,
            shell=True,
            executable='/bin/bash',
            capture_output=True,
            text=True,
            timeout=30
        )
        return {
            'returncode': result.returncode,
            'stdout': result.stdout,
            'stderr': result.stderr,
            'success': result.returncode == 0
        }
    except subprocess.TimeoutExpired:
        return {
            'returncode': -1,
            'stdout': '',
            'stderr': 'Command timed out after 30 seconds',
            'success': False
        }
    except Exception as e:
        return {
            'returncode': -1,
            'stdout': '',
            'stderr': str(e),
            'success': False
        }

def process_command(command):
    is_valid, message = validate_command(command)
    if not is_valid:
        return {
            'returncode': -1,
            'stdout': '',
            'stderr': f"Validation error: {message}",
            'success': False
        }
    return execute_command(command)

def main():
    try:
        command = input("Enter bash command: ")
        result = process_command(command)
        print(f"Exit code: {result['returncode']}")
        if result['stdout']:
            print("STDOUT:")
            print(result['stdout'])
        if result['stderr']:
            print("STDERR:")
            print(result['stderr'])
        return result
    except KeyboardInterrupt:
        print("\nCancelled by user")
        return None
    except Exception as e:
        print(f"Error: {e}")
        return None

if __name__ == "__main__":
    main()