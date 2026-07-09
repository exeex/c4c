Status: Active
Source Idea Path: ideas/open/631_direct_global_symbol_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Direct Global-Symbol Evidence

# Current Packet

## Just Finished

Step 1 refreshed the direct `global_symbol` local-memory evidence for the 18-row
focused allowlist in `build/agent_state/631_step1_global_symbol.allowlist`.
Current proof result: total=18, passed=0, failed=18. Fresh prepared dumps are in
`build/agent_state/631_step1_*.prepared.txt`; compact row facts are in
`build/agent_state/631_step1_global_symbol_access_extract.tsv`.

Inspected rows and current owners:
- `src/20021204-1.c`: 1 scalar `global_symbol` access (`z`, offset 0, size 4)
  exists, but the first current owner is `unsupported_prepared_move_bundle_classification`.
- `src/920429-1.c`: 2 scalar `global_symbol` accesses (`i,j`, offset 0,
  size 4) exist, but the first current owner is `unsupported_prepared_move_bundle_classification`.
- `src/921117-1.c`: aggregate global object direct memory (`cell`, offsets
  0..12, sizes 1/4, `byte_storage_aggregate` plus unknown layout), current
  diagnostic `unsupported_local_memory_access`.
- `src/complex-7.c`: aggregate global object direct memory with 16-byte lanes
  (`f*`, `d*`, `ld*`, offsets 0/4/8/16, sizes 4/8/16,
  `byte_storage_aggregate`), current diagnostic `unsupported_local_memory_access`.
- `src/pr46309.c`: true scalar direct `global_symbol` local-memory candidate:
  `symbol=q offset=0 size=8 layout_authority=scalar_layout range=proven_in_bounds
  base_plus_offset=yes`; current diagnostic `unsupported_local_memory_access`.
- `src/pr49073.c`: mixed aggregate/scalar global object direct memory
  (`a,c`, offsets 0..24, size 4, `byte_storage_aggregate` plus
  `scalar_layout`), current diagnostic `unsupported_local_memory_access`.
- `src/pr57861.c`: true scalar direct `global_symbol` local-memory candidate:
  symbols `a,b,c,d,e,f,g,h,i,j`, offset 0, sizes 2/4/8,
  `scalar_layout`, proven in bounds, base-plus-offset; current diagnostic
  `unsupported_local_memory_access`.
- `src/pr58431.c`: true scalar direct `global_symbol` local-memory candidate:
  symbols `a,b,c,d,e,g,h,i,j,k`, offset 0, sizes 1/2/4,
  `scalar_layout`, proven in bounds, base-plus-offset; current diagnostic
  `unsupported_local_memory_access`.
- `src/pr58984.c`: true scalar direct `global_symbol` local-memory candidate:
  symbols `a,b,c,e,m,n`, offset 0, sizes 4/8, `scalar_layout`, proven in
  bounds, base-plus-offset; current diagnostic `unsupported_local_memory_access`.
- `src/pr60017.c`: aggregate global object direct memory (`x`, offsets
  0/4/5/6/7/8/10/12/14, sizes 1/2/4, `byte_storage_aggregate` plus unknown
  layout), current diagnostic `unsupported_local_memory_access`.
- `src/pr60822.c`: large-offset aggregate global object direct memory (`x`,
  offsets 800000 and 1700004, size 4, `byte_storage_aggregate`), current
  diagnostic `unsupported_local_memory_access`; route owner should include
  large selected offset/materialization, not Step 2 scalar carrier tracing.
- `src/pr66556.c`: mixed aggregate/scalar global object direct memory
  (`a,b,c,d,e,f,g,h,j,k,l`, offsets 0/4, sizes 1/2/4/8,
  `byte_storage_aggregate` plus `scalar_layout`), current diagnostic
  `unsupported_local_memory_access`.
- `src/pr68185.c`: true scalar direct `global_symbol` local-memory candidate:
  symbols `a,b,c,d,e,f,o,q,t,u,w,z`, offset 0, sizes 2/4,
  `scalar_layout`, proven in bounds, base-plus-offset; current diagnostic
  `unsupported_local_memory_access`.
- `src/pr68321.c`: true direct `global_symbol` local-memory candidate with
  scalar/unknown layout: symbols `a,b,e,m,n,t,t2,t5,u`, offset 0, sizes 1/4,
  proven in bounds, base-plus-offset; current diagnostic `unsupported_local_memory_access`.
- `src/pr70005.c`: true scalar direct `global_symbol` local-memory candidate:
  symbols `a,b,c`, offset 0, sizes 1/4, `scalar_layout`, proven in bounds,
  base-plus-offset; current diagnostic `unsupported_local_memory_access`.
- `src/pr88739.c`: mixed aggregate/scalar global object direct memory
  (`__static_local_bar_1,v`, offsets 0/12/14, sizes 2/4,
  `byte_storage_aggregate` plus `scalar_layout`), current diagnostic
  `unsupported_local_memory_access`.
- `src/struct-ret-1.c`: aggregate-home/struct-return owner candidate
  (`B1,B2,__static_local_f_23,c2,d3,fp`, offsets 0..33, sizes 1/4/8,
  aggregate/scalar/unknown layout), current diagnostic `unsupported_local_memory_access`.
- `src/pr79737-2.c`: direct global base-plus-offset facts are present
  (`i,j`, offsets 0/4/8, size 4, `byte_storage_aggregate`, proven in bounds),
  but the current proof reaches `RV64_BACKEND_RUNTIME_MISMATCH`, so this row is
  not the Step 2 local-memory rejection carrier.

No refreshed row is a prepared global value-location/idea-621 candidate. No
row's first current owner is string-constant policy. `address_space` and
relocation/addressing-mode fields are not printed in these prepared access
rows; the visible addressing fact is `base_plus_offset=yes`.

## Suggested Next

Step 2 should trace the carrier path for `src/pr46309.c` only: two scalar
direct `global_symbol` local-memory accesses, `symbol=q offset=0 size=8
layout_authority=scalar_layout range=proven_in_bounds base_plus_offset=yes`,
with current first owner `unsupported_local_memory_access`. The packet should
identify where global identity, symbol, offset, width, extent/layout authority,
and local-memory use authority are produced or lost before RV64 object
emission.

## Watchouts

Do not use `src/pr79737-2.c` as the first Step 2 carrier despite its docs-bucket
role; the current allowlisted proof has moved it to runtime mismatch. Keep
aggregate byte-storage rows, `src/pr60822.c` large offsets, 16-byte lanes in
`src/complex-7.c`, and `src/struct-ret-1.c` aggregate-home/struct-return
evidence out of the scalar Step 2 trace. Do not infer missing address-space or
relocation facts from final symbols or assembly.

## Proof

Ran:

`cmake --build --preset default && ALLOWLIST=build/agent_state/631_step1_global_symbol.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/631_step1_global_symbol.log 2>&1`

Result: command exited 1 because all 18 focused rows still fail in the current
backend scan; log path is `build/agent_state/631_step1_global_symbol.log`.
The delegated proof intentionally used the packet-local log path and did not
create a root-level `test_after.log`.
