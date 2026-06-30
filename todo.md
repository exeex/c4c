Status: Active
Source Idea Path: ideas/open/462_rv64_preterminator_predecessor_edge_parallel_copy_materialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Preterminator Parallel-Copy Materialization Contract

# Current Packet

## Just Finished

Completed Step 2 for idea 462: defined the RV64 preterminator
predecessor-edge parallel-copy materialization contract. Supporting artifact:
`build/agent_state/462_step2_preterminator_parallel_copy_contract/contract.md`.

Accepted materialization conditions:

| Field | Value |
| --- | --- |
| Traversal event | `PreparedObjectTraversalEventKind::PreTerminatorCopies` at the current event site |
| Move bundle authority | `phase=block_entry`, `authority=out_of_ssa_parallel_copy`, coordinates matching the traversal event |
| Edge identity | `classification.parallel_copy_bundle` exists and matches predecessor, successor, execution block, and `execution_site=predecessor_terminator` |
| Select publication | destination edge publication intent is `Available` for the same destination value and same parallel-copy bundle |
| Move shape | acyclic single `Move` step, no cycle temp, destination kind `value`, destination storage `register`, contiguous width 1 |
| Source | explicit supported immediate or prepared GPR-compatible source value home/storage |
| Destination | prepared GPR-compatible destination home/storage or explicit single-register destination |
| Emission | emit nothing when source and destination resolve to the same GPR; otherwise emit one RV64 GPR-to-GPR move |
| Authority source | prepared coordinate/authority facts only; no raw BIR/testcase/function/value-id inference |

Rejected adjacent shapes: raw-only or diagnostic-only bundles, missing
`out_of_ssa_parallel_copy` authority, mismatched event/phase/edge/coordinate,
missing parallel-copy bundle, unavailable or mismatched select edge publication,
ordinary block-entry/before-instruction generic moves, stack or non-register
destinations, missing homes, stack/stale-load sources, FPR/vector/multi-register
destinations, cycle-temp or multi-step bundles, and generic move-bundle support
outside this coordinate-backed predecessor-edge route.

No contract ambiguity or producer/prepared metadata blocker is visible for the
representative event.

## Suggested Next

Execute Step 3: implement or route the first preterminator parallel-copy
consumer packet.

Target files/tests for Step 3:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`
- `test_after.log`
- `build/agent_state/462_step3_preterminator_parallel_copy_consumer/**`

Exact Step 3 proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Do not add generic stack-to-register, register-to-register, or all-purpose
  move-bundle support.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
- The representative has source and destination both in `t0`; Step 3 may admit
  no-op emission only when homes actually resolve to the same GPR, and must not
  assume all register-to-register copies are no-ops.
- Distinct GPR-to-GPR support is acceptable only inside the same
  coordinate-backed predecessor-edge contract and with focused fail-closed
  coverage.
- Do not reopen ideas 456, 458, 459, 460, or 461 without new coordinate-bearing
  evidence that their exact route owns the first failure.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 2 proof:

```sh
git diff --check
```

Result: passed.
