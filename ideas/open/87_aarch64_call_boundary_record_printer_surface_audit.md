# AArch64 Call-Boundary Record And Printer Surface Audit

## Goal

Investigate the adjacent cleanup left by idea 82 by auditing call-boundary and
aggregate-lane surfaces that cross `calls.cpp`, `instruction.*`, and
`machine_printer.cpp`.

## Why This Exists

Idea 82 exhausted clear instruction/printer table-driving candidates and said
remaining adjacent cleanup should start from a separate source idea. Idea 84
also retained call-boundary helpers because they own AArch64 ABI, diagnostics,
scratch choices, and machine-record construction.

The overlap suggests an evidence route before implementation: call-boundary and
aggregate-lane lowering may have duplicated record/printer validation surfaces,
but those surfaces may also be correct target-local ABI and printer ownership.

## Owned Files

- Read/audit:
  - `src/backend/mir/aarch64/codegen/calls.cpp`
  - `src/backend/mir/aarch64/codegen/calls.hpp`
  - `src/backend/mir/aarch64/codegen/instruction.cpp`
  - `src/backend/mir/aarch64/codegen/instruction.hpp`
  - `src/backend/mir/aarch64/codegen/machine_printer.cpp`
  - `src/backend/mir/aarch64/codegen/machine_printer.hpp`
- No implementation files should change unless a later generated idea
  explicitly owns a narrower slice.

## In Scope

- Trace call-boundary move records from call lowering through instruction
  records to printer output.
- Trace aggregate-lane publication records through instruction records and
  printer output.
- Classify repeated validation or spelling helpers as:
  - target-local ABI/printer ownership
  - local table/helper contraction candidate
  - call-boundary record schema cleanup candidate
  - missing evidence
- Produce implementation follow-up ideas only for concrete, narrow candidates.

## Out Of Scope

- Continuing broad instruction/printer table-driving from idea 82.
- Moving concrete AArch64 call-boundary or aggregate-lane spelling into shared
  BIR/prealloc.
- Refactoring all of `calls.cpp`.
- Changing assembly output or diagnostics without a dedicated implementation
  idea and proof.

## Proof Expectations

- This is audit-only; implementation proof is not required unless files are
  changed.
- Any generated implementation idea must name a focused proof set covering
  call-boundary moves, aggregate-lane publication, instruction records, and
  machine printer output.

## Reviewer Reject Signals

- The route claims printer cleanup by deleting validation.
- It moves ABI-specific call-boundary construction into shared authority.
- It proposes broad calls/printer rewrites without a traced record path.
- It ignores idea 82's conclusion that obvious table-driving candidates are
  already exhausted.

