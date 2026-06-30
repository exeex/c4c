Status: Active
Source Idea Path: ideas/open/474_prepared_frame_slot_materialization_no_clobber_facts.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route Prepared Source-Fact Producer

# Current Packet

## Just Finished

Completed Step 3 implementation for idea 474. Added the smallest independent
prepared source-fact carrier/status surface for frame-slot
materialization/no-clobber facts without changing downstream
`PreparedBranchStackLoadAuthority` availability.

Implemented surface:

| Surface | Result |
| --- | --- |
| `PreparedFrameSlotSourceFactStatus` | Independent status vocabulary for available, missing source/materialization, mismatches, missing/path failures, same-slot writes, effect unknowns/clobbers, and unsupported boundaries. |
| `PreparedFrameSlotSourceFactInputs` / `PreparedFrameSlotSourceFact` | Planner input and output carrier for target value/home/slot/object, source value/producer, materialization event, and explicit no-clobber classifiers. |
| `plan_prepared_frame_slot_source_fact` | Returns `available` only when explicit materialization/write, path coverage, same-slot exclusion, effect safety, and publication/move/parallel-copy non-clobber inputs are all present. |
| `collect_prepared_frame_slot_source_facts` | Emits independent unavailable records for prepared stack-home branch values; scalar condition rows report `missing_materialization_event`, while select-result stack-destination, pointer/provenance, and unsupported-terminator rows remain `unsupported_boundary`. |

Focused coverage:

| Case | Coverage |
| --- | --- |
| Explicit synthetic materialization/no-clobber inputs | `available` with target/source ids, slot/object, offsets, and materialization kind preserved. |
| No materialization event | `missing_materialization_event`. |
| Wrong materialization slot | `materialization_slot_mismatch`. |
| Wrong materialization value | `materialization_value_mismatch`. |
| Missing path validity | `missing_path_validity`. |
| Same-slot write present | `same_slot_write_found`. |
| Unknown call/helper effect | `call_or_helper_effect_unknown`. |
| Explicit unsupported boundary | `unsupported_boundary`. |
| Prepared collector scalar stack-home row | Emits an unavailable `missing_materialization_event` source-fact record. |
| Prepared collector select-result stack-destination row | Emits `unsupported_boundary`; does not blur into generic missing materialization. |
| Prepared collector pointer/provenance row | Emits `unsupported_boundary`; pointer status/provenance remains a separate owner. |
| Prepared collector unsupported-terminator row | Emits `unsupported_boundary`; terminator lowering remains out of scope. |

Artifacts:

- `build/agent_state/474_step3_source_fact_carrier/summary.md`

## Suggested Next

Execute Step 4 from `plan.md`: Residual Disposition And Close Readiness.
Re-probe/classify whether idea 474 is close-ready as an independent
carrier/status surface, or whether a precise population/printer/follow-up
packet remains before downstream branch-site source-fact consumption can
resume.

## Watchouts

- Step 3 did not mark `PreparedBranchStackLoadAuthority` available and did not
  edit RV64 lowering.
- The new collector records missing materialization only for ordinary scalar
  stack-home branch rows; boundary rows stay classified as unsupported.
  Downstream certificates still need a later consumer/population packet before
  branch-stack-load authority can become available.
- Do not implement RV64 branch-load emission in this producer plan.
- Do not infer source facts from stack homes/storage, offsets, object ids, raw
  BIR, value names, block labels, function names, testcase names, or dump
  order.
- Keep pointer-value/provenance, select-result stack-destination, and
  unsupported-terminator boundaries separate.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Delegated proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed; `test_after.log` reports `100% tests passed, 0 tests failed out of 327`.
Log: `test_after.log`.
