# RV64 Vector Register Inline Asm Constraints Stage 2 Runbook

Status: Active
Source Idea: ideas/open/341_rv64_vector_register_inline_asm_constraints_stage2.md
Activated from: ideas/open/339_rv64_inline_asm_custom_vector_encoding_umbrella.md

## Purpose

Make RV64 inline asm vector operands first-class enough for later EV `.insn.d`
work.

## Goal

Implement `VR`, `VRM2`, and `VRM4` inline asm constraints with real vector
register-class allocation, overlap rules, and base-register substitution.

## Core Rule

Do not prove vector constraint support by accepting raw constraint strings,
weakening scalar `.insn` behavior, or adding testcase-shaped shortcuts. The
compiler must own register class, occupancy, alignment, tied-operand, and
substitution semantics.

## Read First

- `ideas/open/341_rv64_vector_register_inline_asm_constraints_stage2.md`
- `ideas/open/339_rv64_inline_asm_custom_vector_encoding_umbrella.md`
- Existing RV64 inline asm lowering, constraint parsing, register allocation,
  and object/substitution tests from the completed Stage 1 route.

## Current Targets

- Constraint classification for `VR`, `=VR`, `+VR`, `VRM2`, `=VRM2`, `+VRM2`,
  `VRM4`, `=VRM4`, and `+VRM4`.
- RV64 vector register allocation over `v0` through `v31`.
- Group occupancy:
  - `VR`: width 1, alignment 1.
  - `VRM2`: width 2, alignment 2.
  - `VRM4`: width 4, alignment 4.
- Placeholder substitution that prints the selected base vector register.
- Diagnostics for malformed, unsupported, misaligned, overlapping, or
  impossible vector constraint cases.

## Non-Goals

- Do not implement EV `.insn.d` encoding in this runbook.
- Do not add named operand or `%c[...]` modifier support.
- Do not add mask-specific constraints or policy bits.
- Do not broaden into RVV semantic lowering or custom EV operation semantics.
- Do not reopen Stage 1 except to preserve compatibility with the existing
  scalar `.insn` route.

## Working Model

Treat vector constraints as target-owned physical-register classes, not string
decorations. Grouped operands reserve all occupied registers, but inline asm
template substitution uses the group base register because later encoders need
the base field.

## Execution Rules

- Keep each implementation packet focused on one observable capability.
- Prefer shared inline asm helper extension only where it directly supports the
  vector constraint contract.
- Add negative tests with every acceptance path that could otherwise overfit.
- Preserve existing scalar `.insn` tests and expectations.
- For code-changing steps, run `cmake --build --preset default` and the
  supervisor-delegated CTest subset before acceptance.

## Step 1: Map Stage 1 Inline Asm Surfaces

Goal: identify the exact classification, lowering, allocation, substitution,
and test surfaces that Stage 2 must extend.

Primary targets:

- RV64 inline asm constraint parser/classifier.
- LIR/BIR inline asm metadata carrier.
- RV64 prepared/register-allocation path.
- RV64 inline asm substitution and object/test fixtures.

Actions:

- Inspect the Stage 1 `.insn` implementation and tests before editing.
- Find where scalar `r`, `=r`, and `+r` constraints become allocatable
  operands.
- Identify whether vector constraints need frontend/HIR carrier changes or can
  stay in backend-target classification.
- Record any discovered source-language value representation limit in
  `todo.md` instead of expanding this runbook unless it blocks Stage 2.

Completion check:

- Executor can name the files and functions that own vector constraint
  classification, allocation, substitution, and proof tests.

## Step 2: Add Vector Constraint Classification

Goal: classify `VR`, `VRM2`, and `VRM4` forms as vector register-class inline
asm operands.

Actions:

- Add target-owned classification for input, output, and read-write forms:
  `VR`, `=VR`, `+VR`, `VRM2`, `=VRM2`, `+VRM2`, `VRM4`, `=VRM4`, `+VRM4`.
- Preserve scalar constraint behavior.
- Reject malformed vector constraints explicitly.
- Add tests proving vector constraints are represented as vector operands, not
  scalar GPRs and not generic unsupported strings.

Completion check:

- Focused tests show the supported vector constraint tokens reach the expected
  operand kind, and malformed or unsupported tokens diagnose clearly.

## Step 3: Implement Group Allocation And Overlap Rules

Goal: allocate vector operands with correct width, alignment, occupancy, and
tied/read-write semantics.

Actions:

- Model `VR` as width 1, alignment 1 over `v0` through `v31`.
- Model `VRM2` as width 2, alignment 2 over `v0,v2,...,v30`.
- Model `VRM4` as width 4, alignment 4 over `v0,v4,...,v28`.
- Reserve every occupied register in a group.
- Reject impossible allocation and accidental overlap for untied operands.
- Preserve intended overlap for tied operands and `+VR*` read-write operands.
- Add positive and negative allocator tests.

Completion check:

- Tests prove legal bases, full group reservation, no untied overlap, allowed
  tied reuse, and explicit impossible-allocation failure.

## Step 4: Implement Base-Register Substitution

Goal: substitute grouped vector operands into inline asm templates using the
allocated base vector register.

Actions:

- Extend positional placeholder substitution for vector register operands.
- Ensure `VRM2` and `VRM4` print the base register, not all members and not a
  non-base member.
- Add tests covering `%0`, `%1`, and mixed scalar/vector substitution where
  useful.
- Keep the substituted template as target-owned intermediate text; do not
  claim EV `.insn.d` object encoding in this step.

Completion check:

- Focused substitution tests show stable base-register spellings for `VR`,
  `VRM2`, and `VRM4` operands.

## Step 5: Validate Stage 2 Against Stage 1 And Negative Cases

Goal: prove the vector constraint work is integrated without weakening the
completed scalar `.insn` path.

Actions:

- Run the build command selected by the supervisor.
- Run the narrow backend CTest subset selected by the supervisor.
- Add frontend/HIR CTest only if parser or carrier layers changed.
- Check that existing scalar `.insn` expectations remain supported.
- Confirm negative tests reject malformed constraints, overlap, misalignment,
  and impossible allocation.

Completion check:

- Fresh proof logs show the delegated build/test subset is green or only fails
  for supervisor-accepted unrelated baseline failures, and `todo.md` records
  the exact proof command and result.
