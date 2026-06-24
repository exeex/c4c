# RV64 Prepared Edge Publication Module Flow

## Goal

Repair the RV64 prepared module/function emission flow so prepared block-entry
edge-publication moves are emitted in the final assembly when the shared
prepared facts say they are required.

This is a target prepared-flow orchestration fix. The RISC-V `mv` instruction
and the edge-publication helper already exist; the missing behavior is that
module/function emission does not reliably splice those helper results into
the emitted text.

## Why This Exists

The last known full-suite baseline failure is:

```text
backend_riscv_prepared_edge_publication
RISC-V prepared module should emit a register edge move
```

The failing focused test builds a prepared module with a join-edge publication
from `%src` in `a0` to `%dst` in `a1`. It expects final RV64 assembly to contain:

```asm
mv a1, a0
```

The helper path itself is not the problem:

- `consume_edge_publication_move_intent(...)` can consume the shared prepared
  `edge_publications` lookup.
- It renders the target-local instruction text `mv a1, a0`.
- It still fails closed when the shared lookup authority is removed.

The suspected orchestration gap is in `emit_prepared_module_text(...)`:

```cpp
if (append_simple_prepared_bir_function_asm(out, module, &lookups, *function_it)) {
  continue;
}

for (const auto& publication : lookups.edge_publications.publications) {
  ...
  append_edge_publication_move_instruction(...);
}
```

If the simple function emitter returns success, the module loop skips the
edge-publication append path entirely. The minimal fixture has almost no BIR
body; the required move exists in prepared control-flow/move-bundle facts, so
the final assembly misses it even though the helper can render it.

This is not an AArch64-versus-RV64 ISA difference. RV64 supports `mv` and the
codebase emits it in many other paths. The issue is where prepared edge moves
are scheduled into RV64 target output.

## In Scope

- Reproduce `backend_riscv_prepared_edge_publication`.
- Inspect the RV64 prepared module/function emission flow around
  `append_simple_prepared_bir_function_asm(...)`,
  `append_edge_publication_move_instruction(...)`, and prepared
  block-entry/edge-publication facts.
- Ensure available prepared edge-publication moves are emitted in the final
  RV64 assembly at a semantically valid point.
- Preserve the helper's shared-lookup authority: do not rediscover edge moves
  by scanning predecessor/successor BIR blocks.
- Keep unsupported homes and mismatched publication cases fail-closed.
- Preserve existing RV64 prepared runtime behavior and the broader backend
  route.

## Out Of Scope

- Changing RV64 instruction spelling or treating `mv` as unsupported.
- Reworking AArch64 edge-copy/publication lowering.
- Migrating prepared edge publication authority into BIR or target-local fact
  tables.
- Fixing the separate `c_testsuite_src_00124_c` frontend LLVM-route function
  pointer return type regression; that belongs to idea 327.
- Broad RV64 control-flow rewrites beyond the minimal prepared edge-publication
  scheduling gap.
- Weakening the focused test or marking the supported register edge move
  unsupported.

## Suggested Execution Order

1. Capture the current emitted text for the focused fixture if needed, and
   confirm it lacks `mv a1, a0` while the helper intent is available.
2. Decide the correct insertion point:
   - inside `append_simple_prepared_bir_function_asm(...)` when entering a
     block or join block; or
   - in module/function orchestration after simple function emission without
     skipping available edge-publication moves.
3. Implement the smallest flow repair that emits prepared edge-publication
   instructions through `append_edge_publication_move_instruction(...)`.
4. Keep fail-closed behavior for unsupported publication statuses and homes.
5. Prove:
   - `ctest --test-dir build -R '^backend_riscv_prepared_edge_publication$' --output-on-failure`
     passes;
   - a narrow RV64/backend subset remains monotonic;
   - after idea 327 also closes, full-suite failures are expected to drop to
     zero.

## Acceptance Criteria

- `backend_riscv_prepared_edge_publication` passes.
- Final emitted RV64 assembly for the focused register-edge fixture contains
  `mv a1, a0`.
- The helper still proves it consumes shared prepared lookup authority and does
  not rediscover edge moves when the lookup is removed.
- Unsupported source/destination homes remain explicit fail-closed cases.
- The repair is placed in RV64 prepared module/function flow, not in
  AArch64-specific code or broad shared prepared authority.
- It does not mask or combine the separate idea 327 frontend LLVM-route
  regression.

## Reviewer Reject Signals

- The patch special-cases the fixture function `join_regs`, value ids `1`/`2`,
  registers `a0`/`a1`, or the literal string `mv a1, a0`.
- The patch emits edge moves by scanning BIR blocks instead of consuming shared
  prepared `edge_publications`.
- The helper still returns `Available`, but final module text can omit the move
  on the supported register-edge path.
- Unsupported edge-publication cases become silently ignored or accepted as
  successful supported paths.
- The route turns into a broad RV64 control-flow rewrite, AArch64 port, or BIR
  authority migration without focused evidence.
- The test is weakened to check only helper intent while no final assembly move
  is emitted.

## Notes For The Agent

- Start from `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`
  and `src/backend/mir/riscv/codegen/prepared_module_emit.cpp`.
- The crucial distinction is helper availability versus final assembly
  scheduling. Do not spend time proving RV64 has `mv`; it already does.
- Coordinate with idea 327 if both are open: this idea owns only the old
  prepared edge publication failure.

## Completion Note

Closed after the RV64 prepared module emitter was repaired to append available
shared edge-publication moves after successful simple prepared function
emission. The focused final-assembly regression now emits the required
register-edge move, helper authority remains shared-lookup based, and the
matching regression guard resolved only
`backend_riscv_prepared_edge_publication` with no new failures.
