# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.25
Current Step Title: Publish i64 inline-asm output binding authority

## Just Finished

- Completed Plan Step 7.25 for PS's single non-explicit-register output-only
  scalar i64 `LirInlineAsmOp` binding without production or schema changes.
- Focused production coverage proves that the accepted Step-7.21 generic path
  resolves i64 natively, allocates the semantic output through `fresh_value`,
  stores the exact ID in the sole i64/Output/index-zero binding, and preserves
  that ID through representation-preserving coercion into the later i64 Store.
- Reachable malformed coverage rejects missing, invalid, duplicate, and wrong-
  alternative definitions; wrong role, index, count/position, binding/operation
  width; unknown/cross-function Store uses; and Store-width conflicts.
- Misleading compatibility-result, binding, Store, rendered, and opaque text
  passes with native facts unchanged. The accepted i32 Step-7.21 contract stays
  unchanged and proves the same mechanism at its neighboring width.

## Suggested Next

- Supervisor: select the next carrier-ready Step-7 row from the updated matrix;
  do not reopen the separately blocked parity, multi-output, inline-asm input,
  or implicit-coercion families without their required native carriers.

## Watchouts

- Step 7.25 is evidence that the single scalar integer Output binding mechanism
  is width-generic; it does not authorize input-count, multi-result mapping,
  tied/read-write, explicit-register, floating/vector/aggregate, or text-derived
  semantics.
- Keep compatibility `result`, rendered operands, assembly/constraint text,
  clobbers, and other mirrors authority-free. No width-purpose field was needed
  or added.
- Producer-specific implicit coercion still needs a coercion/source-purpose
  carrier; inline-asm inputs still need native expected-shape/count authority;
  scalar multi-output still needs result mapping/extraction authority.

## Proof

- Fresh `cmake --build --preset default`: passed.
- Matched focused before/after
  `ctest --test-dir build -R '^frontend_hir_tests$' --output-on-failure`:
  passed 1/1 before and 1/1 after.
- Transient focused logs: `/tmp/c4c_step_7_25_before.log` and
  `/tmp/c4c_step_7_25_after.log`; no canonical full logs were written.
- Supervisor full matched regression guard: 3033/3033 passed before and
  3033/3033 passed after, delta 0/0 with no new failures.
- `git diff --check`: passed.
