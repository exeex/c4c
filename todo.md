Status: Active
Source Idea Path: ideas/open/236_aarch64_i128_pair_lowering.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Consume Prepared I128 Runtime Helper Boundaries

# Current Packet

## Just Finished

Step 6 added selected AArch64 i128 runtime helper-boundary records for the
prepared div/rem helper authority supplied by idea 248.

What changed:

- Added `I128RuntimeHelperBoundaryRecord` as a selected machine-node payload
  for supported i128 `SDiv`, `UDiv`, `SRem`, and `URem` helper boundaries.
- The selected record consumes `PreparedI128RuntimeHelper` facts directly:
  helper family/kind, callee identity, source opcode, source/result type,
  function/block/instruction identity, result/operand value ids and names,
  direct low/high result ownership, resource policy, ABI/register-bank policy,
  and prepared caller-saved clobbers.
- Low/high argument and result operands are selected through canonical
  `PreparedI128Carrier` register-pair facts and cross-checked against the
  helper lane bindings, rather than inferred from register names, adjacency,
  opcodes, or fixed ABI registers.
- AArch64 dispatch now routes i128 div/rem source operations to matching
  prepared helper records by prepared block/instruction identity. Missing
  helper records, incomplete helper policy, missing clobbers, missing carriers,
  unsupported carrier kinds, and non-direct result ownership fail closed with
  explicit diagnostics.
- Focused target-record and dispatch tests prove signed/unsigned helper record
  selection, callee/kind preservation, lane preservation, clobber/resource/ABI
  facts, and missing-helper or missing-clobber diagnostics.

No terminal printer output, runtime library changes, target-local helper-call
synthesis, fixed `x0`/`x1` marshaling, scalar-i64 substitute, float/i128
conversion helper mapping, or memory-return helper-family support was added.

## Suggested Next

Execute Step 7 printer work for the supported structured i128 selected-record
subset. The printer packet should consume selected pair/transport and
`I128RuntimeHelperBoundaryRecord` fields directly, keep incomplete helper or
carrier facts fail-closed, and avoid recovering operands from rendered names,
fixed conventions, or source opcodes.

Suggested focused proof:

```sh
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_target_instruction_records|backend_aarch64_target_record_core_contract|backend_aarch64_mir_carrier|backend_aarch64_instruction_dispatch|backend_aarch64_machine_printer)$') > test_after.log 2>&1
```

## Watchouts

- `I128RuntimeHelperBoundaryRecord` is a selected machine-node handoff only;
  `machine_printer.cpp` has not been updated for helper output in this packet.
- Current helper support is div/rem with direct low/high result lanes only.
  Float/i128 conversion helper mapping and memory-return helper families remain
  deferred until separate prepared/shared authority exists.
- Dispatch uses the opcode only to identify the helper-required source family
  and then requires a matching prepared helper record. Do not move helper
  callee/kind selection into AArch64 lowering.
- `PreparedCallPlan` remains retained-call-only; helper-required i128 source
  operations are represented by the new helper-boundary record, not fake call
  plans.

## Proof

Passed:

```sh
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepared_printer|backend_aarch64_target_instruction_records|backend_aarch64_target_record_core_contract|backend_aarch64_mir_carrier|backend_aarch64_instruction_dispatch)$') > test_after.log 2>&1
```

Result: build succeeded; `backend_prepare_liveness`,
`backend_prepared_printer`, `backend_aarch64_target_instruction_records`,
`backend_aarch64_target_record_core_contract`, `backend_aarch64_mir_carrier`,
and `backend_aarch64_instruction_dispatch` passed, 6/6 tests. Proof log:
`test_after.log`.

Additional hygiene: `git diff --check` passed.

Supervisor full-suite acceptance also passed for this Step 6 slice:

```sh
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure) > test_after.log 2>&1
```

Regression guard used `test_before.log` copied from accepted
`test_baseline.log`; result was 3167/3167 before and 3167/3167 after.
