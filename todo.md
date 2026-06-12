Status: Active
Source Idea Path: ideas/open/224_phase_e2_control_flow_branch_target_helper_private_pass_context.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce Private Pass-Context Branch-Target Identity Read

# Current Packet

## Just Finished

Completed Step 1 - Map Branch-Target Helper Consumers And Baseline by
inventorying the public helper family around
`find_prepared_control_flow_branch_target_labels(...)` before code edits.

Discovered direct public callers and immediate wrappers:

- `src/backend/prealloc/control_flow.hpp:472`: two-argument
  `find_prepared_control_flow_branch_target_labels(function_cf, block_label)`.
  Classification: retained prepared fallback/public prepared authority. It
  rejects invalid labels, absent blocks, non-conditional blocks, and invalid
  prepared targets, then returns prepared true/false successor ids.
- `src/backend/prealloc/control_flow.hpp:492`: three-argument
  `find_prepared_control_flow_branch_target_labels(function_cf, block_label,
  source_block)`. Classification: selected identity read plus retained
  prepared fallback. It may read agreeing structured BIR successor ids when the
  BIR terminator is a conditional branch with valid ids matching prepared, but
  every absent, invalid-id, mismatch, or non-conditional path returns the
  prepared two-argument result.
- `src/backend/prealloc/control_flow.hpp:584`: immediate wrapper
  `resolve_prepared_compare_branch_target_labels(...)`. Classification:
  retained compare-branch policy/oracle gate plus selected identity
  verification. It first uses prepared branch-condition targets, then calls the
  three-argument helper only to prove control-flow target agreement; mismatch
  returns `std::nullopt`, and missing control-flow targets keep the prepared
  branch-condition targets.
- `src/backend/prealloc/control_flow.hpp:612`: immediate overload wrapper for
  `resolve_prepared_compare_branch_target_labels(...)` resolving a BIR block
  label through prepared names. Classification: retained public wrapper and
  prepared name-resolution behavior.
- `src/backend/prealloc/control_flow.hpp:2122`: short-circuit join helper uses
  `resolve_prepared_compare_branch_target_labels(...)`. Classification:
  retained prepared join/branch policy and selected identity verification; not
  a direct branch-target identity consumer to migrate first.
- `src/backend/mir/aarch64/codegen/comparison.cpp:154`: production direct call
  to the three-argument helper when a conditional branch has no compare facts.
  Classification: selected identity read with prepared fallback retained for
  non-compare branch target labels.
- `src/backend/mir/aarch64/codegen/comparison.cpp:164`: production call to
  `resolve_prepared_compare_branch_target_labels(...)` for compare branches.
  Classification: retained prepared compare-branch policy/oracle behavior and
  selected target-agreement verification.
- `src/backend/mir/x86/module/module.cpp:4424`, `:4814`, and `:5653`:
  production calls to `resolve_prepared_compare_branch_target_labels(...)` in
  local-slot/immediate guard lowering. Classification: retained prepared
  compare-branch policy, drift rejection, and expected output behavior with
  selected target-agreement verification.
- `src/backend/mir/x86/module/module.cpp:719` plus
  `prepared_compare_join_owns_branch_target_block(...)`: production handoff
  validation consumes prepared control-flow block/branch ownership without
  directly calling the target helper. Classification: retained prepared
  fallback/policy/handoff responsibility.
- `src/backend/prealloc/prepared_printer/control_flow.cpp:151` and `:217`:
  prepared control-flow printer/debug rows render branch, branch-condition,
  join-transfer, continuation-target, and parallel-copy data. Classification:
  retained printer/debug output responsibility; no selected identity read
  should migrate here in Step 2.
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:2114-2187`:
  helper-oracle direct calls cover prepared-only positive, absent prepared
  block, agreeing structured BIR ids, raw-name drift, invalid ids, id mismatch,
  non-conditional BIR fallback, and invalid prepared source label rejection.
  Classification: retained oracle/fallback/status coverage plus selected
  identity-read proof.
- `tests/backend/bir/backend_x86_handoff_boundary_joined_branch_test.cpp`
  joined-branch `check_route_outputs(...)` and
  `check_join_route_*prepared_control_flow(...)` consumers. Classification:
  retained wrapper/output/expected-string and prepared handoff behavior, not a
  direct helper migration target.

## Suggested Next

Proceed to Step 2: introduce the private pass-context branch-target identity
read for only the selected agreement-proven structured successor identity path.
The next code packet should target the three-argument helper path and its safe
production selected-identity caller first, while leaving public prepared
fallback, compare-branch policy, printer/debug, oracle, wrapper, and
expected-string responsibilities intact.

## Watchouts

- The two-argument helper remains the retained prepared fallback/public
  authority for absent, invalid-label, non-conditional, mismatch, and invalid-id
  paths.
- `resolve_prepared_compare_branch_target_labels(...)` is an immediate wrapper
  with policy/oracle semantics: missing control-flow agreement can preserve
  prepared branch-condition targets, but target drift returns `std::nullopt`.
- AArch64 `comparison.cpp` has the only direct production selected-identity
  call found by `rg`; x86 production consumers use the compare resolver wrapper
  and handoff validators.
- Keep prepared printer/debug rows, x86 joined-branch wrapper outputs,
  helper-oracle strings/statuses, and expected strings byte-stable.
- No aggregate `PreparedControlFlow`, `PreparedFunctionLookups`, or
  `PreparedBirModule` retirement belongs in Step 2.

## Proof

Docs/inventory-only packet; no build or ctest execution was required and no
`test_after.log` was created.

Supervisor-selected implementation proof route recorded for the next
code-changing packet:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering'
```

`ctest --test-dir build -N -R 'backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering'`
showed exactly these two tests:

- `backend_aarch64_branch_control_lowering`
- `backend_prepared_lookup_helper`
