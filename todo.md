Status: Active
Source Idea Path: ideas/open/617_scalar_compare_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Scalar Compare Publication Evidence

# Current Packet

## Just Finished

Completed Step 1 (`Refresh Scalar Compare Publication Evidence`) with fresh current-tree targeted probes under `/tmp/c4c_617_step1_probe`. The earlier broad `build/rv64_gcc_c_torture_backend` artifact is stale for full classification because it still contains select `unsupported_source_stack_offset` rows that idea `616` Step 3 already proved moved; keep that broad artifact only as stale candidate/context evidence, not as accepted post-616 classification.

Fresh accepted Step 1 target set:

- `src/20000731-1.c`: current probe failed at `[RV64_C4C_OBJ_COMPILE_FAIL]`; first owner `unsupported_scalar_compare_publication`; log `/tmp/c4c_617_step1_probe/src_20000731-1.c/case.log`.
- `src/20011217-1.c`: current probe failed at `[RV64_C4C_OBJ_COMPILE_FAIL]`; first owner `unsupported_scalar_compare_publication`; log `/tmp/c4c_617_step1_probe/src_20011217-1.c/case.log`.
- `src/930603-1.c`: current probe failed at `[RV64_C4C_OBJ_COMPILE_FAIL]`; first owner `unsupported_scalar_compare_publication`; log `/tmp/c4c_617_step1_probe/src_930603-1.c/case.log`.
- `src/990117-1.c`: current probe failed at `[RV64_C4C_OBJ_COMPILE_FAIL]`; first owner `unsupported_scalar_compare_publication`; log `/tmp/c4c_617_step1_probe/src_990117-1.c/case.log`.
- `src/gofast.c`: current probe failed at `[RV64_C4C_OBJ_COMPILE_FAIL]`; first owner `unsupported_scalar_compare_publication`; log `/tmp/c4c_617_step1_probe/src_gofast.c/case.log`.
- `src/loop-8.c`: current probe failed at `[RV64_C4C_OBJ_COMPILE_FAIL]`; first owner `unsupported_scalar_compare_publication`; log `/tmp/c4c_617_step1_probe/src_loop-8.c/case.log`.
- `src/strct-pack-1.c`: current probe failed at `[RV64_C4C_OBJ_COMPILE_FAIL]`; first owner `unsupported_scalar_compare_publication`; log `/tmp/c4c_617_step1_probe/src_strct-pack-1.c/case.log`.

Each fresh row reports `prepared module shape: unsupported_scalar_compare_publication: RV64 object route requires prepared scalar compare result homes and materializable operands`, so scalar compare publication remains a confirmed current-tree implementation/localization target.

Stale broad-context owner separation from `build/rv64_gcc_c_torture_backend` only:

- Scalar compare publication: the stale broad artifact identified the same 7 candidates, now confirmed fresh above and owned by idea `617`.
- Branch/source freshness context: 7 `unsupported_branch_stack_load_authority` rows plus 3 `unsupported_branch_stack_load_source_freshness` rows (`src/20000314-3.c`, `src/20001017-1.c`, `src/20050125-1.c`, `src/20060910-1.c`, `src/20080519-1.c`, `src/20140828-1.c`, `src/930930-1.c`, `src/990127-1.c`, `src/loop-2e.c`, `src/pr39100.c`); not owned by scalar compare publication.
- Select/source freshness context: the stale broad artifact still shows 7 `unsupported_source_stack_offset` rows (`src/20000706-1.c`, `src/20000706-2.c`, `src/20000717-5.c`, `src/20071213-1.c`, `src/20120427-1.c`, `src/20120427-2.c`, `src/991216-1.c`) and 5 `intent_status_unsupported_source_home`/`unsupported_source_home` rows (`src/pr45034.c`, `src/pr53160.c`, `src/pr58726.c`, `src/pr59221.c`, `src/pr68250.c`); the stack-offset rows are known stale after idea `616` Step 3 and must not drive Step 2.
- RV64 instruction/terminator fragments: 115 `unsupported_instruction_fragment` rows and 26 `unsupported_terminator_fragment` rows, including `src/920710-1.c` and `src/921124-1.c`; RV64 consumer/terminator ownership, not scalar compare publication.
- Runtime: 148 `[RV64_BACKEND_RUNTIME_MISMATCH]` rows and 5 `[RV64_C4C_RUN_TIMEOUT]` rows; emitted-object behavior owners.
- Other unrelated visible owners include 122 `unsupported_prepared_move_bundle_classification`, 51 `unsupported_call_abi`, 37 `unsupported_move_bundle_target_shape`, 35 `unsupported_local_memory_access`, 26 `[RV64_C4C_LINK_FAIL]`, 24 `unsupported_global_data`, 21 `unsupported_inline_asm_fragment`, 12 `unsupported_stack_frame`, 10 `[RV64_C4C_OBJ_COMPILE_TIMEOUT]`, 5 `unsupported_pointer_arithmetic`, 5 `unsupported_param_home`, 5 `unsupported_floating_cast`, 2 `unsupported_source_immediate_i32_range`, and 2 `unsupported_but_coherent` rows.

## Suggested Next

Proceed to Step 2: localize the prepared scalar compare publication authority gap for the 7 fresh-confirmed rows, starting from the prepared-layer producer/carrier/consumer diagnostics for scalar compare result homes and materializable operands.

## Watchouts

- The static recovery docs still say `3` scalar compare publication rows, and the stale broad artifact shows `7` candidates; use the fresh targeted probe set under `/tmp/c4c_617_step1_probe` as the accepted Step 2 row set unless the supervisor requests a fresh full scan.
- Do not describe `build/rv64_gcc_c_torture_backend` as post-616 full classification until it is refreshed; it is stale for select/source freshness accounting.
- Keep branch stack-load authority/source freshness, select source stack-offset/source-home, RV64 instruction/terminator fragments, runtime mismatches, and generic move-bundle target-shape rows out of the scalar compare implementation slice.
- Do not make expectation, unsupported-marker, allowlist, timeout, or accounting changes.
- Do not add named-case shortcuts for the low-count target family.

## Proof

Diagnostics-only packet. Built `c4cll` and ran targeted current-tree CMake runner probes for the 7 candidate scalar compare rows, writing temporary outputs to `/tmp/c4c_617_step1_probe` and build output to `/tmp/c4c_617_step1_build.log`. No delegated backend proof was run, and `test_after.log` was not created or modified.
