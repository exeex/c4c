# Current Packet

Status: Complete
Source Idea Path: ideas/open/205_aarch64_arm_reference_layout_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Current AArch64 And ARM Reference Surfaces

## Just Finished

Completed Step 1 audit of the ARM reference layout and current AArch64
surfaces.

Ledger target for Step 2:

- Primary: `src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md`
- Companion/index cross-link: `src/backend/mir/aarch64/CLASSIFICATION_INDEX.md`
- Context contract to preserve: `src/backend/mir/aarch64/BACKEND_ENTRY_CONTRACT.md`

Reference ARM families to cover in the ledger:

- Top-level backend pipeline: codegen, builtin assembler, builtin linker, and
  AAPCS64 target profile assumptions from
  `ref/claudes-c-compiler/src/backend/arm/README.md`.
- Codegen families: public emission entry, register/operand model, frame and
  prologue/epilogue, calls, returns, variadic handling, memory/addressing,
  globals/data, ALU, comparisons/branches, casts, float ops, f128, i128,
  atomics, intrinsics, inline asm, asm emitter constraints, and peephole.
- Assembler families: parser/preprocessor/directives, operand model,
  ELF object writer, encoder entry, data-processing, compare/branch,
  load/store, bitfield, FP scalar, system, and NEON encoder families.
- Linker families: input loading, ELF/object parsing, symbol/type model,
  relocation handling, PLT/GOT, static executable emission, dynamic emission,
  shared-library emission, and link orchestration.

Current AArch64 surfaces audited:

- Existing markdown contracts/indexes:
  `BACKEND_ENTRY_CONTRACT.md`, `BIR_PREPARED_GAP_LEDGER.md`,
  `CLASSIFICATION_INDEX.md`, `BINARY_UTILS_CONTRACT.md`, `mod.md`, plus
  markdown mirrors under `codegen/`, `assembler/`, `assembler/encoder/`, and
  `linker/`.
- Live public entry surface: `api/api.hpp` and `api/api.cpp`, which currently
  expose `build_prepared_module(const PreparedBirModule&)`.
- Live ABI handoff surface: `abi/abi.hpp` and `abi/abi.cpp`, which validate
  AArch64 target arch and AAPCS64 backend ABI.
- Live module surface: `module/module.hpp` and `module/module.cpp`, which
  already collect prepared-module records for registers, operands, frame
  slots, callee saves, dynamic stack, branches, calls, moves, ABI bindings,
  spills/reloads, parallel copies, globals, strings, relocation needs,
  functions, blocks, and module build status.
- Staged non-ledger surfaces to classify, not expand: legacy raw-BIR/LIR
  text emission in `codegen/emit.hpp`, assembler text/object stubs in
  `assembler/`, and first-static linker stubs in `linker/`.

Required feature families for Step 2 coverage:

- Public prepared-module entry and rejected raw-BIR/LIR/text fallback roles.
- Target ABI/AAPCS64, target profile validation, call/return/variadic facts,
  register banks/classes, and callee-save/frame conventions.
- Module/function/block containers, semantic ids, operands/value identity,
  registers, frame/prologue/epilogue, dynamic stack, branches/compares,
  calls, memory/addressing, globals/string/data, moves, spills, reloads,
  parallel copies, scalar operations, floating/special-width operations,
  atomics, intrinsics, inline asm, assembler, encoder, object writer,
  binary utilities, and linker surfaces.
- BIR/prepared carrier status for every family: present BIR fact, present
  prepared fact, target-local record type, missing carrier, deferred status,
  owning file, and first allowed implementation idea.

## Suggested Next

Proceed to Step 2 by drafting the layout ownership ledger in
`src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md`, with cross-link updates
only where needed in `CLASSIFICATION_INDEX.md`.

## Watchouts

- Do not edit `ideas/open/205_aarch64_arm_reference_layout_contract.md` unless
  source intent itself proves wrong.
- `BIR_PREPARED_GAP_LEDGER.md` already contains the closest existing ledger
  shape; Step 2 should extend it into the full responsibility ledger rather
  than starting from a plain directory listing.
- `CLASSIFICATION_INDEX.md` is an index/cross-surface triage aid, not the
  primary responsibility ledger.
- `module/module.hpp` already owns many prepared record snapshots; Step 2 must
  decide which records stay module-owned and which later belong to `abi/`,
  `codegen/`, `assembler/`, object/binary-utils, linker, or deferred contracts.
- Do not add instruction selection, assembly emission, object writing, linker
  behavior, or target-local workarounds for missing BIR/prepared facts.
- Do not expand `module/module.cpp` or `module/module.hpp` as a convenient
  destination without first deciding ownership in the ledger.
- Do not recover backend facts from rendered names, printed BIR, old markdown
  examples, assembly-string parsing, parser operands, or legacy shape matching.
- Split required missing BIR/prepared carriers into separate `ideas/open/`
  source ideas with concrete reviewer reject signals.
- Preserve unrelated dirty files and transient `review/` artifacts.

## Proof

Docs-only packet; delegated proof command:

`git diff --check -- todo.md`

Result: pass, exit 0.
