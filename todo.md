Status: Active
Source Idea Path: ideas/open/631_direct_global_symbol_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Reclassify Direct-Global Rows

# Current Packet

## Just Finished

Step 5 re-ran the focused direct global-symbol row probe from Step 1 after the
Step 4 RV64 consumer admission and generated fresh prepared dumps/extracts
under `build/agent_state/631_step5_*`.

The focused probe still reports `total=18 passed=0 failed=18`, but the
remaining failures do not justify another direct-global local-memory consumer
packet:

| Row | Step 5 result | Classification |
| --- | --- | --- |
| `src/20021204-1.c` | `unsupported_prepared_move_bundle_classification` | Out of scope: move-bundle authority remains the first owner. |
| `src/920429-1.c` | `unsupported_prepared_move_bundle_classification` | Out of scope: move-bundle authority remains the first owner. |
| `src/921117-1.c` | `unsupported_local_memory_access` | Out of scope: byval/aggregate local-memory copies; no direct `addr @symbol` local-memory row. |
| `src/complex-7.c` | `unsupported_local_memory_access` | Out of scope: aggregate global-object materialization into locals. |
| `src/pr46309.c` | `unsupported_local_memory_access` | Reclassified: pointer-loaded-from-global route. The refreshed dump has `%t15 = bir.load_global ptr @q` followed by `bir.load_local ... addr %t15`, not `addr @q`. |
| `src/pr49073.c` | `unsupported_local_memory_access` | Out of scope: aggregate/select-materialized global-object and local publication rows. |
| `src/pr57861.c` | `unsupported_local_memory_access` | Reclassified: scalar global-memory facts exist, but the stop is mixed ordinary local/pointer-global traffic, including `bir.store_local %lv.l, ptr @f` and `%t3 = bir.load_global ptr @g`; no direct-global local-memory consumer row remains proven. |
| `src/pr58431.c` | `unsupported_local_memory_access` | Reclassified: scalar global-memory facts exist, but the local-memory stop is mixed local/pointer traffic such as `bir.store_local %lv.p, ptr @h`; no direct `addr @symbol` local-memory row remains proven. |
| `src/pr58984.c` | `unsupported_local_memory_access` | Reclassified: pointer-loaded-from-global plus byval/local copies; dump includes `%t8 = bir.load_global ptr @c`, not direct local memory at `@c`. |
| `src/pr60017.c` | `unsupported_local_memory_access` | Out of scope: aggregate global-object materialization into locals. |
| `src/pr60822.c` | `unsupported_local_memory_access` | Out of scope: aggregate/byte-storage global-object rows with very large offsets. |
| `src/pr66556.c` | `unsupported_local_memory_access` | Out of scope: aggregate/pointer-loaded-from-global mix, including `%t0 = bir.load_global ptr @k`. |
| `src/pr68185.c` | `unsupported_local_memory_access` | Reclassified: scalar global-memory facts exist, but no direct `addr @symbol` local-memory row remains proven; residual is mixed local/global publication. |
| `src/pr68321.c` | `unsupported_local_memory_access` | Reclassified: scalar global-memory facts plus one `layout_authority=unknown` aggregate lane; residual belongs to mixed local/global publication or aggregate owner, not this direct-local consumer. |
| `src/pr70005.c` | `unsupported_local_memory_access` | Reclassified: scalar global-memory facts exist, but residual local stores are publication/freshness-shaped; no direct `addr @symbol` local-memory row remains proven. |
| `src/pr88739.c` | `unsupported_local_memory_access` | Out of scope: aggregate/bitfield global-object and local aggregate traffic. |
| `src/struct-ret-1.c` | `unsupported_local_memory_access` | Out of scope: byval/struct-return aggregate local copies. |
| `src/pr79737-2.c` | `RV64_BACKEND_RUNTIME_MISMATCH` | Out of scope for this idea: already moved past object compile; runtime mismatch remains a separate owner. |

## Suggested Next

Plan-owner lifecycle packet: close idea 631 as exhausted/complete for the
direct `PreparedAddressBaseKind::GlobalSymbol` local-memory consumer policy,
then split any durable residuals that should continue outside this idea.

## Watchouts

The Step 5 evidence separates prepared global-memory accesses whose metadata
says `base=global_symbol` from local-memory instructions whose address operand
is literally `addr @symbol`. The focused rows still contain many scalar
`base=global_symbol` global accesses, but those are already global-memory
consumer facts, not the direct-global local-memory shape admitted in Step 4.
Do not widen idea 631 to pointer-loaded-from-global local memory, aggregate
homes, byval copies, byte-storage aggregate global objects, move bundles, large
aggregate offsets, runtime mismatches, or value-location rows owned by idea
621.

## Proof

Ran exactly:
`cmake --build --preset default && ALLOWLIST=build/agent_state/631_step1_global_symbol.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/631_step5_global_symbol.log 2>&1`

Result: build succeeded and the focused probe returned nonzero because the
allowlisted rows still fail as expected for classification evidence:
`total=18 passed=0 failed=18`.

Additional evidence artifacts:
- `build/agent_state/631_step5_global_symbol.log`
- `build/agent_state/631_step5_global_symbol_access_extract.tsv`
- `build/agent_state/631_step5_*.prepared.txt`
- `build/agent_state/631_step5_*.prepared.err`
