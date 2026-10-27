import argparse
import json


def copy_string(text):
    return text[:]


def concatenate_strings(*parts):
    return "".join(parts)


def reverse_string(text):
    return text[::-1]


def change_case(text, mode):
    modes = {
        "upper": str.upper,
        "lower": str.lower,
        "title": str.title,
        "swapcase": str.swapcase,
        "capitalize": str.capitalize,
    }
    if mode not in modes:
        raise ValueError(f"Unsupported case mode: {mode}")
    return modes[mode](text)


def replace_substring(text, old, new, count=-1):
    return text.replace(old, new, count)


def slice_string(text, start=None, end=None, step=None):
    return text[slice(start, end, step)]


def count_substring(text, substring):
    return text.count(substring)


def remove_whitespace(text):
    return "".join(text.split())


def split_string(text, separator=None):
    return text.split(separator)


def join_strings(parts, separator=""):
    return separator.join(parts)


def perform_operation(operation, text="", parts=None, **options):
    if operation == "copy":
        return copy_string(text)
    if operation == "concatenate":
        return concatenate_strings(*(parts if parts is not None else [text]))
    if operation == "reverse":
        return reverse_string(text)
    if operation == "case":
        return change_case(text, options.get("mode", "upper"))
    if operation == "replace":
        return replace_substring(
            text,
            options.get("old", ""),
            options.get("new", ""),
            options.get("count", -1),
        )
    if operation == "slice":
        return slice_string(
            text,
            options.get("start"),
            options.get("end"),
            options.get("step"),
        )
    if operation == "count":
        return count_substring(text, options.get("substring", ""))
    if operation == "remove-whitespace":
        return remove_whitespace(text)
    if operation == "split":
        return split_string(text, options.get("separator"))
    if operation == "join":
        return join_strings(
            parts if parts is not None else [text],
            options.get("separator", ""),
        )
    raise ValueError(f"Unsupported operation: {operation}")


def main():
    parser = argparse.ArgumentParser(description="Perform common string operations.")
    parser.add_argument(
        "operation",
        choices=[
            "copy",
            "concatenate",
            "reverse",
            "case",
            "replace",
            "slice",
            "count",
            "remove-whitespace",
            "split",
            "join",
        ],
    )
    parser.add_argument("text", nargs="*", help="Input text or parts to process.")
    parser.add_argument("--mode", default="upper", help="Case mode.")
    parser.add_argument("--old", default="", help="Substring to replace.")
    parser.add_argument("--new", default="", help="Replacement text.")
    parser.add_argument("--count", type=int, default=-1, help="Maximum replacements.")
    parser.add_argument("--substring", default="", help="Substring to count.")
    parser.add_argument("--separator", help="Separator used for splitting or joining.")
    parser.add_argument("--start", type=int)
    parser.add_argument("--end", type=int)
    parser.add_argument("--step", type=int)
    args = parser.parse_args()

    text = args.text[0] if args.text else ""
    result = perform_operation(
        args.operation,
        text=text,
        parts=args.text,
        mode=args.mode,
        old=args.old,
        new=args.new,
        count=args.count,
        substring=args.substring,
        separator=args.separator,
        start=args.start,
        end=args.end,
        step=args.step,
    )
    if isinstance(result, (list, dict)):
        print(json.dumps(result, ensure_ascii=False))
    else:
        print(result)


if __name__ == "__main__":
    main()