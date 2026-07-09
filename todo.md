Status: Active
Source Idea Path: ideas/open/628_fpr_abi_frame_policy_and_placement.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Reclassify Representative Rows

# Current Packet

## Just Finished

Step 5 re-ran the idea 628 representative RV64 gcc-torture rows with an exact five-row allowlist and classified the post-Step 3/4 results.

The scan passed three rows:
- `src/ieee/compare-fp-2.c`
- `src/ieee/unsafe-fp-assoc.c`
- `src/pr39501.c`

The scan still failed two rows, both outside FPR ABI/frame authority:
- `src/980605-1.c`: blocked by `unsupported_call_abi` for ordinary same-module call ABI/result lowering on `getval()` with scalar `i32` result and zero arguments. This is not an FPR home, FPR result, FPR save-slot placement, or FPR consumer-admission residual.
- `src/ieee/unsafe-fp-assoc-1.c`: blocked by `ambiguous_non_parallel_multi_source_stack_destination` / `unsupported_prepared_move_bundle_classification`, where two register sources feed one stack destination without ordering or mutually-exclusive authority. This remains the prepared move bundle fan-in owner bucket, not FPR ABI/frame placement.

Recommendation: treat idea 628 as close-ready from the executor perspective. The representative FPR ABI/frame rows that remain in this five-row probe either pass or are now blocked by non-FPR owner buckets. Split future work only for the scalar same-module call ABI/result bucket and the prepared move fan-in-to-stack bucket; do not broaden idea 628 for those residuals.

## Suggested Next

Ask the plan owner to decide whether to close idea 628 or record closure notes. No further FPR ABI/frame authority packet is recommended from this scan.

## Watchouts

The delegated scan command returned nonzero because two representative rows still fail. That nonzero status is expected for this classification packet and is recorded in `build/agent_state/628_step5_fpr_representatives.log`. The allowlist and scan log are non-root artifacts under `build/agent_state/` as requested.

Do not classify `src/980605-1.c` as FPR ABI/frame work just because it contains double arithmetic; the current object-route rejection is on ordinary scalar same-module calls before FPR frame placement is reached. Do not classify `src/ieee/unsafe-fp-assoc-1.c` as FPR frame work unless a later probe reaches an FPR-specific rejection; the present blocker is producer authority for non-parallel register-source fan-in to one stack destination.

## Proof

Created `build/agent_state/628_step5_fpr_representatives.allowlist` with:
- `src/980605-1.c`
- `src/ieee/compare-fp-2.c`
- `src/ieee/unsafe-fp-assoc.c`
- `src/ieee/unsafe-fp-assoc-1.c`
- `src/pr39501.c`

Ran `cmake --build --preset default && ALLOWLIST=build/agent_state/628_step5_fpr_representatives.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/628_step5_fpr_representatives.log 2>&1`.

Result: build passed; focused scan returned nonzero with total=5, passed=3, failed=2. Log paths:
- `build/agent_state/628_step5_fpr_representatives.log`
- `build/rv64_gcc_c_torture_backend/src_980605-1.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_ieee_unsafe-fp-assoc-1.c/case.log`
