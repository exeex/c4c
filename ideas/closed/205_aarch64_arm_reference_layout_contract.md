# AArch64 ARM-Reference Layout Contract

Status: Closed
Created: 2026-05-13
Closed: 2026-05-13

Depends On:
- `ideas/closed/203_aarch64_markdown_first_backend_reconstruction.md`
- `ideas/closed/204_aarch64_prepared_module_mir_boundary.md`

## Goal

Define the AArch64 backend file layout and ownership contract before adding
more implementation, using `ref/claudes-c-compiler/src/backend/arm` as the
reference layout vocabulary while adapting it to the current C++ BIR /
`PreparedBirModule` pipeline.

The semantic MIR source is BIR. The target-local entry may consume the
`PreparedBirModule` wrapper, but every planned AArch64 MIR family must state
which BIR facts it consumes, which prepared facts supplement those BIR facts,
and whether either carrier is missing data.

The goal is to prevent lazy implementation drift where each missing behavior is
patched into whichever file is convenient. Every backend feature family should
have an explicit owning directory/file, input facts, target records, and first
implementation route before code grows.

## Why This Idea Exists

The markdown-first reconstruction removed the failed old AArch64 `.cpp`
surface, and the prepared-module boundary introduced a small live skeleton
under `api/`, `abi/`, and `module/`. That skeleton is intentionally not
instruction selection, but it already concentrates many records in
`module/module.hpp` and `module/module.cpp`.

If the next route simply fills gaps as tests demand them, the backend will
recreate the previous failure mode: useful pieces in the wrong files, implicit
interfaces, and hard-to-review debug loops. The reference ARM backend has a
clear responsibility layout (`codegen`, `assembler`, `linker`, and per-family
codegen files). This idea uses that layout as a planning scaffold before the
C++ implementation grows.

## Reference Inputs

- `ref/claudes-c-compiler/src/backend/arm/README.md`
- `ref/claudes-c-compiler/src/backend/arm/codegen/README.md`
- `ref/claudes-c-compiler/src/backend/arm/assembler/README.md`
- `ref/claudes-c-compiler/src/backend/arm/linker/README.md`
- Current AArch64 markdown artifacts under `src/backend/mir/aarch64/*.md`
- Current live AArch64 boundary files under:
  - `src/backend/mir/aarch64/api/`
  - `src/backend/mir/aarch64/abi/`
  - `src/backend/mir/aarch64/module/`

## In Scope

- Compare current C++ AArch64 layout against the reference ARM layout.
- For every reference ARM feature family, map the intended AArch64 MIR input
  back to current BIR and `PreparedBirModule` carriers before assigning an
  owning file.
- Produce an AArch64 layout ownership ledger covering at least:
  - public prepared-module entry
  - target ABI and AAPCS64 facts
  - module/function/block MIR containers
  - operands and value identity records
  - registers and register classes
  - frame/prologue/epilogue/callee-save records
  - branch/compare records
  - call/return/variadic records
  - memory/addressing records
  - globals/string/data records
  - moves, spills, reloads, and parallel-copy records
  - scalar ALU, casts, float ops, atomics, intrinsics, inline asm, f128/i128
  - assembler, encoder, object writer, binary utils, and linker surfaces
- Decide which current `module/` records are stable module-owned data and
  which should be split later into `abi/`, `codegen/`, `assembler/`, or
  deferred contracts.
- Produce a BIR/prepared carrier checklist beside the layout ledger. For each
  feature family, classify the needed input facts as:
  - present in BIR
  - present only after shared preparation
  - missing from both and requiring a BIR/prepared gap idea
  - intentionally deferred
- Define the target C++ directory/file plan for the next implementation wave,
  including which files are allowed to exist as `.cpp` first and which remain
  markdown/deferred.
- Add or update markdown documentation under `src/backend/mir/aarch64/` with
  the ownership ledger and placement rules.
- If layout review discovers a missing BIR or `PreparedBirModule` carrier,
  record it explicitly and create a separate `ideas/open/` BIR/prepared gap
  idea instead of working around it in AArch64 target code.

## Required Outputs

1. A markdown responsibility-boundary update under `src/backend/mir/aarch64/`
   that maps the AArch64 design to the reference ARM layout while adapting it
   to BIR and `PreparedBirModule`.
2. One or more `ideas/open/` BIR/prepared gap ideas for every required
   backend-facing BIR/prepared carrier found insufficient during the layout
   review. If no BIR/prepared gap is found, the markdown output must say so
   explicitly.

## Out Of Scope

- Adding instruction selection for scalar, memory, calls, branches, or returns.
- Emitting assembly text, assembling objects, or linking binaries.
- Implementing codegen behavior only because a test currently needs it.
- Treating the ARM reference layout as sufficient without checking whether
  current BIR/prepared carriers can actually feed that feature.
- Moving large amounts of live code before the ownership contract is accepted.
- Copying the Rust reference layout mechanically when the C++ prepared-module
  pipeline needs a different boundary.
- Adding rendered-name lookup or old markdown-derived shape matching as a
  backend semantic shortcut.

## Acceptance Criteria

- A committed AArch64 layout ownership ledger exists under
  `src/backend/mir/aarch64/`.
- The ledger maps each feature family to:
  source BIR facts, supplemental prepared facts, target-local record type,
  owning directory/file, deferred status if any, and first allowed
  implementation idea.
- The ledger explicitly calls out BIR/prepared gaps discovered while comparing
  the ARM reference layout to the current C++ pipeline.
- The ledger explicitly decides the role of current `api/`, `abi/`, and
  `module/` files and identifies any records that should move before broad
  implementation proceeds.
- No new instruction selection, assembly emission, object writing, or linker
  behavior is added in this idea.
- Any discovered BIR/prepared carrier gap is split into a separate
  `ideas/open/` source idea; no AArch64 target-local workaround is accepted.
- The final output includes both required deliverables: updated markdown
  responsibility boundaries and concrete BIR/prepared gap ideas when gaps
  exist.
- Future backend implementation slices can be checked against the ledger before
  adding a new `.cpp` or expanding an existing one.

## Reviewer Reject Signals

- The route adds feature behavior before producing the layout ownership ledger.
- The ledger is only a directory listing and does not map feature families to
  owning files plus BIR/prepared input facts.
- The route copies an ARM reference feature into an AArch64 owner without
  checking whether BIR carries the semantic input and whether preparation
  carries the backend supplement.
- The route keeps expanding `module/module.cpp` or `module/module.hpp` without
  deciding which responsibilities belong elsewhere.
- A missing BIR/prepared fact is worked around through rendered strings,
  printed BIR, old markdown examples, or target-local shape matching.
- A missing BIR/prepared carrier is only mentioned in the ledger but no
  `ideas/open/` gap idea is created for it.
- The Rust ARM reference layout is copied mechanically without explaining how
  it maps to the C++ `PreparedBirModule` pipeline.
- Tests are weakened, skipped, or reclassified to make a misplaced feature
  slice appear acceptable.
