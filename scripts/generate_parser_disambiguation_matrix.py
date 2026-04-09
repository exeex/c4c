#!/usr/bin/env python3
"""Generate idea 44 parser-disambiguation matrix patterns."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from itertools import product
from pathlib import Path
from typing import Iterable


@dataclass(frozen=True)
class OwnerSpelling:
    label: str
    text: str


@dataclass(frozen=True)
class DeclaratorShape:
    label: str
    abstract_template: str

    def render(self, owner: str) -> str:
        return self.abstract_template.format(owner=owner)


@dataclass(frozen=True)
class ParseContext:
    label: str
    example_template: str

    def render(self, type_id: str) -> str:
        return self.example_template.format(type_id=type_id)


OWNERS: tuple[OwnerSpelling, ...] = (
    OwnerSpelling("simple", "T"),
    OwnerSpelling("qualified", "ns::T"),
    OwnerSpelling("global_qualified", "::ns::T"),
    OwnerSpelling("dependent_typename", "typename H<T>::Type"),
    OwnerSpelling(
        "dependent_template_member",
        "typename H<T>::template Rebind<U>::Type",
    ),
)

DECLARATORS: tuple[DeclaratorShape, ...] = (
    DeclaratorShape("pointer", "{owner}*"),
    DeclaratorShape("lvalue_ref", "{owner}&"),
    DeclaratorShape("rvalue_ref", "{owner}&&"),
    DeclaratorShape("cv_pointer", "{owner}* const"),
    DeclaratorShape("grouped_pointer", "{owner} (*)"),
    DeclaratorShape("grouped_lvalue_ref", "{owner} (&)"),
    DeclaratorShape("grouped_rvalue_ref", "{owner} (&&)"),
    DeclaratorShape("member_pointer", "{owner} C::*"),
    DeclaratorShape("function_pointer", "{owner} (*)(Arg)"),
    DeclaratorShape("function_lvalue_ref", "{owner} (&)(Arg)"),
    DeclaratorShape("function_rvalue_ref", "{owner} (&&)(Arg)"),
    DeclaratorShape("member_function_pointer", "{owner} (C::*)(Arg)"),
)

CONTEXTS: tuple[ParseContext, ...] = (
    ParseContext("c_style_cast_target", "({type_id})expr"),
    ParseContext("local_declaration", "{type_id} value;"),
    ParseContext("parameter_declaration", "void probe({type_id} value);"),
    ParseContext("parenthesized_type_id_consumer", "sizeof({type_id})"),
    ParseContext("ambiguous_statement_context", "{type_id}(value);"),
)


def iter_lines() -> Iterable[str]:
    index = 1
    for owner, declarator, context in product(OWNERS, DECLARATORS, CONTEXTS):
        type_id = declarator.render(owner.text)
        example = context.render(type_id)
        yield (
            f"{index:03d}. "
            f"owner={owner.label} "
            f'owner_text="{owner.text}" '
            f"declarator={declarator.label} "
            f'type_id="{type_id}" '
            f"context={context.label} "
            f'example="{example}"'
        )
        index += 1


def build_entries() -> list[dict[str, str]]:
    entries: list[dict[str, str]] = []
    index = 1
    for owner, declarator, context in product(OWNERS, DECLARATORS, CONTEXTS):
        type_id = declarator.render(owner.text)
        example = context.render(type_id)
        entries.append(
            {
                "index": f"{index:03d}",
                "owner_label": owner.label,
                "owner_text": owner.text,
                "declarator_label": declarator.label,
                "type_id": type_id,
                "context_label": context.label,
                "example": example,
            }
        )
        index += 1
    return entries


def format_entry(entry: dict[str, str]) -> str:
    return (
        f'{entry["index"]}. '
        f'owner={entry["owner_label"]} '
        f'owner_text="{entry["owner_text"]}" '
        f'declarator={entry["declarator_label"]} '
        f'type_id="{entry["type_id"]}" '
        f'context={entry["context_label"]} '
        f'example="{entry["example"]}"'
    )


def render_flat(entries: list[dict[str, str]]) -> str:
    return "\n".join(format_entry(entry) for entry in entries) + "\n"


def render_grouped(entries: list[dict[str, str]], group_by: str) -> str:
    assert group_by in {"owner", "declarator"}

    label_key = "owner_label" if group_by == "owner" else "declarator_label"
    value_key = "owner_text" if group_by == "owner" else "type_id"

    ordered_labels: list[str]
    if group_by == "owner":
        ordered_labels = [owner.label for owner in OWNERS]
    else:
        ordered_labels = [declarator.label for declarator in DECLARATORS]

    chunks: list[str] = []
    for label in ordered_labels:
        group_entries = [entry for entry in entries if entry[label_key] == label]
        if not group_entries:
            continue
        chunks.append(f"## {label}")
        sample_values: list[str] = []
        for entry in group_entries:
            sample = entry[value_key]
            if sample not in sample_values:
                sample_values.append(sample)
        chunks.append("samples: " + ", ".join(sample_values[:5]))
        for entry in group_entries:
            chunks.append(format_entry(entry))
        chunks.append("")
    return "\n".join(chunks).rstrip() + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--output",
        help="Write the generated matrix to this file instead of stdout.",
    )
    parser.add_argument(
        "--group-by",
        choices=("owner", "declarator"),
        help="Group output by a top-level matrix dimension.",
    )
    parser.add_argument(
        "--emit-grouped-files",
        action="store_true",
        help="Write flat, owner-grouped, and declarator-grouped files next to --output.",
    )
    args = parser.parse_args()

    entries = build_entries()
    if args.group_by:
        payload = render_grouped(entries, args.group_by)
    else:
        payload = render_flat(entries)

    if args.output:
        with open(args.output, "w", encoding="ascii") as f:
            f.write(payload)
        if args.emit_grouped_files:
            output_path = Path(args.output)
            stem = output_path.stem
            suffix = output_path.suffix
            owner_path = output_path.with_name(f"{stem}_by_owner{suffix}")
            declarator_path = output_path.with_name(
                f"{stem}_by_declarator{suffix}"
            )
            owner_path.write_text(render_grouped(entries, "owner"), encoding="ascii")
            declarator_path.write_text(
                render_grouped(entries, "declarator"), encoding="ascii"
            )
    else:
        print(payload, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
