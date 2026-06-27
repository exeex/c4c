# Prepared Value Materialization Contract Plan

Source idea: `ideas/open/415_prepared_value_materialization_contracts.md`

## Current Slice

Step 2 starts with `PreparedValueHomeKind::RematerializableImmediate` integer
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

This slice does not add target-local BIR expression recovery. Producer-side
diagnostics and target consumer migration are reserved for the next steps in
the runbook.

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
