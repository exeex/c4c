# AArch64 Machine Node ASM Printer External Smoke

Status: Active
Source Idea: ideas/open/216_aarch64_machine_node_asm_printer_external_smoke.md

## Purpose

Implement the first AArch64 structured machine-node assembly printer and prove
it through an external-toolchain smoke path.

## Goal

Expose a user-facing `c4cll` route that takes `.c` input through the real
frontend, prepared BIR, AArch64 target MIR, and selected machine nodes, then
prints GNU/LLVM-compatible AArch64 `.s` for external `clang` / `as`
validation.

## Core Rule

The `.s` printer is an output consumer of structured machine nodes only. It
must not become a semantic input path for codegen, an internal assembler, an
encoder, an object writer, a linker, or a parser-backed handoff.

## Read First

- `ideas/open/216_aarch64_machine_node_asm_printer_external_smoke.md`
- `src/apps/c4cll.cpp`
- `src/backend/mir/aarch64/MACHINE_INSTRUCTION_NODE_CONTRACT.md`
- `src/backend/mir/aarch64/module/module.hpp`
- `src/backend/mir/aarch64/module/module.cpp`
- `src/backend/mir/aarch64/codegen/records.hpp`
- `src/backend/mir/aarch64/codegen/records.cpp`
- existing AArch64 machine-node selection code and tests from idea 215
- existing `c4cll` HIR/LIR/BIR/MIR dump or printer routes
- existing focused backend and CLI tests under `tests/`

## Current Targets

- a machine-node-to-AArch64-assembly printer for the subset accepted by idea
  215
- a stable `c4cll` assembly-output mode or option for the AArch64 path
- route wiring from `.c` source through the accepted backend pipeline before
  printing `.s`
- focused printer-formatting and unsupported-node tests
- smoke tests that emit `.s`, assemble or compile it with an external
  toolchain, and run it when the environment supports AArch64 execution or
  emulation

## Non-Goals

- Do not implement an internal assembler, instruction encoder, ELF writer,
  linker, relocation applier, object reader, or `.s` parser.
- Do not expand machine-node selection beyond the subset accepted by idea 215.
- Do not hand-author fixture assembly as the main route proof.
- Do not bypass BIR/prepared BIR, AArch64 target MIR, or machine-node
  selection when serving the public CLI route.
- Do not add inline assembly support, broad instruction-family printing, or
  CISC-style optimization work.

## Working Model

The accepted handoff for this plan is:

```text
.c source
  -> c4c frontend / BIR / prepared module
  -> AArch64 target MIR
  -> AArch64 machine instruction nodes
  -> AArch64 .s printer
  -> external clang/as
  -> executable smoke test when supported
```

Unsupported machine nodes must fail closed with clear diagnostics rather than
printing malformed assembly. If local AArch64 execution is unavailable, the
fallback proof should still compile or assemble the printed `.s` externally and
record the missing execution capability in `todo.md`.

## Execution Rules

- Start by inspecting the accepted idea 215 machine-node subset and current
  `c4cll` printer routes before making implementation edits.
- Keep printer output compatible with external GNU/LLVM AArch64 tools in the
  supported test environment.
- Keep the printer structurally downstream of machine nodes. Do not make
  assembly text an input to backend semantics or future encoder/object work.
- Add focused unsupported-node coverage so the printer fails closed.
- Use public `c4cll` route tests for smoke coverage where practical; private
  helper tests alone are not sufficient.
- For code-changing steps, run a fresh build plus the supervisor-selected
  focused proof. Escalate validation if CLI or backend route changes affect
  shared behavior beyond the AArch64 assembly-output path.

## Steps

### Step 1: Inspect Printer Inputs And CLI Route Shape

Goal: identify the exact machine-node subset, route boundary, and test harness
available for the first `.s` printer.

Primary target:
idea 215 machine-node surfaces, `src/apps/c4cll.cpp`, and existing printer or
dump tests.

Actions:

- Inspect the structured machine-node representation and accepted first subset
  from idea 215.
- Inspect existing `c4cll` dump/printer routes, especially LIR-style output
  boundaries.
- Identify where the AArch64 source-to-machine-node pipeline can be invoked for
  a public assembly-output route.
- Identify the local availability of external AArch64 `clang` / `as` and any
  runnable AArch64 execution or emulation path.
- Record the first safe printer subset, route target, smoke-test location, and
  environment limits in `todo.md`.

Completion check:
`todo.md` records the accepted machine-node subset, planned CLI route shape,
test placement, and external-toolchain execution capability without
implementation edits.

### Step 2: Add The Minimal AArch64 Machine-Node Printer

Goal: print the accepted structured machine-node subset as
external-toolchain-compatible AArch64 assembly.

Primary target:
the AArch64 MIR/codegen/module boundary selected in Step 1.

Actions:

- Add a printer surface that consumes structured machine instruction nodes, not
  target-MIR records or assembly strings.
- Print GNU/LLVM-compatible function, label, instruction, register, immediate,
  and directive text required by the accepted subset.
- Preserve output ordering and labels from the machine-node stream.
- Return clear unsupported-node diagnostics for unhandled node forms.
- Keep the printer independent from any future assembler, encoder, object, or
  linker pipeline.

Completion check:
Focused tests can print representative accepted nodes and unsupported nodes
fail closed without malformed assembly.

### Step 3: Wire The Public c4cll Assembly-Output Route

Goal: expose the printer through a stable `c4cll` option or mode that starts
from `.c` input and reaches the printer through the real AArch64 backend
pipeline.

Primary target:
`src/apps/c4cll.cpp` and nearby CLI route tests.

Actions:

- Add or stabilize the user-facing assembly-output flag or mode for the
  AArch64 path.
- Require or document the target triple / target selection needed to choose
  AArch64.
- Route through frontend parsing, BIR/prepared module construction, AArch64
  target MIR, and machine-node selection before printing `.s`.
- Support writing assembly to stdout or an output path following existing
  `c4cll` conventions.
- Reject unsupported route combinations with clear diagnostics.

Completion check:
CLI tests exercise the public route from `.c` input to printed `.s` and prove
it does not bypass the accepted backend pipeline.

### Step 4: Add External Toolchain Smoke Tests

Goal: validate printed assembly through an external AArch64 toolchain and run
the result when supported.

Primary target:
repo-native CTest or CLI smoke tests selected in Step 1.

Actions:

- Add smoke cases that start from `.c`, emit `.s` through `c4cll`, and compile
  or assemble that `.s` with external `clang` / `as`.
- Cover more than one trivial case, such as an integer constant, scalar integer
  operation, and simple branch or compare when those nodes are available.
- Run the produced executable when the environment supports AArch64 execution
  or emulation.
- If execution is unavailable locally, keep the strongest available external
  assembly/object proof and record the remaining runnable-smoke requirement in
  `todo.md`.

Completion check:
Smoke tests prove the public `.c` -> `.s` -> external toolchain path for the
accepted subset, with runnable proof where the environment supports it.

### Step 5: Reconcile Failure Boundaries And Documentation

Goal: make the new printer route discoverable while preserving the boundary
between assembly output and future internal assembler/encoder work.

Primary target:
AArch64 backend docs, relevant CLI help text, and nearby test documentation.

Actions:

- Document the `.s` printer as final/debug output from structured machine
  nodes.
- Clarify that codegen does not parse printed `.s` and future encoder/object
  work must not depend on parsing printed `.s`.
- Update CLI help or route docs for the exact user-facing flag or mode.
- Keep broader debug-flag skill documentation for idea 217; only add local
  route documentation needed by this implementation.

Completion check:
Docs and help text describe the implemented route without implying internal
assembler, encoder, object, linker, or `.s` parser support.

### Step 6: Add Focused Proof And Handoff Notes

Goal: leave the printer slice validated and ready for the follow-up
debug-flags documentation idea.

Primary target:
the supervisor-selected build and focused AArch64/CLI test subset.

Actions:

- Run a fresh build or compile proof for touched code.
- Run focused printer, CLI, and external-toolchain smoke tests.
- Escalate to broader validation if shared CLI, frontend, or backend route
  changes affect non-AArch64 behavior.
- Record final proof commands, environment limitations, and the exact
  implemented `c4cll` command shape in `todo.md` for idea 217.

Completion check:
Fresh proof supports the assembly-output route, no expectation downgrades or
testcase-shaped shortcuts are present, and `todo.md` records the exact command
shape needed by the debug-flags follow-up.
