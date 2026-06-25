# c4c-objdump RV64 Custom Roundtrip Runbook

Status: Active
Source Idea: ideas/open/351_c4c_objdump_rv64_custom_roundtrip.md
Activated After: 32b039cf6 Close c4c-as file parser idea

## Purpose

Build the first real `c4c-objdump` path for c4c-produced RV64 relocatable
objects and prove that the current EV64 `.insn.d` object route roundtrips
through canonical assembly.

## Goal

Make this flow stable for the current RV64 VRM custom-instruction object case:

```text
source.c -> c4cll --codegen obj -> a.o
a.o -> c4c-objdump -> a.s
a.s -> c4c-as -> b.o
b.o -> c4c-objdump -> b.s
assert a.s == b.s
```

## Core Rule

Decode and print real object content. Do not recognize one known byte string,
skip unknown bytes, or use external objdump/assembler output as the semantic
source of truth.

## Read First

- `ideas/open/351_c4c_objdump_rv64_custom_roundtrip.md`
- `src/apps/c4c-objdump.cpp`
- `src/backend/mir/riscv/codegen/rv64_line_assembler.hpp`
- `src/backend/mir/riscv/codegen/rv64_line_assembler.cpp`
- `src/backend/mir/riscv/codegen/object_emission.hpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/assembler/elf_writer.cpp`
- `src/backend/mir/riscv/linker/input.cpp`
- `tests/backend/CMakeLists.txt`
- `tests/backend/cmake/run_c4c_as_parse_suite.cmake`
- `tests/backend/case/riscv64_vrm_insn_d_source.c`

## Current Targets

- `src/apps/c4c-objdump.cpp`
- New or existing reusable RV64 ELF / disassembly helpers, if the CLI becomes
  too large for direct implementation.
- Backend CMake test coverage for `c4c-objdump`, `c4c-as`, and the existing
  `riscv64_vrm_insn_d_source.c` object route.

## In Scope

- ELF64 relocatable objects produced by c4c for RV64.
- `.text` section extraction.
- Global function symbol discovery for simple function labels such as `main`.
- EV64 `.insn.d` 64-bit custom instruction decoding.
- The simple currently emitted `li a0, 0` shape.
- `ret`.
- Canonical assembly printing compatible with `c4c-as`.
- End-to-end object-to-assembly-to-object-to-assembly roundtrip proof.

## Non-Goals

- Full objdump clone behavior.
- Disassembly of arbitrary system objects.
- DWARF, source mapping, C/C++ reconstruction, or comments.
- Relocation expression pretty-printing beyond fail-closed diagnostics.
- External `llvm-objdump` compatibility for EV64 custom instructions.
- Activating or implementing `ideas/open/352_full_rv64_assembly_object_disassembly_roundtrip.md`.

## Working Model

- Treat c4c-produced relocatable objects as the supported input format.
- Parse enough ELF metadata to find `.text` bytes and matching function
  symbols.
- Decode instructions by field layout:
  - EV64 `.insn.d` must reconstruct major, operation, vector operands, and
    dtype fields from the 64-bit custom instruction encoding.
  - `li a0, 0` may support the simple emitted encoding first, but should be
    shaped as an instruction decoder rather than a full-text shortcut.
  - `ret` should decode from the emitted return instruction bytes.
- Print one canonical assembly form:

```asm
.text
.globl main
main:
  .insn.d 10, 11, v6, v0, v2, v4, 3
  li a0, 0
  ret
```

## Execution Rules

- Keep each implementation packet small enough to prove with build plus a
  focused backend CTest subset.
- Prefer reusable decoder/ELF helpers only when they reduce `c4c-objdump.cpp`
  complexity or match existing local module boundaries.
- Fail closed on unsupported ELF shape, missing `.text`, unsupported
  relocations in `.text`, or unsupported instruction bytes.
- Do not weaken existing `c4c-as` or object-route tests to make objdump pass.
- Keep `todo.md` as the only routine progress scratchpad.

## Step 1: CLI Contract And ELF Text Extraction

Goal: turn `c4c-objdump` from a placeholder into a real file-to-file tool for
the supported RV64 relocatable object envelope.

Primary target: `src/apps/c4c-objdump.cpp`

Actions:

- Parse `<input.o>` and `-o <output.s>` consistently with existing c4c CLI
  style.
- Read input bytes from disk and report clear diagnostics for missing,
  malformed, or unsupported input.
- Reuse the repo's ELF parser if practical; otherwise add the smallest local
  ELF64 reader needed to validate RV64 relocatable objects and extract `.text`.
- Locate a simple global function symbol such as `main` that starts in `.text`.
- Write a fail-closed focused test that proves unsupported or malformed input
  does not silently succeed.

Completion check:

- `cmake --build --preset default` succeeds.
- A focused CTest subset proves `c4c-objdump` rejects unsupported input and can
  find `.text` for the canonical c4c RV64 object without printing fake
  disassembly.

## Step 2: RV64 Instruction Decoder

Goal: decode the initial supported instruction subset from bytes into a
structured representation.

Primary target: reusable helper or `src/apps/c4c-objdump.cpp`, depending on
the smallest clean boundary discovered in Step 1.

Actions:

- Decode EV64 `.insn.d` from fields, not from one full known byte string.
- Decode the currently emitted `li a0, 0` shape.
- Decode `ret`.
- Reject unknown instruction bytes with an explicit diagnostic or unsupported
  marker chosen by the implementation contract.
- Add focused tests that include at least the canonical object bytes and one
  unsupported-byte case.

Completion check:

- `cmake --build --preset default` succeeds.
- Focused backend tests prove canonical bytes decode to the intended
  instruction sequence and unsupported bytes fail closed.

## Step 3: Canonical Assembly Printer

Goal: print decoder output as assembly accepted by `c4c-as`.

Primary target: `c4c-objdump` output path and backend CMake test coverage.

Actions:

- Emit `.text`, `.globl <symbol>`, `<symbol>:`, and indented instructions in
  the canonical form from the source idea.
- Keep output deterministic and whitespace-stable for direct text comparison.
- Assemble the printed output with `c4c-as` in the focused test.
- Extract `.text` from the assembled object and verify it matches the original
  canonical text bytes.

Completion check:

- `cmake --build --preset default` succeeds.
- Focused backend tests prove `a.o -> a.s -> b.o` preserves
  `0a0320080b0300001305000067800000`.

## Step 4: Repeated Roundtrip Stabilization

Goal: prove that the custom instruction stream is stable across repeated
disassembly and assembly.

Primary target: a new or extended backend CMake roundtrip suite.

Actions:

- Produce `a.o` from `tests/backend/case/riscv64_vrm_insn_d_source.c` through
  `c4cll --codegen obj --target riscv64-linux-gnu`.
- Run `c4c-objdump a.o -o a.s`.
- Run `c4c-as a.s -o b.o`.
- Run `c4c-objdump b.o -o b.s`.
- Compare `a.s` and `b.s` exactly.
- Keep the test independent of external objdump/as tools for semantic truth.

Completion check:

- `cmake --build --preset default` succeeds.
- Focused backend tests prove `a.s == b.s` and the printed assembly contains
  `.insn.d`, `li a0, 0`, and `ret`.

## Step 5: Acceptance And Broader Validation

Goal: finish the idea with enough proof for supervisor acceptance.

Actions:

- Ensure `c4c-objdump input.o -o output.s` succeeds for the current RV64 VRM
  `.insn.d` object case.
- Ensure EV64 custom instruction bytes decode to `.insn.d` instead of
  `<unknown>`.
- Ensure unsupported bytes are not silently skipped or misdecoded.
- Run the supervisor-selected focused subset at minimum:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(c4c_objdump_|c4c_as_|cli_riscv64_vrm_insn_d_source_obj$)'
```

- Ask the supervisor whether this milestone needs a broader regression guard
  or full CTest pass before closure.

Completion check:

- All source acceptance criteria are covered by committed tests and fresh
  proof logs.
- `todo.md` records the final executor proof and any remaining follow-up
  notes.
