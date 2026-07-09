Status: Active
Source Idea Path: ideas/open/634_large_selected_pointer_offset_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Reclassify Large Selected Pointer-Offset Rows

# Current Packet

## Just Finished

Step 5 of idea `634` reclassified the large selected pointer-offset residual rows for `src/ipa-sra-2.c` and `src/pr60822.c` after commit `155e89e15` and the Step 4 producer-plus-consumer admission.

Row-by-row classification is recorded in `build/agent_state/634_step5_large_selected_pointer_offset_classification.tsv`.

- `src/ipa-sra-2.c`, `foo`, `block_1`, inst `0`, selected offset `3999996`: moved past the Step 1 `unsupported_local_memory_access` owner. The current first owner is `RV64_BACKEND_RUNTIME_MISMATCH` with `c4c_exit=Segmentation fault`. `build/agent_state/634_step5_ipa_sra_2_c4c_bin_objdump.txt` shows the admitted row now materializes the large offset through `t6` and performs `lw s1, 0(t6)`. The remaining failure is not selected pointer authority, scratch/clobber target policy, or RV64 consumer admission; `main` computes the `foo` predicate from the post-`calloc` return value in `a0` instead of preserved `argc`, so this belongs to an out-of-scope scalar call-clobber/freshness owner bucket.
- `src/pr60822.c`, `Avg`, `entry`, inst `0`, selected offset `800000`: moved past the Step 1 `unsupported_local_memory_access` owner. The current first owner is `unsupported_global_data: RV64 object route requires supported prepared global memory facts`, captured in `build/agent_state/634_step5_pr60822_case.log`. This is an out-of-scope global-data owner bucket, not selected pointer authority, freshness/range authority, scratch/clobber target policy, or RV64 consumer admission.
- `src/pr60822.c`, `Avg`, `entry`, inst `1`, selected offset `1700004`: same movement and owner as the prior `Avg` row; it now stops at `unsupported_global_data`, not the large selected pointer-offset local-memory boundary.

## Suggested Next

Recommendation: close or split away from idea `634` rather than continuing selected pointer-offset admission work. The named in-scope large selected pointer-offset rows no longer identify selected pointer authority, freshness/range authority, scratch/clobber target policy, or RV64 consumer admission as the first blocker. If the supervisor wants more progress from these cases, the next coherent packet should target the separate scalar call-clobber/freshness owner exposed by `ipa-sra-2.c` or the global-data owner exposed by `pr60822.c`.

## Watchouts

The delegated progress script still exits nonzero because the allowlisted residual set contains many unrelated failing rows. For this packet, the target evidence is classification rather than green subset acceptance. No implementation files, tests, `plan.md`, source ideas, or root-level canonical logs were edited.

## Proof

Ran the delegated proof command:

`cmake --build --preset default && ALLOWLIST=build/agent_state/614_step3_residual_refresh/local_memory_candidates.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/634_step5_large_selected_pointer_offset.log 2>&1`

Result: exit code 1 after the build completed; the progress script reported total `35`, passed `3`, failed `32`. Target row evidence is in `build/agent_state/634_step5_large_selected_pointer_offset.log`, `build/agent_state/634_step5_ipa_sra_2_case.log`, `build/agent_state/634_step5_pr60822_case.log`, `build/agent_state/634_step5_ipa_sra_2_c4c_bin_objdump.txt`, and `build/agent_state/634_step5_large_selected_pointer_offset_classification.tsv`.
