Status: Active
Source Idea Path: ideas/open/482_semantic_frame_slot_materialization_probe_decomposition.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Resume Implementation On The Narrowest Generic Seam

# Current Packet

## Just Finished

Completed Step 4 for idea 482 by adding the narrow scalar compare result
forced to frame-slot destination probe.

Added focused probe:

- Fixture: `tests/backend/case/riscv64_scalar_compare_frame_slot_destination.c`
- CTest: `backend_dump_riscv64_scalar_compare_frame_slot_destination`
- Route: `--dump-prepared-bir --target riscv64-linux-gnu`
- Owner: BIR/prepared dump coverage for semantic identity plus
  destination/layout facts.

Accepted facts asserted by the probe:

| Fact | Evidence |
| --- | --- |
| Scalar compare result identity | `%t2 = bir.ne i32 %t0, %t1` |
| Local destination store | `bir.store_local %lv.comparison, i32 %t2` |
| Compare result stack home | `home %t2 value_id=2 kind=stack_slot slot_id=5 offset=24` |
| Destination local object | `%lv.comparison` is an address-exposed i32 local slot. |
| Destination frame-slot layout | `slot #0 object_id=2 ... offset=0 size=4 align=4` |
| Store-site frame-slot access | `access block=entry inst_index=5 base=frame_slot stored=%t2 frame_slot=#0 offset=0 size=4 align=4` |
| Store source producer class | `store_source ... source=%t2 ... source_producer=binary` |

Fail-closed boundary asserted by the probe:

- no available semantic write-event authority;
- no available semantic materialization interval;
- no available frame-slot source fact;
- no available branch-stack-load authority.

This is a focused capability probe, not a materialization-authority
implementation. It establishes that semantic identity plus destination/layout
facts can be isolated without copying the `930930-1` monolith and without
claiming downstream authority or RV64 consumption.

Artifacts:

- `build/agent_state/482_step4_scalar_compare_destination_probe/decision.md`
- `build/agent_state/482_step4_scalar_compare_destination_probe/scalar_compare.prepared.out`
- `build/agent_state/482_step4_scalar_compare_destination_probe/scalar_compare.prepared.err`
- `build/agent_state/482_step4_scalar_compare_destination_probe/scalar_compare.prepared.status`

## Suggested Next

Plan-owner should decide whether idea 482 is complete enough to close/split
back to an implementation idea. The next coherent technical packet is the
storage-only move rejection probe from Step 3 only if supervisor wants another
focused negative baseline before resuming materialization-point producer work.

## Watchouts

- Do not treat this probe as proof of materialization authority. It only proves
  semantic identity plus destination/layout facts are isolatable.
- Do not enable source-fact, branch-stack-load authority, or RV64 branch-load
  consumption from this probe.
- Do not merge this probe with select-result stack-destination or explicit
  materialization positive coverage; those remain separate candidate families.

## Proof

Delegated proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed.
