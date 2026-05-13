# AArch64 Machine Node ASM Printer External Smoke

Status: Open
Created: 2026-05-13

Depends On:
- `ideas/closed/215_aarch64_first_machine_node_selection_slice.md`

## Goal

Implement the first AArch64 machine-node assembly printer and use an external
toolchain smoke test as the validation foundation.

The printer consumes structured machine instruction nodes and emits assembly
text that an external `clang` / `as` toolchain can assemble and run. It is not
an internal semantic handoff and must not become an input path for codegen.

## Why This Idea Exists

The backend can avoid building an internal assembler, encoder, ELF writer, and
linker in the early AArch64 phase. Once idea 215 can produce a small structured
machine-node subset, the fastest useful validation loop is:

```text
.c source
  -> c4c frontend / BIR / prepared module
  -> AArch64 target MIR
  -> AArch64 machine instruction nodes
  -> AArch64 .s printer
  -> external clang/as
  -> executable smoke test
```

This keeps the machine-node contract honest while deferring the internal
assembler/object pipeline.

## In Scope

- Add a user-facing c4cll assembly-output route in `src/apps/c4cll.cpp`,
  modeled after the existing HIR/LIR printer or dump routes. The LIR printer
  route is the closest reference because this slice needs an emission-boundary
  printer rather than an internal debug-only structure dump.
- Add a machine-node-to-assembly printer for the first subset accepted by idea
  215.
- Wire the CLI route through the real frontend path from `.c` source to BIR /
  prepared module / AArch64 target MIR / machine nodes before printing `.s`.
  The printer must not be a hand-authored fixture printer or a shortcut that
  bypasses the accepted backend pipeline.
- Print GNU/LLVM-compatible AArch64 assembly accepted by an external `clang` or
  `as` invocation in the supported test environment.
- Preserve the rule that assembly text is output only:
  - codegen must not parse the printed `.s`
  - encoder/object writer work must not depend on parsing printed `.s`
  - future internal assembler remains a separate contract
- Add a small set of smoke tests that start from `.c`, produce `.s`, compile
  the `.s` with an external `clang` toolchain, and run the resulting executable
  where the test environment supports AArch64 execution or emulation.
- Place the smoke testcase design near the existing LIR-printer style tests
  when practical, or document why a backend-specific test directory is a better
  fit. Testcases should exercise the public `c4cll` route rather than calling
  the printer as a private helper only.
- If the local environment cannot execute AArch64 binaries, document and test
  the strongest available fallback, such as external assembly plus object
  creation, while keeping the close condition tied to a runnable smoke path in
  the intended environment.
- Add focused printer formatting tests for the supported machine-node subset so
  unsupported nodes fail closed with clear diagnostics.

## Out Of Scope

- Implementing an internal assembler, instruction encoder, ELF writer, linker,
  relocation applier, or object-file reader.
- Printing every AArch64 instruction family.
- Expanding machine-node selection beyond the subset accepted by idea 215.
- Using assembly text as an internal semantic source.
- Supporting inline assembly or CISC-style optimization passes.

## Acceptance Criteria

- Structured AArch64 machine instruction nodes from the idea 215 subset can be
  printed to external-toolchain-compatible `.s`.
- `src/apps/c4cll.cpp` exposes a stable assembly-output option or mode for the
  AArch64 path, analogous in shape to the existing HIR/LIR printer routes and
  suitable for use by CTest smoke cases.
- At least a few simple smoke tests start from `.c`, emit `.s`, compile that
  `.s` with external `clang`, and run successfully in the supported validation
  environment.
- The smoke tests cover more than one trivial case, such as returning an
  integer constant, a scalar integer operation, and a simple branch or compare
  if those nodes are available from idea 215.
- Unsupported machine nodes fail closed rather than printing malformed
  assembly.
- No internal assembler, encoder, object writer, linker, or `.s` parser is
  introduced as part of this slice.

## Reviewer Reject Signals

- The route validates by parsing the emitted `.s` back into backend semantics.
- The printer becomes the semantic bridge between codegen and encoder/object
  writer.
- Smoke tests only snapshot text and never compile through an external
  toolchain when that toolchain is available.
- The c4cll route prints assembly by bypassing BIR/prepared, target MIR, or
  machine-node selection.
- The slice expands into internal assembler, ELF, linker, or broad instruction
  selection work.
- Tests prove only a single named fixture through hard-coded output rather than
  a general printer path for the accepted machine-node subset.
