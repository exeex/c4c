Status: Active
Source Idea Path: ideas/open/249_prepared_i128_helper_marshaling_abi_binding.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Call-Clobber And Live Preservation Authority

# Current Packet

## Just Finished

Step 4 now derives helper-boundary live-preservation authority from structured
live-across-helper program-point facts. The earlier
`review/step4_i128_helper_call_ownership.md` rejection remains addressed:
terminal-call ownership is true only when live preservation is structurally
complete.

What changed:

- Prepared helper enrichment evaluates the helper instruction program point
  against liveness intervals and builds `PreparedCallPreservedValue` facts for
  values live across supported i128 div/rem helper boundaries.
- Live preservation is complete when every live-across-helper value has a known
  preserved route such as a stack slot or saved callee-saved register; unknown
  routes diagnose as `live_preservation_requires_complete_preserved_value_routes`.
- `selected_call_ownership.owns_terminal_call` becomes true only when callee
  identity, resources, clobbers, ABI bindings, marshaling/unmarshaling, and
  live preservation are all structurally complete.
- Missing liveness/regalloc/helper program-point authority still diagnoses as
  `live_preservation_requires_structured_live_across_helper_facts`.
- Prepared printer output can expose helper preserved-value facts when present,
  and AArch64 selected helper-boundary records preserve the live-preservation
  and selected-call ownership payloads.
- Focused prepared, selected-record, dispatch, and machine-printer tests cover
  complete ownership plus incomplete live-preservation fail-closed behavior.

Terminal helper-call printing still fails closed; this packet only publishes
the structured ownership facts needed for a later printer consumer before
`bl <callee>` is allowed.

## Suggested Next

Execute the validation/handoff packet for idea 249, or hand back to idea 236 so
the selected AArch64 helper-call printer route can consume the now-structured
ABI binding, marshaling, clobber/resource, preserved-value, and selected-call
ownership facts. Terminal printing should still remain fail-closed until that
consumer packet emits real marshaling/unmarshaling and call output from the
record fields.

## Watchouts

- Complete selected-call ownership is now available only from structured facts;
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

Passed:

```sh
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_aarch64_target_instruction_records|backend_aarch64_instruction_dispatch|backend_aarch64_machine_printer)$') > test_after.log 2>&1
```

Result: build succeeded; `backend_prepare_liveness`,
`backend_prepare_frame_stack_call_contract`, `backend_prepared_printer`,
`backend_aarch64_target_instruction_records`,
`backend_aarch64_instruction_dispatch`, and `backend_aarch64_machine_printer`
passed, 6/6 tests. Proof log: `test_after.log`.

Additional hygiene: `git diff --check` passed.

Supervisor full-suite acceptance: `(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure) > test_after.log 2>&1`
passed with regression guard against `test_before.log` copied from
`test_baseline.log`; before 3167/3167 and after 3167/3167.
