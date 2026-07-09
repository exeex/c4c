Status: Active
Source Idea Path: ideas/open/629_prepared_return_destination_home_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Return Authority Evidence

# Current Packet

## Just Finished

Step 1 refreshed return destination-home authority evidence for the representative rows:

- `src/20001130-2.c`: RV64 object route rejects `output_25` at `phase=before_return` with `unsupported_move_bundle_target_shape`; the prepared move is `reason=return_stack_to_register`, `destination_kind=function_return_abi`, `destination_storage=register`, `destination_width=1`, `source_home_kind=stack_slot`, `destination_home_kind=stack_slot`, `source_type=ptr`, `destination_type=ptr`, and `authority=none`.
- `src/20080719-1.c`: RV64 object route rejects `xxx` at `phase=before_return` with the same `unsupported_move_bundle_target_shape`; the prepared move is `reason=return_stack_to_register`, `destination_kind=function_return_abi`, `destination_storage=register`, `destination_width=1`, `source_home_kind=stack_slot`, `destination_home_kind=stack_slot`, `source_type=ptr`, `destination_type=ptr`, and `authority=none`.

No adjacent return-transfer rows were revealed by this focused two-row scan. The current blocker is still missing prepared function-return transfer authority and destination-home publication for return stack-to-register moves; it is not a scalar call/result, pointer stack-result, FPR policy, runtime, local/global, variadic/library, or expectation/unsupported-marker bucket.

## Suggested Next

Execute Step 2: trace the prepared return producer authority boundary for `return_stack_to_register` before-return move bundles, identify the carrier fields that should publish source home, destination home, width, return ABI storage, and function-return association, and name the smallest producer or prepared-layer coverage packet that can make the facts explicit without RV64 ABI inference.

## Watchouts

Both rows currently use pointer-typed return values, but the rejection is specifically a function-return ABI before-return move with `authority=none`; do not route it to the pointer stack-result bucket unless Step 2 finds a distinct non-return owner. Do not infer return destination homes from ABI convention, final assembly shape, source filenames, or runtime behavior. Keep scalar call/result transport, pointer stack-results, FPR policy, generic move-bundle authority, runtime triage, local/global repair, variadic/library policy, expectation changes, unsupported marker changes, allowlists, timeouts, and accounting outside this idea.

## Proof

Ran the delegated evidence command:

`cmake --build --preset default && ALLOWLIST=build/agent_state/629_step1_return_authority.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/629_step1_return_authority.log 2>&1`

Build completed and the focused scan returned the expected current blocker state: `total=2 passed=0 failed=2`. Log paths: `build/agent_state/629_step1_return_authority.log`, `build/rv64_gcc_c_torture_backend/src_20001130-2.c/case.log`, and `build/rv64_gcc_c_torture_backend/src_20080719-1.c/case.log`.
