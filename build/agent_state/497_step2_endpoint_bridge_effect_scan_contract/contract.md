# 497 Step 2 - Endpoint Bridge And Effect Scan Contract

## Decision

The Step 1 audit found enough surface area to define a lower-owned endpoint
bridge and bounded effect-scan contract. It did not find enough existing facts
to publish available same-value/no-clobber interval results.

Step 3 is implementable as a real bridge only if it creates or populates an
authoritative prepared/BIR endpoint binding for each supported dynamic
local-array producer row. The bridge must fail closed when the binding cannot
be proven.

## Producer Key Contract

The producer key starts from the exact `lir_producer_lookup_key` on a
`LocalArrayElementPathRecord`.

The bridge must also compare these producer-row fields before accepting an
endpoint match:

- `lir_producer_function_name`
- `lir_producer_block_label`
- `lir_producer_instruction_index`
- `lir_producer_operation_role`
- `lir_producer_coordinate_status`
- `result_name`
- `source_object_name`
- `derivation_result_name`
- `indices`
- `element_type`
- `element_size_bytes`
- `byte_offset`
- `element_count`
- `status`

For the supported dynamic local-array route, the row must describe an address
derivation for the same local-array source object and derivation result, with a
dynamic index identity that matches the address materialization endpoint. A
duplicate, partial, mismatched, or stale row identity is not a weaker available
fact; it is fail-closed bridge unavailability.

`lir_producer_instruction_index` remains a LIR producer-site coordinate. It is
never a prepared traversal index, a BIR instruction index, or an ordered effect
endpoint by itself.

## Endpoint Identity Contract

A bridged endpoint is a prepared/BIR address-derivation endpoint for the same
producer row. The endpoint identity must include:

- prepared function name and stable prepared function identity when available;
- prepared block label and stable prepared block identity when available;
- prepared block index in the owning function;
- BIR block label when this differs from the prepared block representation;
- BIR instruction index or prepared instruction index in the ordered effect
  stream;
- address-materialization kind;
- result value name and stable result value id when available;
- matched source object name;
- matched derivation result name;
- matched dynamic index identity or a precise reason it cannot be matched;
- the originating `lir_producer_lookup_key`;
- a copy of enough producer-row identity fields to reject coordinate drift.

The preferred endpoint source is a prepared address materialization whose
function, block, instruction coordinate, kind, result value, source object,
derivation result, and dynamic index identity are demonstrably in the same
prepared/BIR effect stream that the scanner will use. A BIR instruction
coordinate can be accepted only when it is bound to that same prepared address
materialization or an equivalent ordered address-derivation record.

Name interning, value homes, target final homes, selected proof-edge
availability, dump order, testcase names, and synthetic bridge booleans are not
endpoint identity.

## Interval Boundary Semantics

The bounded scan interval starts at the selected proof source and ends at the
bridged address-derivation endpoint.

The proof-source boundary must be represented in prepared/BIR coordinates, not
only by a selected path record. It must include:

- proof function name and prepared function identity when available;
- proof block label, prepared block index, and BIR block label when needed;
- proof instruction index in the ordered prepared/BIR stream;
- proof condition value and selected outcome;
- selected successor and non-selected successor labels;
- path-validity, reachability, dominance, and guard facts already produced by
  the selected proof-edge path surface.

The endpoint boundary is the accepted endpoint identity above.

Same-block intervals are valid only when both the proof source and endpoint are
in the same ordered stream and source-before-endpoint ordering is proven.
Cross-block intervals are valid only when the selected path has a concrete
ordered traversal from the proof source through the selected successor path to
the endpoint block. If either boundary is missing from that order, the result
is fail-closed `missing_order` or `unsupported_boundary`.

The scan should treat the proof-source instruction as the lower boundary and
the endpoint instruction as the upper boundary. Effects after evaluating the
proof source and before completing the address derivation are in scope. The
endpoint address-derivation instruction itself is in scope for unknown,
publication, move, or helper effects if the model says it can clobber or
change the consumed address/value identity; otherwise it may be recorded as
the terminal non-clobber address derivation. Effects before the proof source or
after the endpoint are out of scope for this contract.

## Ordered Effect Stream Requirements

The scanner must consume one ordered stream that can compare every relevant
effect to both interval boundaries. Required inputs are:

- BIR block and instruction order from `bir::Function::blocks[*].insts[*]`;
- prepared block labels and indices from prepared control-flow data;
- prepared liveness points, call points, value definition points, use points,
  and intervals when available;
- prepared address materializations and memory accesses by function, block,
  and instruction index;
- prepared call plans and call-boundary effects with phase and order index;
- move bundles and parallel-copy bundles;
- block-entry, current-block, edge, route4, route5, store-source, and
  call-argument publication records;
- inline-asm, runtime, helper, and special carrier records that may imply
  unknown effects.

If an effect source has no comparable order coordinate, the scan must not skip
it silently. It must return a precise fail-closed status unless the source can
be proven outside the interval.

## Effect Classes

The bounded scan must classify these effect classes:

- `none`: no effect in the interval can redefine, alias, clobber, publish, or
  obscure the producer address/value identity.
- `assignment_or_redefinition`: a definition, assignment, store-source update,
  value-home change, or local-array derivation update can replace the relevant
  address/value identity.
- `memory_clobber`: a memory access may write the source object, derived
  element, aliased local-array storage, or an unknown overlapping location.
- `alias_or_phi_transfer`: a phi, alias transfer, selected edge merge,
  publication, or copy can make the consumed identity differ from the proven
  identity unless tracked exactly.
- `call_or_helper`: a normal call, runtime helper, lowering helper, carrier
  helper, or call-boundary effect may clobber or publish the relevant state.
- `inline_asm`: inline assembly appears in the interval and cannot be proven
  harmless to the local-array address/value identity.
- `publication`: store-source, call-argument, edge, route4, route5,
  block-entry, or current-block publication changes visible identity or order.
- `move_or_parallel_copy`: move bundles or parallel copies can change the
  consumed value/address identity unless each move is tracked exactly.
- `unknown_modeled_effect`: any modeled effect in the interval has no safe
  class or lacks a comparable coordinate.

The implementation may use finer-grained internal classes, but the externally
reported outcome must distinguish the no-effect case from each fail-closed
family above.

## No-Clobber Criterion

An available no-clobber result is valid only when all of these are true:

- the producer row is uniquely matched by `lir_producer_lookup_key` and row
  identity fields;
- the row is bridged to a unique ordered prepared/BIR address-derivation
  endpoint for the same local-array producer;
- the proof source is represented in the same ordered prepared/BIR stream;
- the selected proof-source-to-endpoint interval is bounded and ordered;
- every required effect source in the interval is scanned or proven outside
  the interval;
- all in-interval effects classify as `none` or as terminal harmless endpoint
  address derivation under this contract;
- no dynamic index, result value, source object, derivation result, alias, phi,
  publication, move, parallel copy, call/helper, inline asm, memory access, or
  unknown effect can change or obscure the address/value identity between the
  proof source and endpoint.

If any condition is not proven, the scanner must publish an unavailable status,
not an available same-value/no-clobber result.

## Fail-Closed Statuses

Bridge statuses:

- `missing_producer_row`
- `duplicate_producer_row`
- `invalid_producer_coordinate`
- `coordinate_confusion`
- `missing_prepared_bir_endpoint_bridge`
- `duplicate_endpoint`
- `mismatched_function`
- `mismatched_result_value`
- `mismatched_source_object`
- `mismatched_derivation_result`
- `mismatched_dynamic_index`
- `unsupported_operation_role`
- `missing_endpoint_order`

Boundary and ordering statuses:

- `missing_proof_source_coordinate`
- `missing_endpoint_coordinate`
- `missing_order`
- `unsupported_boundary`
- `same_block_order_unknown`
- `selected_path_not_valid`
- `selected_path_not_guarded`

Effect statuses:

- `assignment_or_redefinition_clobber`
- `memory_clobber`
- `alias_or_phi_unresolved`
- `call_or_helper_effect_unknown`
- `inline_asm_effect_unknown`
- `publication_effect_unknown`
- `move_or_parallel_copy_unresolved`
- `unknown_modeled_effect`
- `unscanned_effect_source`

These names may be adapted to the local enum style during implementation, but
the implemented surface must retain the same distinctions. In particular,
`coordinate_confusion` is required for any attempted use of
`lir_producer_instruction_index` as a prepared/BIR endpoint coordinate.

## Implementation Implications

Step 3 should add the bridge as a lower-owned record or result object that is
keyed by `lir_producer_lookup_key` and carries the endpoint identity above. It
must fail closed on missing, duplicate, mismatched, or unordered endpoints. It
must not publish available interval facts.

Step 4 should consume the bridge record and selected proof-source path record,
build the bounded ordered interval, scan all required effect sources, and
return either available no-clobber or the precise fail-closed status. It must
not infer no-clobber from selected path availability or synthetic bridge flags.

The existing synthetic booleans `endpoint_bridge_available`,
`same_block_ordering_known`, and `effect_scan_available` are not sufficient
implementation endpoints. They may only remain as fail-closed compatibility
surfaces until backed by the concrete bridge and scan results.

## Test Implications

Focused Step 3 bridge tests should prove:

- one dynamic local-array producer row bridges to a unique prepared/BIR
  address-derivation endpoint;
- missing endpoint fails closed;
- duplicate endpoint fails closed;
- mismatched result/source/derivation/dynamic index fails closed;
- coordinate confusion fails closed when only `lir_producer_instruction_index`
  is available.

Focused Step 4 effect-scan tests should prove:

- one bridged selected interval with no relevant effects returns available
  no-clobber;
- same-block and cross-block ordering failures fail closed;
- assignment/redefinition, memory clobber, alias/phi transfer,
  call/helper, inline asm, publication, move/parallel-copy, and unknown
  modeled effects each fail closed with distinguishable status.

These tests should exercise lower endpoint/effect facts only. They should not
populate idea 489 proof facts, idea 486 checker inputs, idea 490
certification, idea 484 packaging, scalar local-load consumption, RV64/MIR
lowering, unsupported expectation rewrites, allowlists, or pass/fail
accounting changes.

## Step 3 Handoff

The bridge is implementable as a lower-owned prepared/BIR endpoint binding.
The exact lower prerequisite for Step 3 is to create the authoritative binding
from `lir_producer_lookup_key` plus row identity to a unique ordered prepared
address-materialization or equivalent BIR address-derivation endpoint.

If Step 3 cannot find or create that authoritative endpoint binding, it should
stop on `missing_prepared_bir_endpoint_bridge` or the more precise bridge
status above and leave available same-value/no-clobber unpublished.

## Proof

```sh
git diff --check
```
