Status: Active
Source Idea Path: ideas/open/653_stack_carried_pointer_source_publication_materialization.md
Source Plan Path: plan.md
Current Step ID: 3B
Current Step Title: Connect Prepared And RV64 Stack-Carried Pointer Authority

# Current Packet

## Just Finished

Step 3B, `Connect Prepared And RV64 Stack-Carried Pointer Authority`: connected
the Step 3A local-frame pointer producer through prepared branch stack-load
authority and RV64 fused pointer branch consumption.

- Prepared branch stack-load clobber safety now distinguishes unrelated
  same-slot clobbers from a branch pointer value's own explicit frame-slot
  address materialization.
- RV64 fused pointer branch lowering can materialize a stack-homed pointer
  operand directly from a selected prepared frame-slot address materialization
  when no prior stack-preservation carrier applies.
- Focused RV64 object-emission coverage was added for a materialized RHS pointer
  branch source and fail-closed missing/mismatched materialization payloads.
- Fresh prepared output for `20140828-1.c` now reports `%t6` RHS branch
  authority as `status=available`, and the focused RV64 object route for that
  file exits 0.
- Evidence is in
  `build/agent_state/653_step3b_prepared_rv64_pointer_authority/summary.md`.

## Suggested Next

Supervisor review/commit for the Step 3B slice. If continuing execution after
commit, use the next failing owner from the backend subset rather than reopening
`missing_stack_clobber_safety` for `20140828-1.c`.

## Watchouts

- The real `%t6` address-materialization row currently carries block, inst,
  result value name, frame-slot id, and byte offset; it does not require
  `result_home_kind` or `result_value_id` to be present.
- Same-slot stack moves remain clobbers unless the destination value has an
  explicit matching pointer address-materialization at that instruction.
- `backend_riscv_object_emission` still has known existing failures in the full
  executable, so use the focused `20140828-1.c` object probe and backend subset
  failure-list comparison for this packet's acceptance signal.
- Do not weaken expectation files, unsupported markers, allowlists, or runtime
  accounting to claim progress.

## Proof

`test_after.log`: `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_'`.

Result: build completed and the delegated backend subset remains red with 32
failed tests out of 365, matching `test_before.log`; no new backend failure
names were introduced. `backend_prepare_stack_layout` passed after narrowing the
same-slot move rule. `test_after.log` is the preserved proof log.

Additional focused probes:

- `build/c4cll --target riscv64-linux-gnu --dump-prepared-bir
  tests/c/external/gcc_torture/src/20140828-1.c` wrote
  `build/agent_state/653_step3b_prepared_rv64_pointer_authority/after_prepared.txt`
  and shows `%t6` RHS branch authority as `status=available`.
- `build/c4cll --target riscv64-linux-gnu --codegen obj
  tests/c/external/gcc_torture/src/20140828-1.c -o
  build/agent_state/653_step3b_prepared_rv64_pointer_authority/final.o` exited
  0; log:
  `build/agent_state/653_step3b_prepared_rv64_pointer_authority/final_object_probe.log`.
- `ctest --test-dir build --output-on-failure -R '^backend_prepare_stack_layout$'`
  passed; log:
  `build/agent_state/653_step3b_prepared_rv64_pointer_authority/backend_prepare_stack_layout.log`.
