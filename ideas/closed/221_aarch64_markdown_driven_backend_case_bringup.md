# AArch64 Markdown Driven Backend Case Bringup

Status: Closed
Created: 2026-05-14
Closed: 2026-05-14

Depends On:
- `ideas/closed/216_aarch64_machine_node_asm_printer_external_smoke.md`
- `ideas/closed/218_aarch64_structured_asm_encoder_linker_contract.md`
- `ideas/closed/219_aarch64_natural_operator_naming_and_printer_spelling.md`

## Completion Notes

Closed after committing
`src/backend/mir/aarch64/BACKEND_CASE_BRINGUP_MATRIX.md` as the durable
markdown-owned backend case routing artifact. The matrix maps relevant
`tests/backend/case` files to AArch64 markdown owners, structured implementation
owners, dependent machine-node/operator coverage, public smoke expectations,
status labels, and deferred or blocked reasons.

The selected first follow-up implementation group is scalar return/ALU and is
tracked separately in
`ideas/open/222_aarch64_scalar_return_alu_selected_nodes.md`. Future AArch64
case work should pick one markdown owner at a time from the matrix and keep
case enablement behind structured implementation coverage, public `c4cll`
assembly output, and external assembler/compiler proof where applicable.

## Goal

Reopen and grow AArch64 backend case coverage under `tests/backend/case` using
a markdown-driven implementation strategy.

Each backend feature should be brought up by converting the relevant
`src/backend/mir/aarch64/**/*.md` design surface into structured `.cpp/.hpp`
behavior first, then enabling or adding the corresponding backend case smoke
test. Tests should validate architecture progress, not dictate ad hoc
testcase-shaped code.

## Why This Idea Exists

The previous AArch64 bring-up failed because implementation often chased the
next failing testcase directly. That caused overfitting: narrow fixtures became
the design authority, and code paths grew around observed case shapes instead
of around the backend contracts.

The new route has stronger contracts:

```text
BIR / prepared facts
  -> target MIR records
  -> machine instruction nodes
  -> terminal .s printer for smoke validation
  -> future structured asm/encoding/object/linker records
```

The test suite should now be reopened through that structure. A `.c` case in
`tests/backend/case` becomes enabled when the matching AArch64 markdown feature
has a structured implementation path.

## In Scope

- Audit existing `tests/backend/case` AArch64-relevant cases and classify them
  by the AArch64 markdown feature that must own them, such as:
  - `codegen/returns.md`
  - `codegen/alu.md`
  - `codegen/comparison.md`
  - `codegen/memory.md`
  - `codegen/calls.md`
  - `codegen/prologue.md`
  - `codegen/globals.md`
  - later assembler/encoder/linker markdown only after the structured contract
    allows it
- Define an ordered bring-up matrix:
  - markdown owner
  - required structured `.cpp/.hpp` owner
  - dependent machine-node/operator coverage
  - backend case files
  - external `clang`/`as` smoke expectation
  - unsupported/deferred reason when not ready
- Establish the rule that case tests are opened only after the corresponding
  markdown feature has been converted into structured implementation.
- Prefer small, semantic case groups over one-off fixture matching.
- Keep real feature smoke tests under `tests/backend/case`.
- Keep record/model unit tests under focused `tests/backend/backend_aarch64_*`
  C++ tests when a feature needs internal record coverage.
- Update CMake test wiring only to reflect the markdown-driven case matrix and
  the external assembler/compiler proof route.

## Out Of Scope

- Implementing all AArch64 backend features in one slice.
- Enabling a large batch of backend cases before their markdown owners have
  structured implementations.
- Rewriting expectations to make unsupported cases appear supported.
- Adding testcase-shaped MIR/codegen shortcuts.
- Using external `.s` or `.ll` as backend input.
- Implementing internal assembler, encoder, object writer, linker, or binary
  output as part of this test bring-up idea.

## Acceptance Criteria

- A committed bring-up matrix maps relevant `tests/backend/case` files to
  AArch64 markdown owners and structured implementation owners.
- The matrix states which cases are currently enabled, deferred, or blocked,
  and why.
- At least the first next feature group has a clear `.md -> .cpp/.hpp -> case`
  route identified.
- The test strategy explicitly rejects testcase-overfit work and expectation
  downgrades.
- Backend case smoke tests use the public c4cll route and external
  assembler/compiler validation established by idea 216 where appropriate.
- Future ideas can pick one markdown owner at a time and open its matching
  case group without rediscovering the whole test strategy.

## Reviewer Reject Signals

- Tests are enabled before the corresponding markdown feature has a structured
  implementation route.
- Implementation is shaped around one fixture instead of the markdown-owned
  feature contract.
- Case expectations are weakened or rewritten instead of adding backend
  capability.
- The route bypasses BIR/prepared, target MIR, machine nodes, or the terminal
  `.s` printer smoke path.
- The plan treats `tests/backend/case` as a random backlog instead of a
  markdown-owned feature matrix.
