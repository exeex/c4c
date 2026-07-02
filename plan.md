# RV64 Object Encoding And Byte Helper Cleanup Runbook

Status: Active
Source Idea: ideas/open/534_rv64_object_encoding_byte_helpers_cleanup.md

## Purpose

Reduce low-level RV64 object-emission helper coupling by extracting or sharing
pure instruction encoding and little-endian byte append helpers without
changing emitted object bytes, object-route public behavior, or structured
fragment/fixup ownership.

## Goal

Make the U/I/S/R/B/J encoder and byte append helper boundary smaller and
reviewable while preserving current object emission behavior exactly.

## Core Rule

This is a behavior-preserving cleanup only. Do not change RV64 capability,
object bytes, relocations, labels, fixups, unsupported contracts, gcc_torture
expectations, or runtime comparisons.

## Read First

- `ideas/open/534_rv64_object_encoding_byte_helpers_cleanup.md`
- `docs/rv64_object_emission_cleanup/structure_baseline.md`
- `docs/rv64_object_emission_cleanup/aarch64_comparison.md`
- `docs/rv64_object_emission_cleanup/staged_followups.md`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/object_emission.hpp`
- `src/backend/mir/riscv/codegen/rv64_line_assembler.cpp`
- `src/backend/mir/riscv/codegen/rv64_line_assembler.hpp`

## Current Targets

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/object_emission.hpp`
- `src/backend/mir/riscv/codegen/rv64_line_assembler.cpp`
- `src/backend/mir/riscv/codegen/rv64_line_assembler.hpp`

## Non-Goals

- Do not move symbol kind mapping, relocation mapping, label binding, object
  module assembly, ELF writing, prepared data-object emission, or final text
  module assembly.
- Do not route object emission through text parsing.
- Do not change `RiscvEncodedFragment` structure or object-route caller
  contracts except through compatibility-preserving wrappers.
- Do not repair RV64 semantics or adjust tests, unsupported markers,
  allowlists, expected output, or pass/fail accounting.

## Working Model

- `object_emission.cpp` currently owns object-route fragments, labels, fixups,
  module layout, and public object/ELF entrypoints.
- `rv64_line_assembler.*` is an existing live low-level owner for line parsing
  and instruction word encoders.
- Only pure bit encoders and byte append helpers are early candidates for
  sharing. Anything that knows about object fragments, labels, fixup targets,
  or prepared facts stays in the object route.

## Execution Rules

- Start with a declaration and dependency map before moving code.
- Prefer the smallest helper API that is independent of prepared BIR, object
  module assembly, labels, and fixup attachment.
- Preserve wrapper names in `object_emission.cpp` when that keeps the diff
  smaller and callers stable.
- If branch, JAL, AUIPC, LO12, inline-asm, or call word emission is touched,
  add one affected runtime object filter to the validation proof.
- If the first pass proves no safe extraction exists, record the blocker in
  `todo.md` and stop rather than forcing a rename-only slice.

## Steps

### Step 1 - Map Encoder And Byte Helper Surfaces

Goal: identify the exact duplicate or shareable low-level helpers before code
movement.

Primary target: `object_emission.cpp` and `rv64_line_assembler.*`.

Actions:

- Use AST-backed symbol queries where practical, then targeted source reads, to
  list U/I/S/R/B/J encoders and byte append helpers in both locations.
- Classify each helper as pure word encoding, pure little-endian byte append,
  object-fragment-specific, text-parser-specific, or fixup/label-aware.
- Record unsafe helpers whose signatures or callers depend on labels, fixups,
  prepared facts, module assembly, or parser state.
- Update `todo.md` with the selected Step 2 boundary.

Completion check:

- `todo.md` names the safe helper set, destination owner, wrappers to preserve,
  unsafe helpers, and exact validation command for the first implementation
  packet.

### Step 2 - Extract Or Share Pure Helpers

Goal: move or share only the helpers proven pure in Step 1.

Primary target: `rv64_line_assembler.*` or a narrower shared helper surface if
Step 1 proves `rv64_line_assembler` is not the right owner.

Actions:

- Move declarations and definitions for pure encoders or byte append helpers
  without pulling object-route concepts into the text line assembler.
- Keep object-route fragment helpers, symbol/fixup mapping, label binding, and
  module assembly in `object_emission.cpp`.
- Preserve public object-route APIs in `object_emission.hpp`.
- Keep compatibility wrappers where that reduces churn and prevents broad
  caller rewrites.

Completion check:

- The build succeeds and the focused backend proof passes:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|rv64_roundtrip_contract|cli_riscv64_return_zero_writes_elf_obj|cli_riscv64_vrm_insn_d_source_obj)'`
- If branch or call word emission was touched, at least one affected runtime
  object filter is included in the proof.

### Step 3 - Probe Remaining Direct Cleanup

Goal: decide whether any additional include/API simplification is safe after
the helper movement.

Actions:

- Check whether object emission can call the shared helpers directly without
  adding parser, label, fixup, prepared, or module dependencies to the helper
  owner.
- Remove only redundant private wrappers that no longer protect ownership or
  readability.
- Keep wrappers when they document object-route semantics or keep the diff
  localized.

Completion check:

- Either a small direct cleanup lands with the same proof as Step 2, or
  `todo.md` records why further movement is parked.

### Step 4 - Close Readiness Review

Goal: prove the idea is complete or record the remaining boundary as a separate
follow-up.

Actions:

- Review the diff against the source idea reject signals.
- Confirm object bytes, structured fragments, labels, fixups, public APIs, and
  validation expectations were preserved.
- Ask the supervisor for broader backend proof if helper movement affected a
  wider emitted-object surface than the focused tests cover.

Completion check:

- The runbook has no remaining in-scope helper movement, `todo.md` records the
  final proof, and closure can be evaluated against the source idea.
