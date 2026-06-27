# Prepared Value Materialization Contract Plan

Source idea: `ideas/open/415_prepared_value_materialization_contracts.md`

## Current Slice

Step 2 of the same-block producer-chain slice adds the typed
`PreparedCallArgumentBinaryProducerMaterializationFact`, exposed by
`find_prepared_call_argument_binary_producer_materialization_fact`.

The fact is a compatibility view over existing prepared same-block source
producer records for AArch64 scalar call-argument materialization:

- destination identity: prepared value name id and BIR value type
- producer identity: producer block label, instruction index, instruction
  pointer, and producer kind `Binary`
- payload: binary opcode plus left and right operand values
- scheduling authority: same prepared block and producer instruction before the
  call instruction
- target admission: the opcode must be accepted by
  `prepared_call_argument_binary_producer_opcode_is_materializable`

Rejected records fail closed by returning no fact:

- missing source-producer lookup table or missing producer record
- invalid or mismatched destination value identity/type
- producer instruction at or after the call, outside the block, or stale against
  the BIR instruction at its recorded index
- cross-family producer payloads such as load-local, cast, select, immediate, or
  unknown producers
- unsupported binary opcodes such as unsigned division, shifts, and comparisons

The compatibility query
`find_prepared_call_argument_source_producer_materialization` now routes binary
admission through the typed fact while preserving the existing load-local
materialization path. This slice does not add target-local BIR expression
recovery, broaden Route 6 target recovery, or migrate the final AArch64
consumer contract; producer-side verifier reports and complete consumer
migration remain later runbook steps.

## Previous Pointer Slice

The pointer slice adds `PreparedValueHomeKind::PointerBasePlusOffset` facts.
The typed view is `PreparedPointerBasePlusOffsetFact`, exposed by
`as_pointer_base_plus_offset_fact`.

The fact is intentionally a compatibility view over existing value-home state:

- destination identity: prepared value id, function name id, and value name id
- base identity: base value name id plus optional base symbol name id
- payload: signed pointer byte delta, including zero
- target admission: whether a consumer may use a direct base-register copy for
  zero delta and whether the delta fits a signed 12-bit immediate field

Rejected records fail closed by returning no fact:

- value home is not `PointerBasePlusOffset`
- missing function name or destination value name identity
- missing or invalid base value name
- missing pointer byte delta
- cross-family immediate payload is also present

This slice does not add target-local pointer-expression recovery. Producer-side
diagnostics and target consumer migration are reserved for the next steps in
the runbook.

## Pointer Producer Verification

Step 3 adds `PreparedPointerBasePlusOffsetContractStatus` plus
`verify_prepared_pointer_base_plus_offset_contract`.

Producer-missing statuses:

- `missing_value_home`
- `missing_function_name`
- `missing_value_name`
- `missing_pointer_base`
- `missing_pointer_byte_delta`

Producer-incoherent statuses:

- `conflicting_home_kind`
- `conflicting_cross_family_payload`

Reports use the `value_materialization_fact` fact family and fail closed for
every non-coherent status. Coherent reports preserve function, value id, and
value name identity without emitting diagnostic detail. Prepared value id `0`
remains valid; pointer base identity and pointer byte delta carry their own
missing-state checks.

## Migrated Pointer Consumers

Step 4 migrates RV64 edge publication pointer source materialization in
`prepared_edge_publication_emit.cpp` to
`verify_prepared_pointer_base_plus_offset_contract` plus
`as_pointer_base_plus_offset_fact`.

The migrated consumer keeps its existing target checks for base-register
availability, destination register support, signed-12 add-immediate emission,
and large-delta materialization. It now fails closed before target emission for
missing destination identity, missing or invalid base identity, missing byte
delta, wrong home kind, or cross-family immediate payloads.

## Completed Immediate Slice

The first slice added `PreparedValueHomeKind::RematerializableImmediate` integer
facts. The typed view is
`PreparedRematerializableIntegerImmediateFact`, exposed by
`as_rematerializable_integer_immediate_fact`.

The fact is intentionally a compatibility view over existing value-home state:

- source identity: prepared value id, function name id, and value name id
- payload: signed 32-bit immediate stored as `std::int64_t`
- interpretation: signed integer, 32-bit width
- target admission: whether the payload fits a signed 12-bit immediate field

Rejected records fail closed by returning no fact:

- value home is not `RematerializableImmediate`
- missing `immediate_i32`
- cross-family `immediate_f128` payload is also present
- missing function name or value name identity

This slice did not add target-local BIR expression recovery.

## Producer Verification

Step 3 adds
`PreparedRematerializableIntegerImmediateContractStatus` plus
`verify_prepared_rematerializable_integer_immediate_contract`.

Producer-missing statuses:

- `missing_value_home`
- `missing_function_name`
- `missing_value_name`
- `missing_immediate_payload`

Producer-incoherent statuses:

- `conflicting_home_kind`
- `conflicting_cross_family_payload`

Reports use the `value_materialization_fact` fact family and fail closed for
every non-coherent status. Coherent reports preserve function, value id, and
value name identity without emitting diagnostic detail. Prepared value id `0`
is valid for the first prepared value; function and value names carry the
invalid sentinels for missing identity.

## Migrated Consumers

Step 4 migrates selected RV64 consumers to
`verify_prepared_rematerializable_integer_immediate_contract` plus
`as_rematerializable_integer_immediate_fact`:

- `prepared_scalar_emit.cpp` prepared integer immediate lookup
- `object_emission.cpp` integer immediate lookup for named BIR values
- `object_emission.cpp` before-return rematerialized immediate moves
- `object_emission.cpp` rematerializable binary-result detection

Non-rematerializable homes keep their existing fall-through behavior. A
rematerializable integer immediate with missing identity, missing payload, or a
cross-family payload fails closed before the target consumes the immediate.
