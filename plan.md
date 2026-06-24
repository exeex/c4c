# RV64 EV `.insn.d` Inline Asm Stage 3 Runbook

Status: Active
Source Idea: ideas/open/342_rv64_ev_insn_d_inline_asm_stage3.md
Activated from: ideas/open/339_rv64_inline_asm_custom_vector_encoding_umbrella.md

## Purpose

Add the EV 64-bit custom instruction inline asm route now that scalar `.insn`
and vector register constraints are available foundations.

## Goal

Implement positional `.insn.d` inline asm lowering and target-owned 64-bit
object emission for EV custom vector instructions.

## Core Rule

Do not prove `.insn.d` support through raw-string acceptance, external
assembler delegation, expectation weakening, or a single sample-string
shortcut. The compiler must own operand substitution, immediate validation,
field encoding, and object bytes.

## Read First

- `ideas/open/342_rv64_ev_insn_d_inline_asm_stage3.md`
- `ideas/open/339_rv64_inline_asm_custom_vector_encoding_umbrella.md`
- `ideas/open/340_rv64_standard_insn_inline_asm_stage1.md`
- `ideas/closed/341_rv64_vector_register_inline_asm_constraints_stage2.md`
- Existing RV64 inline asm `.insn` implementation, vector constraint support,
  substitution tests, and object emission tests.

## Current Targets

- Positional `.insn.d` template parsing and lowering.
- Compile-time immediate fields for EV namespace/op/dtype or policy data.
- Scalar and vector register operand substitution, including grouped vector
  base-register encoding.
- Target-owned 64-bit instruction byte emission.
- Diagnostics for malformed templates, unsupported operand classes,
  non-constant immediates, and out-of-range fields.

## Non-Goals

- Do not add GNU named operands or `%c[...]` modifiers.
- Do not add consteval or template-built asm strings.
- Do not implement runtime-generated asm strings.
- Do not add broad RVV lowering, vector language semantics, or EV operation
  semantics.
- Do not add linker-visible relocations inside `.insn.d` instructions.
- Do not reopen Stage 1 or Stage 2 except to preserve compatibility with their
  completed behavior.

## Working Model

Treat `.insn.d` as a target-owned encoding template. Inline asm still provides
the instruction spelling, operands, and compile-time immediate values; the RV64
backend validates the supported positional shape and emits the resulting 64-bit
instruction bytes.

## Execution Rules

- Keep each implementation packet focused on one observable capability.
- Prefer extending existing RV64 inline asm helpers before adding parallel
  paths.
- Add negative tests with every acceptance path that could otherwise overfit.
- Preserve existing scalar `.insn` and vector constraint tests.
- For code-changing steps, run `cmake --build --preset default` and the
  supervisor-delegated CTest subset before acceptance.

## Step 1: Map `.insn.d` Extension Surfaces

Goal: identify the exact parser, carrier, substitution, encoder, object, and
test surfaces Stage 3 must extend.

Primary targets:

- RV64 inline asm `.insn` parser and template classifier.
- Inline asm operand/immediate carrier through HIR/LIR/BIR as needed.
- RV64 operand substitution for scalar and vector registers.
- RV64 object emission helpers for fixed-width instruction bytes.
- Existing Stage 1 and Stage 2 backend tests.

Actions:

- Inspect the completed Stage 1 scalar `.insn` route before editing.
- Inspect the completed Stage 2 vector constraint and substitution route.
- Identify where immediate operands are represented and whether `i` constraint
  values are already compile-time checked.
- Identify the minimal object writer extension needed for a fixed 64-bit
  instruction.
- Record any frontend/HIR carrier gap in `todo.md`; only expand the runbook if
  `.insn.d` cannot proceed without a contract rewrite.

Completion check:

- Executor can name the files and functions that own `.insn.d` classification,
  operand/immediate extraction, register substitution, 64-bit byte emission,
  and proof tests.

## Step 2: Define And Test The Positional `.insn.d` Shape

Goal: recognize the supported `.insn.d` form and reject unsupported syntax
before encoding.

Actions:

- Add a target-owned parser/classifier for the first positional `.insn.d`
  operand shape.
- Keep named operands and `%c[...]` modifiers rejected or out of scope.
- Validate operand count and placeholder references.
- Validate that immediate fields are compile-time constants.
- Add tests for accepted positional syntax and malformed or unsupported forms.

Completion check:

- Focused tests prove the supported `.insn.d` shape reaches a structured
  target-owned representation, and malformed or unsupported forms diagnose
  clearly.

## Step 3: Encode EV 64-Bit Fields

Goal: map validated `.insn.d` operands and immediates into deterministic
64-bit instruction bytes.

Actions:

- Implement the selected EV field layout from the source idea:
  opcode/namespace, operation selector, dtype or policy immediate, register
  fields, minor function fields, and opcode marker.
- Enforce each field's width and reject out-of-range values.
- Use allocated scalar and vector registers for register fields; grouped
  vector operands must encode the base register.
- Keep relocation-bearing operands out of the initial route.
- Add emitted-byte tests against known expected 64-bit encodings.

Completion check:

- Tests prove correct byte emission for at least one `.insn.d` case with
  vector operands and immediate fields, plus range diagnostics for invalid
  fields.

## Step 4: Integrate With Object Emission And Existing Inline Asm Behavior

Goal: ensure `.insn.d` travels through the same compiler-owned object route as
the completed `.insn` support.

Actions:

- Wire the encoded 64-bit instruction into RV64 object emission.
- Preserve `asm volatile`, clobber, and memory metadata.
- Preserve existing scalar `.insn` behavior and Stage 2 vector constraint
  behavior.
- Add mixed proof coverage where `.insn` and `.insn.d` coexist if the test
  surface supports it.

Completion check:

- Backend tests show `.insn.d` object bytes are emitted by c4c and existing
  `.insn` and vector-constraint tests still pass.

## Step 5: Validate Stage 3 And Record Remaining Children

Goal: prove Stage 3 without expanding into named operands, masks, or consteval
strings.

Actions:

- Run the build command selected by the supervisor.
- Run the narrow backend CTest subset selected by the supervisor.
- Add frontend/HIR CTest only if parser or carrier layers changed.
- Confirm negative tests cover malformed syntax, unsupported operands,
  non-constant immediates, invalid widths, and relocation-like cases.
- Record any next-child follow-up in `todo.md` instead of editing the umbrella
  unless source intent has changed.

Completion check:

- Fresh proof logs show the delegated build/test subset is green or only fails
  for supervisor-accepted unrelated baseline failures, and `todo.md` records
  the exact proof command and result.
