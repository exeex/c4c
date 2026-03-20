#!/usr/bin/env python3

import argparse
import pathlib
import re
import subprocess
import sys
from dataclasses import dataclass
from typing import List, Optional


EXPECT_RE = re.compile(
    r"expected-(error|warning|note)"
    r"(?:@([+-]\d+))?"
    r"(?:\s+([0-9]+(?:\+|-[0-9]+)?))?"
    r"\s*\{\{(.*?)\}\}"
)

DIAG_RE = re.compile(r"^(.*?):(\d+):(\d+): (error|warning|note): (.*)$")
PARSE_ERROR_LINE_RE = re.compile(r"^parse error: (.*) at line (\d+)(?: .*)?$")
PARSE_ERROR_PLAIN_RE = re.compile(r"^parse error: (.*)$")


@dataclass
class Expectation:
    kind: str
    line: int
    text: str
    min_count: int
    max_count: Optional[int]


@dataclass
class Diagnostic:
    kind: str
    line: Optional[int]
    text: str


def line_matches(expected_line: int, actual_line: Optional[int]) -> bool:
    if actual_line is None:
        return True
    return abs(actual_line - expected_line) <= 1


def parse_expectations(src: pathlib.Path) -> List[Expectation]:
    expectations: List[Expectation] = []
    for lineno, raw in enumerate(src.read_text(encoding="utf-8").splitlines(), start=1):
        for match in EXPECT_RE.finditer(raw):
            kind, offset_text, count_text, text = match.groups()
            target_line = lineno + int(offset_text or "0")
            min_count = 1
            max_count: Optional[int] = 1
            if count_text:
                if count_text.endswith("+"):
                    min_count = int(count_text[:-1])
                    max_count = None
                elif "-" in count_text:
                    lo, hi = count_text.split("-", 1)
                    min_count = int(lo)
                    max_count = int(hi)
                else:
                    min_count = int(count_text)
                    max_count = min_count
            expectations.append(
                Expectation(
                    kind=kind,
                    line=target_line,
                    text=text,
                    min_count=min_count,
                    max_count=max_count,
                )
            )
    return expectations


def parse_diagnostics(stderr: str) -> List[Diagnostic]:
    diagnostics: List[Diagnostic] = []
    for raw in stderr.splitlines():
        line = raw.strip()
        if not line:
            continue
        m = DIAG_RE.match(line)
        if m:
            diagnostics.append(
                Diagnostic(kind=m.group(4), line=int(m.group(2)), text=m.group(5))
            )
            continue
        m = PARSE_ERROR_LINE_RE.match(line)
        if m:
            diagnostics.append(Diagnostic(kind="error", line=int(m.group(2)), text=m.group(1)))
            continue
        m = PARSE_ERROR_PLAIN_RE.match(line)
        if m:
            diagnostics.append(Diagnostic(kind="error", line=None, text=m.group(1)))
    return diagnostics


def match_expectations(expectations: List[Expectation], diagnostics: List[Diagnostic]) -> List[str]:
    failures: List[str] = []
    for exp in expectations:
        count = 0
        for diag in diagnostics:
            if diag.kind != exp.kind:
                continue
            if not line_matches(exp.line, diag.line):
                continue
            if exp.text not in diag.text:
                continue
            count += 1

        if count < exp.min_count:
            failures.append(
                f"missing {exp.kind} at line {exp.line}: wanted >= {exp.min_count} containing {exp.text!r}, got {count}"
            )
            continue
        if exp.max_count is not None and count > exp.max_count:
            failures.append(
                f"too many {exp.kind} at line {exp.line}: wanted <= {exp.max_count} containing {exp.text!r}, got {count}"
            )
    return failures


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--src", required=True)
    parser.add_argument("--out-ll", required=True)
    parser.add_argument("--expect-mode", choices=["pass", "fail", "verify"], required=True)
    args = parser.parse_args()

    src = pathlib.Path(args.src)
    out_ll = pathlib.Path(args.out_ll)
    out_ll.parent.mkdir(parents=True, exist_ok=True)

    proc = subprocess.run(
        [args.compiler, str(src), "-o", str(out_ll)],
        capture_output=True,
        text=True,
    )
    stdout = proc.stdout or ""
    stderr = proc.stderr or ""
    combined = (stdout + stderr).strip()

    if args.expect_mode == "pass":
      if proc.returncode != 0:
        sys.stderr.write(f"[FRONTEND_FAIL] {src}\n{combined}\n")
        return 1
      return 0

    if args.expect_mode == "fail":
      if proc.returncode == 0:
        sys.stderr.write(f"[UNEXPECTED_PASS] {src}\n")
        return 1
      return 0

    expectations = parse_expectations(src)
    if not expectations:
      sys.stderr.write(f"[VERIFY_NO_EXPECTATIONS] {src}\n")
      return 1

    diagnostics = parse_diagnostics(stderr)
    failures = match_expectations(expectations, diagnostics)

    if failures:
      sys.stderr.write(f"[VERIFY_FAIL] {src}\n")
      for failure in failures:
        sys.stderr.write(f"{failure}\n")
      if combined:
        sys.stderr.write("\n[compiler output]\n")
        sys.stderr.write(combined + "\n")
      return 1

    if proc.returncode != 0 and not any(exp.kind == "error" for exp in expectations):
      sys.stderr.write(f"[VERIFY_UNEXPECTED_FAILURE] {src}\n{combined}\n")
      return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
