# 497 Step 1 - Producer-To-Endpoint Surface Audit

## Decision

A bounded endpoint bridge can be built only as a new explicit
prepared/BIR-owned record. The current surfaces are sufficient to define the
contract, key, and candidate endpoint/effect inputs, but they are not
sufficient to publish available interval facts today.

The exact lower blocker is the absence of an authoritative prepared/BIR
endpoint binding for a `LocalArrayElementPathRecord` keyed by
`lir_producer_lookup_key`. Existing rows preserve the LIR producer-site
coordinate, but no current record ties that row to an ordered prepared/BIR
address-derivation endpoint in the effect stream.

## Producer Rows

- `bir::Function::local_array_source_objects` records local array objects and
  slot layout.
- `bir::Function::local_array_derivations` records address derivation result,
  source object, base view, derivation kind, and base index. It is not ordered.
- `bir::Function::local_array_element_paths` is the producer row family for
  this bridge. Concrete fields are `result_name`, `source_object_name`,
  `derivation_result_name`, `indices`, `element_type`, `element_size_bytes`,
  `byte_offset`, `element_count`, `scalar_in_bounds`, `status`,
  `lir_producer_function_name`, `lir_producer_block_label`,
  `lir_producer_instruction_index`, `lir_producer_operation_role`,
  `lir_producer_lookup_key`, and `lir_producer_coordinate_status`.
- Producer rows are emitted from local-array GEP lowering through
  `publish_local_array_path_record`. The lookup key currently includes function
  name, LIR block label, LIR instruction index, result name, source object,
  derivation result, and index spellings.
- Dynamic local-array rows use `LocalArrayCarrierStatus::MissingIndexRangeProof`
  and a single dynamic `LocalArrayIndexRecord` when the address derivation is
  recognized but not proven in range.

## Prepared/BIR Endpoint Candidates

- `bir::Block::insts` gives BIR instruction order inside a BIR block and can be
  an endpoint stream only when a prepared/BIR coordinate is bound explicitly.
- `PreparedNameTables` can intern the producer function, block, and result
  names, but name interning alone is not endpoint proof.
- `PreparedControlFlowFunction::blocks` and
  `find_prepared_block_index_in_function` can map a prepared block label to a
  prepared block index.
- `PreparedAddressingFunction::address_materializations` records
  `(function_name, block_label, inst_index, kind, result_value_name, result_value_id, ...)`.
  It is the closest existing prepared address-derivation endpoint candidate,
  but current local-array producer rows do not require or record a match to it.
- `PreparedAddressingFunction::accesses` records memory accesses with
  `(function_name, block_label, inst_index, result_value_name/stored_value_name,
  address)`. It can help classify memory effects but is not the address
  derivation endpoint by itself.
- `PreparedValueLocationFunction::value_homes` and move bundles can identify
  homes and moves for values, but final homes or target placement are not
  endpoint proof.

Rejected endpoint shortcuts:

- `lir_producer_instruction_index` is only a LIR coordinate, not a prepared
  traversal index or BIR instruction index.
- selected proof-edge availability, testcase shape, target behavior, final
  homes, synthetic bridge booleans, and dump order are not endpoint proof.

## Selected Proof-Source And Path Inputs

- `bir::Function::local_array_selected_proof_edge_paths` is populated by
  `populate_local_array_selected_proof_edge_paths`.
- `make_selected_proof_edge_path_inputs` derives proof inputs from
  `PreparedControlFlowFunction::branch_conditions`, reachability/dominance
  matrices, and the local-array element path row.
- Selected path records carry the current lower inputs needed by the effect
  interval: producer lookup key, producer function/block/LIR coordinate,
  operation role, coordinate status, proof function/block, proof condition
  value, predicate, compare type, lhs/rhs operands, proof instruction index,
  selected outcome, selected and non-selected successor labels, path validity,
  reach/cover booleans, and proof dominates/guards booleans.
- Same-block selected paths already fail closed without authoritative same-block
  ordering. Cross-block paths still need a bridged prepared/BIR endpoint before
  any available no-clobber fact can be true.

## Effect-Stream Ordering Surfaces

Candidate effect surfaces that a future bounded scan must integrate:

- BIR instruction order: `bir::Function::blocks[*].insts[*]`.
- Prepared block order and labels: `PreparedControlFlowFunction::blocks`.
- Prepared liveness points: `PreparedLivenessFunction::blocks` with
  `start_point`/`end_point`, `call_points`, value definition/use points, and
  intervals.
- Prepared memory/addressing: `PreparedMemoryAccess` and
  `PreparedAddressMaterialization` by prepared block label and instruction
  index.
- Calls/helpers: `PreparedCallPlan` by block and instruction, plus
  `plan_prepared_call_boundary_effects` / `PreparedCallBoundaryEffectPlan`
  with phase and `order_index`.
- Move bundles and parallel copies:
  `PreparedValueLocationFunction::move_bundles`,
  `PreparedParallelCopyBundle`, and block-entry/current-block publication
  records.
- Publications:
  `PreparedStoreSourcePublicationRecord`,
  `PreparedCallArgumentValuePublicationFact`, `PreparedEdgePublication`, and
  route4/route5 publication records.
- Inline asm/runtime/special carriers: BIR `CallInst::inline_asm` and prepared
  carrier/helper plan families must conservatively classify unknown effects.

## Bridge Shape Available From The Audit

A truthful Step 2 contract can use this key and endpoint model:

- Key: exact `lir_producer_lookup_key`, plus function and row fields to reject
  duplicate/mismatched rows.
- Endpoint identity: prepared function id/name, prepared block label/id,
  prepared block index, BIR instruction index, result value id/name, and a
  pointer to or copy of the matched local-array producer row identity.
- Endpoint match conditions: same function, same derived result value,
  same source object/derivation result, same dynamic index identity, operation
  role `AddressDerivation`, and an address-materialization or BIR instruction
  endpoint that is demonstrably in the prepared/BIR effect stream.
- Fail-closed statuses needed before implementation: missing endpoint,
  duplicate endpoint, mismatched result/source/derivation/dynamic index,
  coordinate confusion, missing order, unsupported boundary, clobber, alias or
  phi unresolved, unknown call/helper/inline-asm/publication/move/parallel-copy
  effect.

## Lower Blocker

Do not implement available interval facts from current inputs alone. The
current bridge/effect evaluator has only booleans:
`endpoint_bridge_available`, `same_block_ordering_known`, and
`effect_scan_available`. Those booleans are intentionally synthetic until a
lower owner adds a concrete endpoint record and bounded scan result.

The next packet should define the endpoint bridge/effect scan contract around
the surfaces above. If implementation starts afterward, it must first add a
real prepared/BIR endpoint binding for local-array producer rows, then scan the
selected proof-source-to-endpoint interval across the ordered effect surfaces.

## Proof

```sh
git diff --check
```

Result: passed.
