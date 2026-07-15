# Current Packet

Status: Active
Source Idea Path: ideas/open/828_shared_worktree_direct_call_authority_isolation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory and preserve the dirty slice

## Just Finished

- Lifecycle switch: 827 Step 2 is paused before implementation because its
  required nearby frontend test is already owned by preserved, unaccepted
  Idea 821/822 material. Its exact selected direct-call contract and proof
  remain in 827's resumption record.

## Suggested Next

- Create and verify one named recoverable preservation artifact for the dirty
  `src/codegen/lir/hir_to_lir/expr/binary.cpp` and
  `tests/frontend/frontend_lir_function_signature_type_ref_test.cpp` slice,
  then remove only that slice from the shared route.

## Watchouts

- Do not overwrite, co-commit, accept, repair, or semantically alter Ideas
  821/822. Do not implement 827 authority during isolation. Preserve the
  conflict-free stash/pop baseline in `test_before.log` and do not modify
  canonical regression logs outside supervisor direction.

## Proof

- The accepted pre-change baseline is fresh `cmake --build --preset default`
  followed by `ctest --test-dir build --output-on-failure -R
  '^frontend_lir_call_type_ref$'` captured in `test_before.log` after a
  conflict-free stash/pop. It is baseline evidence only, not acceptance for
  this isolation or any semantic authority route.
