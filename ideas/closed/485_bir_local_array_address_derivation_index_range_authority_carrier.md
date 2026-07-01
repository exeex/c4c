# BIR Local Array Address Derivation And Index-Range Authority Carrier

Status: Closed
Type: BIR semantic authority carrier/producer prerequisite idea
Parent: `ideas/closed/484_bir_semantic_local_address_provenance_array_element_authority.md`
Source Evidence:
- `build/agent_state/484_step3_local_address_authority_producer/blocker.md`
- `build/agent_state/484_step4_residual_disposition/disposition.md`
Owning Layer: BIR/HIR semantic local-array address derivation and index-range authority
Closed By: lifecycle review after Step 4

## Completion Notes

Idea 485 is complete for the first durable local-array carrier slice.

Step 3 implemented:

- `bir::Function::local_array_source_objects`
- `bir::Function::local_array_derivations`
- `bir::Function::local_array_element_paths`
- `LocalArrayCarrierStatus`
- `LocalArrayDerivationKind`
- `LocalArrayIndexRecord`

The implemented surface publishes source-object records for static local
arrays, derivation records for explicit local array/view GEP paths, and
element-path/layout records for constant in-bounds static local-array GEP
paths. Dynamic local-array GEP rows remain explicit unavailable carrier rows
with `missing_index_range_proof`.

Completed evidence:

- `build/agent_state/485_step1_local_array_carrier_inputs/audit.md`
- `build/agent_state/485_step2_address_derivation_index_range_contract/contract.md`
- `build/agent_state/485_step3_local_array_carrier_producer/summary.md`
- `build/agent_state/485_step4_residual_disposition/disposition.md`

## Handoff

The exact follow-up is:

`ideas/open/486_bir_index_range_proof_path_dominance_carrier.md`

Required follow-up scope:

- dynamic index lower/upper range proof;
- proof source identity;
- path/dominance validity from proof to consumer access;
- index no-clobber between proof and consumer;
- explicit unavailable statuses for missing proof, non-dominating proof,
  uncovered path, clobbered index value, unsupported comparison, raw-shape-only,
  and target-only cases.

## Residual Disposition

Idea 484 packaging may resume only after dynamic range/path authority exists
for dynamic representatives. Scalar local-load production remains blocked until
local-address provenance is available. This idea did not add scalar local-load
consumption, idea 484 packaging, prepared consumers, or RV64/MIR lowering.

Preserved boundaries:

- no loop-shape, value-name, testcase-name, dump-order, final-home, or target
  fallback inference;
- no scalar local-load consumer;
- no idea 484 packaging;
- no RV64/MIR lowering;
- aggregate/member, global, integer-pointer round-trip, variadic/runtime, F128,
  complex/vector, volatile/atomic, bootstrap, raw-shape-only, and
  target-only/final-home-only rows remain fail-closed.

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

- infers dynamic index range proof from loop shape, value names, testcase names,
  dump order, final homes, or target fallback behavior;
- consumes scalar local loads, changes idea 484 packaging, or changes RV64/MIR
  lowering before range/path dominance authority exists;
- treats dynamic GEP rows as available without proof source, path/dominance,
  and no-clobber facts;
- folds aggregate/member, global, integer-pointer round-trip,
  variadic/runtime, F128, complex/vector, volatile/atomic, bootstrap,
  raw-shape-only, or target-only work into this completed carrier slice;
- changes expectations, unsupported markers, allowlists, pass/fail accounting,
  runtime comparison, or baseline logs as proof of progress.
