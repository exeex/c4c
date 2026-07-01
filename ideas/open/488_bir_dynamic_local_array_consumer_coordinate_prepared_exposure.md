# BIR Dynamic Local-Array Consumer Coordinate Prepared Exposure

Status: Open
Type: BIR/prepared metadata exposure prerequisite idea
Parent: `ideas/closed/487_bir_dynamic_local_array_proof_source_path_no_clobber_population.md`
Source Evidence:
- `build/agent_state/487_step3_route_consumer_coordinate_prerequisite/route.md`
- `build/agent_state/487_step4_residual_disposition/disposition.md`
Owning Layer: BIR local-array carrier to prepared consumer-coordinate exposure

## Goal

Publish durable consumer coordinates and prepared lookup keys for dynamic
local-array element paths so later proof-source/path/no-clobber population can
bind branch facts to a specific consumer interval without raw-shape inference.

## Why This Exists

Idea 487 could identify dynamic local-array element paths and prepared
branch/compare facts separately, but it could not soundly connect one proof
source to one dynamic local-array consumer. The missing prerequisite is a
coordinate-bearing prepared exposure surface for the dynamic local-array
consumer.

## In Scope

- Publish a durable record for each supported dynamic local-array element-path
  consumer.
- Carry function identity, path result, source object, derivation result,
  dynamic index value, element type, element size, element count, byte offset,
  current path status, consumer block label, consumer instruction index, and
  consumer operation role.
- Expose a stable prepared/prealloc lookup key by function, block,
  instruction, path result, source object, derivation result, and dynamic index.
- Preserve unsupported/protected-boundary statuses for pointer/provenance,
  aggregate/member, variadic/va_arg, runtime/call, bootstrap, raw-shape-only,
  target-only/final-home-only, and other non-scalar dynamic local-array paths.
- Keep this as coordinate/prepared-exposure metadata only.

## Out Of Scope

- Populating proof-source records, path/dominance certificates, or no-clobber
  facts.
- Marking dynamic local-array range proofs available.
- Idea 486 checker vocabulary changes.
- Idea 484 packaging.
- Scalar local-load consumption.
- RV64/MIR lowering.
- Broad generic range analysis.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline/log
  churn.

## Acceptance Criteria

- Focused audit identifies the BIR/prepared surfaces that can carry dynamic
  local-array consumer coordinates.
- The contract names required coordinate fields, lookup keys, operation roles,
  and fail-closed statuses.
- Positive coverage proves at least one dynamic local-array element path has a
  stable consumer coordinate/prepared exposure record.
- Negative coverage rejects unsupported boundaries and raw-shape/name/proximity
  inference.
- Fresh residual disposition states whether real proof-source/path/no-clobber
  population can resume or whether another prerequisite remains first.

## Reviewer Reject Signals

Reject patches that:

- infer consumer coordinates from dump order, testcase names, value names,
  branch proximity, loop shape, final homes, or RV64 target behavior;
- populate proof-source, path/dominance, or no-clobber facts in this coordinate
  exposure packet;
- mark dynamic range proofs available, change idea 486 checker vocabulary,
  package idea 484 records, consume scalar local loads, or change RV64/MIR
  lowering;
- weaken unsupported diagnostics, expectations, allowlists, pass/fail
  accounting, runtime comparison, or baseline logs as proof of progress;
- claim synthetic or raw-shape matching as durable prepared exposure.
