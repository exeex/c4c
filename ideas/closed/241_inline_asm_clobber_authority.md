# Inline Assembly Structured Clobber Authority

Status: Closed
Created: 2026-05-15
Closed: 2026-05-15

Parent Context: ideas/closed/240_aarch64_inline_asm_machine_nodes.md

## Goal

Represent inline-asm clobbers as structured source, LIR, BIR, prepared, and
selected-machine facts so AArch64 inline assembly can carry clobber authority
without recovering semantics from template text or printed assembly.

## Why This Exists

The AArch64 inline-asm machine-node route can now preserve operands, names,
immediates, modifiers, side effects, outputs, and concrete prepared-home ties.
Clobbers remain diagnostic-only because upstream inline-asm records do not yet
provide source/LIR/BIR structured clobber authority for the backend to consume.

## In Scope

- Define the source and LIR representation for inline-asm clobber lists.
- Preserve clobber facts through BIR inline-asm records.
- Carry clobbers through prepared records and AArch64 selected inline-asm
  machine records.
- Diagnose unsupported or target-invalid clobber spellings explicitly.
- Prove at least one supported AArch64 clobber representative from structured
  authority, plus nearby fail-closed cases.

## Out Of Scope

- Do not parse clobbers from rendered template text or final assembler output.
- Do not invent target scratch-register allocation policy while adding clobber
  carriage.
- Do not weaken existing inline-asm unsupported diagnostics or expectation
  contracts.
- Do not fold memory/address constraints or tied-home coallocation policy into
  this clobber work.

## Acceptance Criteria

- Source/LIR/BIR inline-asm records expose structured clobber facts.
- Prepared and selected AArch64 inline-asm records retain those clobber facts
  without string-derived reconstruction.
- Backend tests prove supported clobber carriage and unsupported clobber
  diagnostics.
- Existing supported inline-asm operand, name, immediate, modifier, side-effect,
  output, and tied-home tests continue to pass.
- A regression guard over the supervisor-selected scope passes.

## Completion Notes

Closed after Step 4 lifecycle review. Structured inline-asm clobbers are carried
from source/LIR through BIR, prepared records, and selected AArch64 machine
records without reconstructing clobber authority from template text, final
assembly, or diagnostic strings. Supported AArch64 clobber representatives are
covered, and unsupported or target-invalid clobbers remain fail-closed with
diagnostics. Memory/address constraints and tied-home allocation policy remain
separate open ideas.

Closure proof:

- Supervisor full-suite validation at current `HEAD`: `3167/3167` tests passed
  in `test_after.log`.
- Accepted full-suite baseline in `test_before.log`: `3167/3167` tests passed.
- Close-time regression guard:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
  passed with no new failing tests.

## Reviewer Reject Signals

- Reject testcase-shaped matching that recognizes one clobber fixture instead
  of adding a general clobber carrier.
- Reject any supported-path claim where clobbers are parsed from final assembly,
  template text, or diagnostic strings rather than source/LIR/BIR facts.
- Reject unsupported expectation downgrades, skipped tests, or weaker
  diagnostics used to claim clobber progress.
- Reject helper renames, classification-only rewrites, or printer-only changes
  claimed as structured clobber support.
- Reject broad allocator, memory/address, or unrelated lowering rewrites hidden
  inside this clobber route.
- Reject a new abstraction that still leaves clobbers absent from the structured
  backend record consumed by AArch64 selection.
