Status: Active
Source Idea Path: ideas/open/653_stack_carried_pointer_source_publication_materialization.md
Source Plan Path: plan.md
Current Step ID: 4A
Current Step Title: Publish The `%t23` Compare Pointer Source Chain

# Current Packet

## Just Finished

Step 4A, `Publish The %t23 Compare Pointer Source Chain`, repaired the
prepared/RV64 branch stack-load authority for the new `%t23` producer shape.

- Prepared branch stack-load clobber safety now recognizes an unambiguous
  same-block pointer-add producer after an intervening call as re-establishing
  the selected stack slot, including the before-instruction stack move emitted
  at the producer instruction.
- The rule is keyed on the named BIR pointer producer and exact prepared value
  home/slot identity; unrelated pointer producers and still-live lhs values
  remain fail-closed.
- For `src/loop-2e.c`, prepared output now records the RHS
  `branch_stack_load_authority` for `%t23` value id `26` / `slot #50+stack368`
  as `status=available`.
- RV64 object emission for `src/loop-2e.c` advances past
  `unsupported_branch_stack_load_authority` /
  `authority_status=missing_stack_clobber_safety` and exits 0.
- Evidence is in
  `build/agent_state/653_step4a_loop_t23_clobber_safety/summary.md`.

## Suggested Next

Delegate the next packet to decide whether Step 4A is complete enough for
integration/runtime probing, or to inspect the current `loop-2e.c` runtime
result under qemu now that object emission reaches a concrete RV64 object.

## Watchouts

- Do not chase the old `%t23` value id `27` / `slot #46+stack336` preservation
  shape as if it were still current; the semantic producer changes value
  numbering and moves `%t23` to value id `26` / `slot #50+stack368`.
- Do not treat the remaining failure as a missing semantic producer:
  `%t23 = bir.add ptr %t21, 156` is present in semantic and prepared BIR.
- Do not re-open the `%t23` clobber-safety owner without checking the current
  prepared authority row first; it is now `status=available` for the refreshed
  `%t23` value id `26` / `slot #50+stack368` shape.
- Do not broaden Step 4A into the parked `%t6` runtime owner; that owner is
  downstream of the pointer-source publication/materialization boundary.
- Do not weaken expectation files, unsupported markers, allowlists, or runtime
  accounting to claim progress.

## Proof

`test_after.log`: `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_'`.

Result: build completed and the delegated backend subset remains red with 32
failed tests out of 365, matching `test_before.log` by failed test name. No new
backend failure names were introduced. `test_after.log` is the preserved proof
log.

Focused proof:

- `cmake --build --preset default --target backend_prepare_stack_layout_test
  c4cll && ./build/tests/backend/bir/backend_prepare_stack_layout_test`
  passed.
- `build/c4cll --target riscv64-linux-gnu --dump-prepared-bir
  tests/c/external/gcc_torture/src/loop-2e.c` wrote
  `build/agent_state/653_step4a_loop_t23_clobber_safety/loop-2e.after.prepared.txt`
  and shows `%t23` branch stack-load authority `status=available`.
- `build/c4cll --target riscv64-linux-gnu --codegen obj
  tests/c/external/gcc_torture/src/loop-2e.c -o
  build/agent_state/653_step4a_loop_t23_clobber_safety/loop-2e.o` exits 0.
