Status: Active
Source Idea Path: ideas/open/240_phase_e5_route3_memory_source_identity_adapter_follow_up.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Prove Fallback And Public Compatibility

# Current Packet

## Just Finished

Step 3 proved fallback and public compatibility for the Route 3 same-block
global-load adapter without adding tests.

Existing focused coverage was sufficient:

- `backend_aarch64_instruction_dispatch` covers selected positive agreement via
  `load_global_call_argument_uses_got_for_got_required_global()`, where the
  matching Route 3 identity still emits the prepared GOT-required load/move/call
  route.
- `backend_aarch64_instruction_dispatch` covers relevant non-agreement fallback
  via `load_global_call_argument_preserves_prepared_got_for_route3_fallbacks()`
  for absent Route 3 address, non-global Route 3 base, mismatched Route 3 global
  symbol, and mismatched Route 3 memory flags.
- `backend_aarch64_prepared_memory_operand_records` covers same-block
  global-load identity agreement with the prepared oracle, before-producer
  fail-closed behavior, root type mismatch rejection, non-global root rejection,
  string-load fail-closed behavior, and nearby FP/I32 same-feature rows.
- `backend_prepared_lookup_helper` preserves the public helper/oracle contract
  for the same-block global-load query, including match, before-producer
  fail-closed, and non-global or mismatched root fail-closed behavior.
- No expectation files, baselines, helper names, supported-path status,
  wrapper output, or public prepared lookup APIs were changed.

## Suggested Next

Execute Step 4 by validating that target-policy surfaces remain unchanged:
inspect the code/test diff for expectation rewrites or target-policy ownership
movement, then run the supervisor-selected proof for the Step 4 packet.

## Watchouts

- Do not delete, privatize, rename, or hide `memory_accesses`,
  `PreparedFunctionLookups`, or `PreparedBirModule`.
- Do not move address formation, materialization, relocation, final operands,
  value homes, wrappers, diagnostics, fallback, or target emission policy into
  Route 3 ownership.
- The adapter deliberately does not extend the existing `globals.cpp`
  `prepared_current_global_load_access(...)` helper because that helper
  compares policy-bearing fields outside this selected semantic reader.
- Step 3 intentionally did not add tests because the delegated subset already
  contains explicit positive, fallback, helper/oracle, and nearby same-feature
  coverage for the selected reader.

## Proof

Supervisor-selected proof ran and passed:

```bash
cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_prepared_memory_operand_records|backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$' --output-on-failure > test_after.log
```

Result: 3/3 tests passed in `test_after.log`
(`backend_aarch64_prepared_memory_operand_records`,
`backend_aarch64_instruction_dispatch`, and
`backend_prepared_lookup_helper`).
