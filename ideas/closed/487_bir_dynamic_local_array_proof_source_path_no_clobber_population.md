# BIR Dynamic Local-Array Proof-Source Path/No-Clobber Population

Status: Closed
Type: BIR semantic proof population producer idea
Parent: `ideas/closed/486_bir_index_range_proof_path_dominance_carrier.md`
Source Evidence:
- `build/agent_state/486_step3_range_proof_path_dominance_carrier/summary.md`
- `build/agent_state/486_step4_residual_disposition/disposition.md`
Owning Layer: BIR semantic dynamic index proof-source, path, and no-clobber population
Closed By: lifecycle review after Step 4

## Completion Notes

Idea 487 is closed as a routed blocker for real dynamic local-array
proof-source/path/no-clobber population.

The runbook audited real proof population inputs and defined why current data
cannot soundly bind prepared branch/compare facts to dynamic local-array
element-path consumers. No implementation should proceed in this source idea
until the prerequisite consumer-coordinate/prepared-exposure carrier exists.

Completed evidence:

- `build/agent_state/487_step1_real_proof_population_inputs/audit.md`
- `build/agent_state/487_step2_real_proof_population_contract/contract.md`
- `build/agent_state/487_step3_route_consumer_coordinate_prerequisite/route.md`
- `build/agent_state/487_step4_residual_disposition/disposition.md`

## Handoff

The exact follow-up is:

`ideas/open/488_bir_dynamic_local_array_consumer_coordinate_prepared_exposure.md`

Required follow-up scope:

- publish durable consumer coordinates for dynamic local-array element paths;
- expose records through a prepared/prealloc lookup surface shared with
  prepared control-flow branch-condition records;
- include function identity, path result, source object, derivation result,
  dynamic index value, element type, element size, element count, byte offset,
  current path status, consumer block label, consumer instruction index,
  consumer operation role, and stable prepared lookup key;
- preserve unsupported/protected statuses for pointer/provenance,
  aggregate/member, variadic/va_arg, runtime/call, bootstrap, raw-shape-only,
  target-only/final-home-only, and other non-scalar dynamic local-array paths;
- avoid proof-source, path/dominance, no-clobber, scalar-load, idea 484
  packaging, and RV64/MIR consumer work.

## Residual Disposition

After the coordinate/prepared-exposure carrier lands, proof population can
resume with:

- normalized branch/compare proof-source records for lower and upper bounds;
- operand-role matching between proof compare operands and dynamic index;
- path/dominance coverage from proof source to consumer;
- no-clobber/same-value interval facts for the dynamic index;
- feeding those facts through the idea 486 range-proof checker.

Until then, proof population, idea 484 dynamic packaging, scalar local-load
production, and RV64/MIR lowering remain blocked.

## Validation

Lifecycle validation:

```sh
git diff --check
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed
```

Both passed. `test_after.log` was absent at close time, and this lifecycle task
forbade touching canonical logs, so no new after log was generated.

## Reviewer Reject Signals

Reject reopening this source as progress if the change:

- matches branch proof sources to dynamic local-array paths by raw shape, names,
  proximity, dump order, or implicit loop/control-flow assumptions;
- implements proof-source/path/no-clobber population before consumer-coordinate
  and prepared-exposure records exist;
- marks dynamic rows available without idea 486 checker inputs;
- combines this routed blocker with scalar-load consumption, idea 484
  packaging, RV64/MIR lowering, broad generic range analysis, expectation
  rewrites, allowlist churn, or baseline/log changes.
