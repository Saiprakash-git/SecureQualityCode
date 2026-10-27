import sys


def copy_string(text):
    return str(text)[:]


def concatenate_strings(*strings):
    return ''.join(str(item) for item in strings)


def reverse_string(text):
    return str(text)[::-1]


def to_upper(text):
    return str(text).upper()


def to_lower(text):
    return str(text).lower()


def capitalize(text):
    return str(text).capitalize()


def title_case(text):
    return str(text).title()


def swapcase(text):
    return str(text).swapcase()


def strip(text, chars=None):
    return str(text).strip(chars)


def replace_substring(text, old, new, count=-1):
    return str(text).replace(str(old), str(new), int(count))


def split_string(text, sep=None, maxsplit=-1):
    return str(text).split(sep, int(maxsplit))


def join_strings(sep, iterable=None, *parts):
    if iterable is not None:
        return str(sep).join(str(item) for item in iterable)
    return str(sep).join(str(item) for item in parts)


def find_substring(text, sub, start=0, end=None):
    text = str(text)
    sub = str(sub)
    start = int(start)
    if end is not None:
        end = int(end)
        return text.find(sub, start, end)
    return text.find(sub, start)


def count_substring(text, sub, start=0, end=None):
    text = str(text)
    sub = str(sub)
    start = int(start)
    if end is not None:
        end = int(end)
        return text.count(sub, start, end)
    return text.count(sub, start)


def substring(text, start=None, end=None):
    text = str(text)
    if start is not None:
        start = int(start)
    if end is not None:
        end = int(end)
    return text[start:end]


def length(text):
    return len(str(text))


def compare(text_a, text_b):
    a = str(text_a)
    b = str(text_b)
    if a < b:
        return -1
    if a > b:
        return 1
    return 0


def is_palindrome(text):
    cleaned = ''.join(ch.lower() for ch in str(text) if ch.isalnum())
    return cleaned == cleaned[::-1]


def pad_left(text, width, fill=' '):
    fill = str(fill)
    if len(fill) != 1:
        fill = fill[:1] or ' '
    return str(text).rjust(int(width), fill)


def pad_right(text, width, fill=' '):
    fill = str(fill)
    if len(fill) != 1:
        fill = fill[:1] or ' '
    return str(text).ljust(int(width), fill)


def center_text(text, width, fill=' '):
    fill = str(fill)
    if len(fill) != 1:
        fill = fill[:1] or ' '
    return str(text).center(int(width), fill)


def zfill(text, width):
    return str(text).zfill(int(width))


def remove_whitespace(text):
    return ''.join(str(text).split())


OPERATIONS = {
    'copy': copy_string,
    'concat': concatenate_strings,
    'reverse': reverse_string,
    'upper': to_upper,
    'lower': to_lower,
    'capitalize': capitalize,
    'title': title_case,
    'swapcase': swapcase,
    'strip': strip,
    'replace': replace_substring,
    'split': split_string,
    'join': join_strings,
    'find': find_substring,
    'count': count_substring,
    'substring': substring,
    'length': length,
    'compare': compare,
    'palindrome': is_palindrome,
    'pad_left': pad_left,
    'pad_right': pad_right,
    'center': center_text,
    'zfill': zfill,
    'remove_whitespace': remove_whitespace,
}


def apply_string_operation(operation, *args, **kwargs):
    if operation not in OPERATIONS:
        raise ValueError(f'Unknown string operation: {operation}')
    return OPERATIONS[operation](*args, **kwargs)


def main():
    if len(sys.argv) > 1:
        operation = sys.argv[1]
        args = sys.argv[2:]
        try:
            result = apply_string_operation(operation, *args)
            if isinstance(result, list):
                print('\n'.join(str(item) for item in result))
            else:
                print(result)
        except Exception as exc:
            print(f'Error: {exc}', file=sys.stderr)
            sys.exit(1)
        return

    print('Usage: python string_operations.py <operation> <arguments...>')
    print('Available operations: ' + ', '.join(sorted(OPERATIONS.keys())))


if __name__ == '__main__':
    main()