Status: Active
Source Idea Path: ideas/open/634_large_selected_pointer_offset_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Large Selected Pointer-Offset Evidence

# Current Packet

## Just Finished

Step 1 of idea `634` refreshed the selected pointer-offset local-memory evidence for `src/ipa-sra-2.c` and `src/pr60822.c`.

Rows classified in `build/agent_state/634_step1_large_selected_pointer_offset_classification.tsv`:

| Test | In-scope rows | Facts | Current rejection |
| --- | --- | --- | --- |
| `src/ipa-sra-2.c` | `foo` block_1 inst 0: `pointer_value` `%p.agg` in `a1`, offset `3999996`, width `4`; `foo` block_2 inst 0 is `%p.agg+0` and classified as narrow/out-of-scope for this idea. | Prepared addressing gives `base_plus_offset=yes`, `layout_authority=unknown`, `range_verdict=unknown_compatible`; address space is only default-inferred, with no explicit per-access address-space field. Store-local publications after the loads carry producer-rematerialization freshness, but the load-local access row itself does not expose explicit memory-use freshness. | Case fails in object emission with `unsupported_local_memory_access: RV64 object route requires prepared frame-slot or pointer-value base-plus-offset local memory addressing`. |
| `src/pr60822.c` | `Avg` entry inst 0: `pointer_value` `%p.p` in `a0`, offset `800000`, width `4`; `Avg` entry inst 1: `%p.p+1700004`, width `4`. | Both pointer rows have `base_plus_offset=yes`, `layout_authority=unknown`, `range_verdict=unknown_compatible`; address space is only default-inferred, and no explicit per-access memory-use freshness field is visible on the load rows. The `main` `@x+800000` and `@x+1700004` rows are direct global-symbol stores with proven bounds and are out of scope. | Case fails at the same object-emission `unsupported_local_memory_access` admission boundary. |

First missing/rejecting boundary: prepared-addressing already exposes selected pointer base identity, selected offset, access width, and large-offset range classification for the three in-scope rows, but the row does not yet carry explicit per-access address-space, memory-use freshness, scratch-register authority, or clobber-safety facts for RV64 large-offset materialization. Visible scratch/clobber evidence is limited to call-plan reserved scratch/clobber facts such as `gpr:t0`; no local-memory-row scratch/clobber contract is visible.

## Suggested Next

Step 2 should trace the prepared carrier boundary for these pointer-value access rows and decide where to publish explicit selected local-memory authority: base identity, offset, width, default address space, load memory-use freshness, and large-offset range status. Keep `pr60822` direct global-symbol stores, the `ipa-sra-2` zero-offset pointer load, frame-slot rows, and scratch/clobber policy out of this producer packet unless the trace proves the same carrier already owns them.

## Watchouts

The current diagnostic is generic and does not distinguish a missing selected pointer carrier from missing RV64 scratch/clobber policy. Do not turn Step 2 into a named-case admission shortcut or final-assembly inference. The non-overfit boundary is a general prepared selected pointer-offset local-memory fact path for parameter-pointer rows with large offsets.

## Proof

Ran the delegated proof:

`cmake --build --preset default && ALLOWLIST=build/agent_state/614_step3_residual_refresh/local_memory_candidates.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/634_step1_large_selected_pointer_offset.log 2>&1`

Result: build succeeded with no work to do; focused allowlist probe returned nonzero because the residual subset still has expected failures (`total=35 passed=3 failed=32`). The two target cases still fail:

- `src/ipa-sra-2.c`: `build/rv64_gcc_c_torture_backend/src_ipa-sra-2.c/case.log`
- `src/pr60822.c`: `build/rv64_gcc_c_torture_backend/src_pr60822.c/case.log`

Proof log: `build/agent_state/634_step1_large_selected_pointer_offset.log`.
