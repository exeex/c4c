# AArch64 Scalar Return ALU Selected Nodes

Status: Open
Created: 2026-05-14

Depends On:
- `ideas/open/221_aarch64_markdown_driven_backend_case_bringup.md`
- `src/backend/mir/aarch64/BACKEND_CASE_BRINGUP_MATRIX.md`

## Goal

Implement the first AArch64 backend case feature group identified by the
bring-up matrix: scalar return/ALU lowering for `return_add.c`.

The intended route is:

```text
src/backend/mir/aarch64/codegen/alu.md
  + src/backend/mir/aarch64/codegen/returns.md
  -> src/backend/mir/aarch64/codegen/records.cpp/.hpp
  -> selected scalar machine-node/operator coverage
  -> src/backend/mir/aarch64/codegen/machine_printer.cpp
  -> tests/backend/case/return_add.c
```

## Why This Idea Exists

Idea 221 deliberately stops at the markdown-driven case matrix. The matrix
selects scalar return/ALU as the first next feature group because the route is
small, semantically meaningful, and already has clear markdown owners.

The implementation work is a separate initiative because it must add backend
capability before public case enablement. Keeping this idea separate prevents
the matrix plan from expanding into ad hoc implementation work and gives the
reviewer a concrete contract for rejecting testcase-shaped `return_add`
shortcuts.

## In Scope

- Convert the relevant scalar `alu.md` and `returns.md` contracts into
  structured implementation coverage in `records.cpp` and `records.hpp`.
- Add or strengthen focused C++ tests for scalar ALU records, return payload
  records, natural operator names, and printable machine-node/operator
  coverage.
- Add semantic instruction selection for simple integer add/sub results that
  are returned through the normal AArch64 backend path.
- Ensure `machine_printer.cpp` prints the selected add/sub and return sequence
  through structured machine nodes/operators, not fixture text.
- Enable or add the public `tests/backend/case/return_add.c` smoke only after
  structured selected-node/operator/printer coverage exists.
- Keep the public smoke on `c4cll --codegen asm --target
  aarch64-linux-gnu`, with emitted assembly checks plus external assembler and
  compiler proof where applicable.

## Out Of Scope

- Enabling `return_add_sub_chain.c`, two-argument ALU fixtures, builtin abs
  cases, pointer returns, or broader arithmetic cases unless they fall out of
  the same structured route without extra named-case handling.
- Implementing comparison, memory, call, globals, aggregate, variadic, inline
  asm, FP/SIMD, atomic, intrinsic, i128, or linker/object behavior.
- Replacing the terminal `.s` printer route with the internal assembler,
  encoder, object writer, linker, or binary-output route.
- Downgrading existing enabled AArch64 smoke expectations or weakening failure
  guards.
- Editing the matrix plan from idea 221 except for lifecycle closure or
  handoff bookkeeping.

## Acceptance Criteria

- Scalar return/ALU behavior is owned by the markdown route:
  `alu.md` plus `returns.md` to `records.cpp/.hpp`, selected machine
  nodes/operators, `machine_printer.cpp`, then `return_add.c`.
- Focused C++ coverage proves the structured records, selected operator kinds,
  natural mnemonic spelling, and return payload/return node behavior needed by
  the public case.
- The public `return_add.c` smoke is enabled only after selected add and return
  output are produced through the normal backend path.
- The smoke checks real AArch64 assembly shape, including an `add` operation
  and `ret`, and uses external assemble/link/run proof according to the
  existing host-gated smoke model.
- `return_add_sub_chain.c` and broader scalar ALU fixtures remain deferred
  unless the implementation naturally supports them through the same semantic
  route and they receive matching proof.
- No supported case is weakened, marked unsupported, or hidden behind looser
  expectations to claim progress.

## Reviewer Reject Signals

- The diff adds testcase-shaped `return_add` shortcuts, named-case matching, or
  fixture-specific MIR/codegen/printer text whose main effect is making only
  `tests/backend/case/return_add.c` pass.
- The route enables `return_add.c` before structured selected-node/operator and
  `machine_printer.cpp` coverage exists for scalar add plus returned result
  emission.
- The slice claims capability progress through expectation downgrade,
  unsupported marking, weaker smoke assertions, helper renames, or
  classification-only changes.
- The implementation bypasses the markdown-owned route from `alu.md` and
  `returns.md` through `records.cpp/.hpp`, selected machine nodes/operators,
  and the terminal machine printer.
- The public smoke no longer uses the normal `c4cll --codegen asm --target
  aarch64-linux-gnu` route or drops external assembler/compiler proof where the
  case is expected to assemble and link.
- Broader ALU, pointer, memory, call, globals, aggregate, variadic, or
  assembler/linker behavior is pulled into this idea without a separate
  markdown-owned contract.
- The exact old failure mode is retained behind a new abstraction name, such
  as producing printable output without real selected scalar machine nodes.
