# 497 Step 4 - Lower Blocker Classification

## Decision

Step 4 cannot truthfully publish an available bounded no-clobber result in the
current production surface.

The exact lower blocker is missing ordered effect-source stream population.
Step 3 provides `bir::LocalArrayEndpointBridgeRecord` entries with an ordered
endpoint coordinate, and selected proof-edge records carry proof-source
coordinates. The missing piece is a production builder that consumes those
records, owns the proof-source-to-endpoint interval, and enumerates every
required prepared/BIR effect family in one comparable ordered stream.

## Evidence Checked

- `src/backend/prealloc/publication_plans.cpp` populates selected proof-edge
  records through `populate_local_array_selected_proof_edge_paths`.
- `src/backend/prealloc/publication_plans.cpp` populates endpoint bridge
  records through `populate_local_array_endpoint_bridges`.
- `src/backend/bir/bir.hpp` still exposes `LocalArrayIntervalEffectInputs`
  with compatibility booleans including `endpoint_bridge_available`,
  `same_block_ordering_known`, and `effect_scan_available`.
- `rg` found no production `LocalArrayBoundedEffectScanRecord`,
  bounded-effect-scan builder, or writer for local-array bounded effect-source
  records.
- AST-backed queries confirmed the endpoint bridge writer directly calls
  `evaluate_local_array_endpoint_bridge`, while no analogous local-array
  bounded effect scan writer exists.

## Not Implemented

This packet did not add caller-supplied coverage booleans, synthetic
effect-source vectors, selected-path availability, `endpoint_bridge_available`,
or `lir_producer_instruction_index` as proof of no-clobber availability.

## Required Owner Before Availability

A lower production builder must be added before Step 4 can return available
facts. It needs to:

- consume `LocalArrayEndpointBridgeRecord` and selected proof-edge records;
- establish one ordered proof-source-to-endpoint interval;
- populate comparable effect-source records for assignments/redefinitions,
  memory accesses, phi/alias transfers, calls/helpers, inline asm,
  publications, move bundles, parallel copies, and unknown modeled effects;
- fail closed when an effect source lacks a comparable coordinate or a modeled
  family is not scanned.

Until that builder exists, the interval classifier must keep failing closed
rather than treating externally supplied coverage or compatibility booleans as
availability.
