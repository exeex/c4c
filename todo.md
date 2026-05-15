Status: Active
Source Idea Path: ideas/open/249_prepared_i128_helper_marshaling_abi_binding.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Validate And Hand Back To I128 Pair Lowering

# Current Packet

## Just Finished

Step 5 validated the accepted idea 249 prerequisite state and records the
handoff back to i128 pair lowering. No implementation files or tests were
changed in this packet.

Accepted prepared/shared authority:

- Supported direct-result i128 `SDiv`, `UDiv`, `SRem`, and `URem` helpers carry
  source-operation mapping, helper kind, callee identity, and result ownership.
- Low/high ABI argument and direct-result ABI bindings are structured for lhs,
  rhs, and result lanes.
- Marshal/unmarshal facts connect canonical `PreparedI128Carrier` lanes to
  helper ABI argument/result bindings without fixed-register inference.
- Helper resource and clobber policy are explicit for the runtime helper
  boundary.
- Prepared helper enrichment evaluates the helper instruction program point
  against liveness intervals and builds `PreparedCallPreservedValue` facts for
  values live across supported i128 div/rem helper boundaries.
- Live preservation is complete only when every live-across-helper value has a
  known preserved route such as a stack slot or saved callee-saved register;
  unknown routes diagnose as
  `live_preservation_requires_complete_preserved_value_routes`.
- `selected_call_ownership.owns_terminal_call` becomes true only when callee
  identity, resources, clobbers, ABI bindings, marshaling/unmarshaling, and
  live preservation are all structurally complete.
- Missing liveness/regalloc/helper program-point authority diagnoses as
  `live_preservation_requires_structured_live_across_helper_facts`.

Idea 236 can resume terminal helper-call printer consumption from these
structured facts. Float/i128 conversion helpers and memory-return helper
families remain deferred.

## Suggested Next

Hand back to idea 236 so its terminal AArch64 i128 helper-call printer packet
can consume the selected helper-boundary record fields. That consumer should
emit marshaling moves, the helper call, and result unmarshaling only from the
structured ABI binding, marshal/unmarshal, clobber/resource, preserved-value,
and selected-call ownership facts.

## Watchouts

- Complete selected-call ownership is available only from structured facts;
  incomplete live-preservation states remain fail-closed and covered by tests.
- Do not treat selected-call ownership as permission to recover operands from
  fixed `x0..x5` conventions. The future printer packet must consume the
  structured ABI binding and marshal/unmarshal fields.
- `machine_printer.cpp` still rejects `I128RuntimeHelperBoundaryRecord` terminal
  output; it is only allowed to become executable when a later consumer emits
  moves and `bl` from structured record fields.
- Float/i128 conversion helpers and memory-return helper families remain
  deferred.

## Proof

No new command was required for this validation/handoff packet. The accepted
Step 4 completion proof is already rolled into `test_before.log`.

Accepted proof:

```sh
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure) > test_after.log 2>&1
```

Supervisor regression guard passed against `test_before.log` copied from
`test_baseline.log`: before 3167/3167 and after 3167/3167.
