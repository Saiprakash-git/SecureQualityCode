def copy_string(source):
    return source[:]

def concatenate_strings(str1, str2, separator=""):
    return str1 + separator + str2

def reverse_string(text):
    return text[::-1]

def to_uppercase(text):
    return text.upper()

def to_lowercase(text):
    return text.lower()

def to_title_case(text):
    return text.title()

def capitalize_string(text):
    return text.capitalize()

def swap_case(text):
    return text.swapcase()

def casefold_string(text):
    return text.casefold()

def find_substring(text, substring):
    return text.find(substring)

def rfind_substring(text, substring):
    return text.rfind(substring)

def count_occurrences(text, substring):
    return text.count(substring)

def replace_substring(text, old, new, count=-1):
    return text.replace(old, new, count)

def split_string(text, delimiter=None, maxsplit=-1):
    return text.split(delimiter, maxsplit)

def rsplit_string(text, delimiter=None, maxsplit=-1):
    return text.rsplit(delimiter, maxsplit)

def join_strings(strings, separator=" "):
    return separator.join(strings)

def strip_whitespace(text, chars=None):
    return text.strip(chars)

def lstrip_whitespace(text, chars=None):
    return text.lstrip(chars)

def rstrip_whitespace(text, chars=None):
    return text.rstrip(chars)

def is_alphabetic(text):
    return text.isalpha()

def is_numeric(text):
    return text.isdigit()

def is_decimal(text):
    return text.isdecimal()

def is_alphanumeric(text):
    return text.isalnum()

def is_whitespace(text):
    return text.isspace()

def is_uppercase(text):
    return text.isupper()

def is_lowercase(text):
    return text.islower()

def is_title_case(text):
    return text.istitle()

def format_string(template, *args, **kwargs):
    return template.format(*args, **kwargs)

def slice_string(text, start, end=None, step=None):
    return text[start:end:step]

def repeat_string(text, count):
    return text * count

def center_string(text, width, fillchar=" "):
    return text.center(width, fillchar)

def ljust_string(text, width, fillchar=" "):
    return text.ljust(width, fillchar)

def rjust_string(text, width, fillchar=" "):
    return text.rjust(width, fillchar)

def zfill_string(text, width):
    return text.zfill(width)

def remove_prefix(text, prefix):
    if text.startswith(prefix):
        return text[len(prefix):]
    return text

def remove_suffix(text, suffix):
    if text.endswith(suffix):
        return text[:-len(suffix)]
    return text

def check_startswith(text, prefix):
    return text.startswith(prefix)

def check_endswith(text, suffix):
    return text.endswith(suffix)

def partition_string(text, separator):
    return text.partition(separator)

def rpartition_string(text, separator):
    return text.rpartition(separator)

def expand_tabs(text, tabsize=8):
    return text.expandtabs(tabsize)

def translate_string(text, table):
    return text.translate(table)

def make_translation_table(x, y=None, z=None):
    return str.maketrans(x, y, z)

def encode_string(text, encoding="utf-8", errors="strict"):
    return text.encode(encoding, errors)

def decode_string(byte_string, encoding="utf-8", errors="strict"):
    return byte_string.decode(encoding, errors)

def get_length(text):
    return len(text)

def get_char_at(text, index):
    return text[index]

def compare_strings(str1, str2):
    if str1 < str2:
        return -1
    elif str1 > str2:
        return 1
    else:
        return 0

def string_to_list(text):
    return list(text)

def list_to_string(char_list):
    return "".join(char_list)