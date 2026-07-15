# Current Packet

Status: Active
Source Idea Path: ideas/open/828_shared_worktree_direct_call_authority_isolation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory and preserve the dirty slice

## Just Finished

- Step 1 complete: preserved exactly the dirty 821/822 frontend slice in
  `review/828_preserved_821_822_frontend_slice.patch`, verified both reverse
  removal and restoration applicability, and removed only that slice from the
  shared worktree. Restore it explicitly with
  `git apply review/828_preserved_821_822_frontend_slice.patch`.

## Suggested Next

- Continue 828 only after the supervisor selects the next isolation packet;
  semantic 821/822 or 827 authority routes are not accepted by this
  preservation step.

## Watchouts

- The preservation artifact contains the exact former diff for
  `src/codegen/lir/hir_to_lir/expr/binary.cpp` and
  `tests/frontend/frontend_lir_function_signature_type_ref_test.cpp`; it is
  reversible only and does not accept, repair, or semantically alter the
  821/822 or 827 routes. Leave unrelated dirty artifacts and `test_before.log`
  untouched.

## Proof

- Artifact checks passed: `git apply --reverse --check
  review/828_preserved_821_822_frontend_slice.patch` before removal, then
  `git apply --check review/828_preserved_821_822_frontend_slice.patch` after
  removal (the restoration command above). Required build and focused test are
  recorded in `test_after.log`.
