# AArch64 Codegen Reference Layout Consolidation

## Intent

Reorganize `src/backend/mir/aarch64/codegen` `.cpp` and `.hpp` boundaries to
match the responsibility model documented in
`ref/claudes-c-compiler/src/backend/arm/codegen/README.md`.

This idea is about source layout, naming, and module boundaries after the
forward-migration work. It is not another prealloc/BIR migration audit.

## Reference Model

Use the reference README file inventory as the target mental model for
architecture codegen:

- core codegen/module entry
- prologue/epilogue and parameter store emission
- calls and return ABI emission
- memory addressing and load/store emission
- ALU, comparison, casts, float, i128, f128, atomics, intrinsics
- globals, variadic support, inline asm, asm emitter, peephole

The C++ implementation may keep multiple `.cpp` shards for reviewability, but
the visible families should read like these modules rather than like historical
migration scaffolding.

## Background

Earlier work reduced header noise and moved several Prepared/MIR responsibilities
toward `prealloc` or shared helpers. Remaining files under
`src/backend/mir/aarch64/codegen` now need a layout pass so future x86/RISC-V
work can understand which files are real target codegen modules and which are
thin adapters.

Current examples that likely need review:

- `dispatch.hpp` collects many unrelated declarations.
- `dispatch_*.cpp` names still reflect old migration mechanics.
- some `calls_*.cpp` shards may now be thin enough to merge or rename.
- adapter files may belong under clearer module families such as `calls`,
  `memory`, `comparison`, or `codegen` instead of `dispatch`.

## Work

First, map every current `src/backend/mir/aarch64/codegen/*.cpp` and `.hpp` file
to one reference-model family:

- `codegen/module`
- `prologue`
- `calls`
- `memory`
- `alu`
- `comparison`
- `float_ops`
- `cast_ops`
- `f128`
- `i128_ops`
- `atomics`
- `intrinsics`
- `globals`
- `returns`
- `variadic`
- `inline_asm`
- `asm_emitter`
- `peephole`
- `machine_printer`
- `operands/instruction`
- `adapter/internal`

Then perform focused consolidation slices:

- reduce oversized catch-all headers such as `dispatch.hpp`
- merge or rename thin `.cpp` files whose current names describe old scaffolding
  rather than durable module responsibility
- keep large, cohesive target-emission files separate even when the reference
  has one Rust module
- update build files and includes mechanically with minimal churn

## Preferred Shape

Use one family header per durable family where practical. Multiple `.cpp` files
may share that header when they are implementation shards of the same family.

Examples:

- `calls*.cpp` should present as the calls family, not as many independent
  public-ish interfaces.
- `dispatch_*` files should either become internal adapter shards with a clear
  private header boundary, or be renamed/merged into their reference-model
  family.
- helper declarations that are private to one `.cpp` should leave broad headers.

## Constraints

- Do not move target-neutral responsibilities to `prealloc` or BIR in this idea;
  create a separate follow-up idea if a new migration candidate is discovered.
- Do not weaken tests or change emitted behavior.
- Do not merge large cohesive files just to reduce file count.
- Do not create a single giant `dispatch.cpp`.
- Keep diffs reviewable; prefer multiple small mechanical slices.
- Preserve the useful C++ implementation shards where they make review easier.

## Required Output

The runbook generated from this idea should produce:

1. A file-to-reference-family map in `todo.md`.
2. A prioritized consolidation list, starting with low-risk header and thin-file
   cleanup.
3. One or more implementation slices that make the layout visibly closer to the
   reference README inventory.
4. A final count of `.cpp` / `.hpp` files under `src/backend/mir/aarch64/codegen`
   and a short note on remaining intentionally separate files.

## Acceptance

- The AArch64 codegen directory reads closer to the reference module inventory.
- Catch-all or historical scaffold headers are reduced or justified.
- Thin historical adapter files are merged, renamed, or explicitly justified as
  durable.
- Fresh backend build/tests pass for each code-changing slice.
- Any newly discovered target-neutral migration candidate is recorded as a new
  `ideas/open/*.md` follow-up rather than hidden inside this layout cleanup.

## Closure Notes

Closed after Step 5 final layout review.

- Final direct inventory under `src/backend/mir/aarch64/codegen/`: 77 files,
  split into 41 `.cpp` and 36 `.hpp`.
- The visible AArch64 codegen layout now maps to the reference family
  inventory, with remaining adapter/internal files explicitly justified.
- `dispatch.hpp` was reduced to the block-dispatch surface:
  `InstructionDispatchResult`, `make_block_lowering_context`, and
  `dispatch_prepared_block`.
- No target-neutral migration candidate was found during the final review, so
  no follow-up idea was required for this lifecycle.
- Close-time regression guard used backend scope:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"`.
  The guard passed with 162/162 tests passing before and after.
