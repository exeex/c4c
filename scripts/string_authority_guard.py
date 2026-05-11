#!/usr/bin/env python3
"""Guard against unclassified declaration-level string authority."""

from __future__ import annotations

import argparse
import dataclasses
import json
import os
import re
import sys
from pathlib import Path
from typing import Iterable


REQUIRED_FIELDS = (
    "path",
    "symbol",
    "pattern",
    "owner",
    "domain",
    "category",
    "reason",
    "removal_condition",
    "evidence",
)

SOURCE_SUFFIXES = {
    ".c",
    ".cc",
    ".cpp",
    ".cxx",
    ".h",
    ".hh",
    ".hpp",
    ".hxx",
}

CONTAINER_RE = re.compile(
    r"\bstd::(?P<kind>unordered_map|map|unordered_set|set)\s*<(?P<args>.*)>",
    re.DOTALL,
)
USING_RE = re.compile(r"\busing\s+(?P<name>[A-Za-z_]\w*)\s*=\s*(?P<rhs>.+)$", re.DOTALL)
STRUCT_RE = re.compile(r"\b(?:struct|class)\s+(?P<name>[A-Za-z_]\w*)\b[^;{]*\{")
FUNCTION_RE = re.compile(
    r"(?:^|[\s:&*])(?P<name>[A-Za-z_]\w*)\s*\((?P<params>[^;{}]*)\)\s*(?:const\s*)?(?:->\s*[^;{]+)?(?:;|\{)",
    re.DOTALL,
)
IDENT_RE = re.compile(r"[A-Za-z_]\w*")

HELPER_ACTION_WORDS = ("find", "lookup")


@dataclasses.dataclass(frozen=True)
class Hit:
    path: str
    line: int
    symbol: str
    pattern: str
    source: str


def repo_relative(path: Path, root: Path) -> str:
    return path.resolve().relative_to(root.resolve()).as_posix()


def strip_comments(line: str, in_block: bool) -> tuple[str, bool]:
    out = []
    i = 0
    while i < len(line):
        if in_block:
            end = line.find("*/", i)
            if end == -1:
                return "".join(out), True
            i = end + 2
            in_block = False
            continue
        if line.startswith("/*", i):
            in_block = True
            i += 2
            continue
        if line.startswith("//", i):
            break
        out.append(line[i])
        i += 1
    return "".join(out), in_block


def first_template_arg(args: str) -> str:
    depth = 0
    current = []
    for ch in args:
        if ch == "<":
            depth += 1
        elif ch == ">":
            depth = max(0, depth - 1)
        elif ch == "," and depth == 0:
            return "".join(current).strip()
        current.append(ch)
    return "".join(current).strip()


def is_string_keyed_container(text: str) -> tuple[bool, str]:
    match = CONTAINER_RE.search(" ".join(text.split()))
    if not match:
        return False, ""
    kind = match.group("kind")
    args = match.group("args")
    if kind in {"unordered_set", "set"}:
        key = first_template_arg(args)
    else:
        key = first_template_arg(args)
    compact = re.sub(r"\s+", "", key)
    return compact in {"std::string", "std::string_view", "string", "string_view"}, kind


def is_suspicious_name(name: str) -> bool:
    lower = name.lower()
    return (
        lower.endswith("_by_name")
        or lower.endswith("_name_map")
        or "_by_name_" in lower
        or "name_map" in lower
        or "raw_symbol" in lower
        or "rendered" in lower
        or "legacy" in lower
        or "fallback" in lower
    )


def classify_container_pattern(kind: str, symbol: str, is_alias: bool) -> str:
    lower = symbol.lower()
    if "raw_symbol" in lower:
        return "raw-symbol-member"
    if is_alias and is_suspicious_name(symbol):
        return "by-name-alias"
    if is_suspicious_name(symbol):
        return "by-name-local" if "::" not in symbol and lower.endswith("_by_name") else "by-name-member"
    if kind in {"unordered_set", "set"}:
        return "string-keyed-container"
    if "string_view" in lower:
        return "string-view-keyed-container"
    return "string-keyed-alias" if is_alias else "string-keyed-container"


def extract_decl_name(statement: str) -> str | None:
    before_init = re.split(r"[=;{]", statement, maxsplit=1)[0]
    before_init = re.sub(r"<[^<>]*(?:<[^<>]*>[^<>]*)*>", " ", before_init)
    ids = IDENT_RE.findall(before_init)
    if not ids:
        return None
    name = ids[-1]
    if name in {"string", "string_view", "unordered_map", "unordered_set", "map", "set"}:
        return None
    return name


def helper_pattern(name: str) -> str | None:
    lower = name.lower()
    if not any(word in lower for word in HELPER_ACTION_WORDS):
        return None
    if "raw_symbol" in lower:
        return "lookup-helper-raw-symbol"
    if "rendered" in lower or "legacy" in lower or "fallback" in lower:
        return "lookup-helper-rendered-compatibility"
    if lower.endswith("_by_name") or "_by_name_" in lower:
        return "lookup-helper-name"
    return None


def classified_symbol(
    rel_path: str, symbol: str, classified_symbols: set[tuple[str, str]]
) -> str | None:
    if (rel_path, symbol) in classified_symbols:
        return symbol
    short = symbol.split("::")[-1]
    if (rel_path, short) in classified_symbols:
        return short
    if len(symbol.split("::")) >= 2:
        two_part = "::".join(symbol.split("::")[-2:])
        if (rel_path, two_part) in classified_symbols:
            return two_part
    suffix = f"::{symbol.split('::')[-1]}"
    candidates = [
        candidate_symbol
        for candidate_path, candidate_symbol in classified_symbols
        if candidate_path == rel_path and candidate_symbol.endswith(suffix)
    ]
    if len(candidates) == 1:
        return candidates[0]
    return None


def statement_hits(
    rel_path: str,
    statement: str,
    line: int,
    scope_stack: list[str],
    in_function_scope: bool,
    classifications: dict[tuple[str, str], dict[str, object]],
) -> list[Hit]:
    hits: list[Hit] = []
    classified_symbols = set(classifications)
    cleaned = " ".join(statement.split())

    using_match = USING_RE.search(cleaned)
    if using_match:
        alias = using_match.group("name")
        keyed, kind = is_string_keyed_container(using_match.group("rhs"))
        if keyed:
            pattern = classify_container_pattern(kind, alias, is_alias=True)
            accepted_symbol = classified_symbol(rel_path, alias, classified_symbols)
            if accepted_symbol is not None:
                hits.append(Hit(rel_path, line, accepted_symbol, pattern, cleaned))
            elif not in_function_scope:
                hits.append(Hit(rel_path, line, alias, pattern, cleaned))
        return hits

    func_match = FUNCTION_RE.search(cleaned)
    if func_match and not cleaned.lstrip().startswith(("if ", "for ", "while ", "switch ")):
        name = func_match.group("name")
        accepted_symbol = classified_symbol(rel_path, name, classified_symbols)
        pattern = helper_pattern(name)
        if accepted_symbol is not None:
            classified = classifications[(rel_path, accepted_symbol)]
            hits.append(Hit(rel_path, line, accepted_symbol, str(classified["pattern"]), cleaned))
        elif pattern is not None and not in_function_scope:
            hits.append(Hit(rel_path, line, name, pattern, cleaned))
        return hits

    container_match = CONTAINER_RE.search(cleaned)
    keyed, kind = is_string_keyed_container(cleaned)
    if container_match:
        name = extract_decl_name(cleaned)
        if name:
            symbol = "::".join(scope_stack + [name]) if scope_stack else name
            short = name
            pattern = classify_container_pattern(kind, symbol, is_alias=False) if keyed else "by-name-member"
            accepted_symbol = classified_symbol(rel_path, symbol, classified_symbols)
            if accepted_symbol is not None:
                classified = classifications[(rel_path, accepted_symbol)]
                hits.append(Hit(rel_path, line, accepted_symbol, str(classified["pattern"]), cleaned))
            elif (keyed or is_suspicious_name(symbol)) and not in_function_scope:
                hits.append(Hit(rel_path, line, symbol, pattern, cleaned))
    return hits


def update_scopes(text: str, brace_text: str, stack: list[tuple[str, int, str]], depth: int) -> int:
    struct_match = STRUCT_RE.search(text)
    if struct_match:
        stack.append((struct_match.group("name"), depth + brace_text.count("{"), "type"))
    func_match = FUNCTION_RE.search(text)
    if func_match and "{" in brace_text and func_match.group("name") not in {
        "if",
        "for",
        "while",
        "switch",
        "catch",
    }:
        stack.append((func_match.group("name"), depth + brace_text.count("{"), "function"))
    return depth + brace_text.count("{") - brace_text.count("}")


def active_scope(stack: list[tuple[str, int, str]], depth: int) -> tuple[list[str], bool]:
    while stack and stack[-1][1] > depth:
        stack.pop()
    return [name for name, _, _ in stack], any(kind == "function" for _, _, kind in stack)


def scan_file(
    path: Path, root: Path, classifications: dict[tuple[str, str], dict[str, object]]
) -> list[Hit]:
    rel_path = repo_relative(path, root)
    hits: list[Hit] = []
    in_block = False
    statement_parts: list[str] = []
    statement_start = 0
    depth = 0
    scopes: list[tuple[str, int, str]] = []

    for line_no, raw in enumerate(path.read_text(encoding="utf-8", errors="ignore").splitlines(), 1):
        line, in_block = strip_comments(raw, in_block)
        stripped = line.strip()
        if not stripped:
            continue

        if not statement_parts:
            statement_start = line_no
        statement_parts.append(stripped)
        statement = " ".join(statement_parts)
        scope_statement = statement

        terminates = ";" in stripped or "{" in stripped
        if terminates:
            scope_names, in_function_scope = active_scope(scopes, depth)
            hits.extend(
                statement_hits(
                    rel_path,
                    statement,
                    statement_start,
                    scope_names,
                    in_function_scope,
                    classifications,
                )
            )
            statement_parts = []

        depth = update_scopes(scope_statement, stripped, scopes, depth)
        active_scope(scopes, depth)

    return hits


def iter_source_files(repo_root: Path, roots: Iterable[str]) -> Iterable[Path]:
    for root_name in roots:
        root = repo_root / root_name
        if not root.exists():
            continue
        for dirpath, dirnames, filenames in os.walk(root):
            dirnames[:] = [d for d in dirnames if d not in {".git", "build", "__pycache__"}]
            for filename in filenames:
                path = Path(dirpath) / filename
                if path.suffix in SOURCE_SUFFIXES:
                    yield path


def load_config(path: Path) -> tuple[list[str], dict[tuple[str, str], dict[str, object]], list[str]]:
    errors: list[str] = []
    data = json.loads(path.read_text(encoding="utf-8"))
    roots = data.get("roots")
    classifications = data.get("classifications")
    if not isinstance(roots, list) or not all(isinstance(root, str) for root in roots):
        errors.append("classification file must contain a string list field: roots")
        roots = []
    if not isinstance(classifications, list):
        errors.append("classification file must contain a list field: classifications")
        classifications = []

    entries: dict[tuple[str, str], dict[str, object]] = {}
    for index, entry in enumerate(classifications):
        if not isinstance(entry, dict):
            errors.append(f"classifications[{index}] must be an object")
            continue
        missing = [field for field in REQUIRED_FIELDS if not entry.get(field)]
        if missing:
            errors.append(
                f"classifications[{index}] is missing required fields: {', '.join(missing)}"
            )
            continue
        key = (str(entry["path"]), str(entry["symbol"]))
        if key in entries:
            errors.append(f"duplicate classification for {key[0]}: {key[1]}")
            continue
        entries[key] = entry
    return roots, entries, errors


def run_guard(repo_root: Path, classifications_path: Path) -> int:
    roots, classifications, errors = load_config(classifications_path)
    hits: list[Hit] = []
    for path in iter_source_files(repo_root, roots):
        hits.extend(scan_file(path, repo_root, classifications))

    unclassified = [hit for hit in hits if (hit.path, hit.symbol) not in classifications]
    if errors or unclassified:
        print("string authority guard failed")
        for error in errors:
            print(f"config: {error}")
        for hit in sorted(unclassified, key=lambda h: (h.path, h.line, h.symbol)):
            print(
                f"{hit.path}:{hit.line}: {hit.symbol}: pattern={hit.pattern}: "
                "classify this exact path+symbol in scripts/string_authority_classifications.json "
                "or replace it with structured authority"
            )
        return 1

    print(f"string authority guard passed: {len(hits)} classified declaration-level hits")
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, default=Path.cwd())
    parser.add_argument(
        "--classifications",
        type=Path,
        default=Path("scripts/string_authority_classifications.json"),
    )
    args = parser.parse_args(argv)
    repo_root = args.repo_root.resolve()
    classifications = args.classifications
    if not classifications.is_absolute():
        classifications = repo_root / classifications
    return run_guard(repo_root, classifications.resolve())


if __name__ == "__main__":
    sys.exit(main())
