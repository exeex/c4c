Status: Active
Source Idea Path: ideas/open/249_prepared_i128_helper_marshaling_abi_binding.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Call-Clobber And Live Preservation Authority

# Current Packet

## Just Finished

Step 4 was revised after `review/step4_i128_helper_call_ownership.md`
rejected the previous ownership claim. The slice now keeps helper boundary
policy facts structured, but it does not claim terminal-call ownership until
real live-across-helper preservation authority exists.

What changed:

- `PreparedI128RuntimeHelper` carries `LivePreservationPolicy` and
  `SelectedCallOwnershipPolicy` facts beside the helper resource, clobber, ABI
  binding, and marshal/unmarshal facts.
- Prepared helper enrichment marks caller-saved helper clobbers as modeled, but
  leaves `no_additional_live_preservation_required` false and records
  `live_preservation_requires_structured_live_across_helper_facts`.
- Prepared selected-call ownership records callee identity, resource policy,
  clobber policy, ABI bindings, and marshaling/unmarshaling as present, but
  keeps `owns_terminal_call` false while live-preservation authority is
  incomplete.
- Incomplete selected-call ownership diagnoses through helper
  `missing_required_facts` instead of letting consumers infer call readiness
  from lane names or helper opcode.
- `I128RuntimeHelperBoundaryRecord` now preserves live-preservation and
  selected-call ownership policies for AArch64 selected consumers.
- AArch64 helper-boundary record construction and selected-node status fail
  closed when live-preservation or selected-call ownership facts are incomplete.
- Focused prepared, selected-record, dispatch, and machine-printer tests cover
  the fail-closed incomplete-live-preservation state instead of proving a false
  readiness bit.

Terminal helper-call printing still fails closed; this packet only publishes
the policy and diagnostic facts needed for a later packet to derive real
live-preservation authority before `bl <callee>` is allowed.

## Suggested Next

Derive real live-preservation authority or explicit preservation facts for
supported i128 div/rem helper boundaries. Only after structured live-across-helper
facts prove that no additional preservation is required, or identify the exact
preservation moves/ownership needed, may `selected_call_ownership.owns_terminal_call`
become true and terminal helper-call printing be reconsidered.

## Watchouts

- The current active prerequisite is intentionally not complete enough for a
  selected consumer to own terminal helper-call emission. It exposes the missing
  live-preservation authority explicitly and fails closed.
- Do not treat selected-call ownership as permission to recover operands from
  fixed `x0..x5` conventions. The future printer packet must consume the
  structured ABI binding and marshal/unmarshal fields.
- `machine_printer.cpp` now names selected-call ownership and live-preservation
  authority in the helper-boundary diagnostic rather than only ABI/marshaling.
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
