Status: Active
Source Idea Path: ideas/open/633_aggregate_stack_home_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Aggregate Stack-Home Evidence

# Current Packet

## Just Finished

Step 1 refreshed the aggregate/sret/byval/pointer stack-home residual evidence
from `build/agent_state/614_step3_residual_refresh/local_memory_candidates.allowlist`.
The copied probe list is
`build/agent_state/633_step1_aggregate_stack_home.allowlist`; the focused run
covered all 35 rows and wrote
`build/agent_state/633_step1_aggregate_stack_home.log`.

Current probe result:
- total rows inspected: 35
- passed now: `src/20010123-1.c`, `src/20030920-1.c`, `src/pr35800.c`
- still failing: 32
- first failing allowlist row: `src/20000722-1.c`, current diagnostic
  `unsupported_local_memory_access`, but its prepared accesses are
  register-pointer plus string-constant adjacent, so it is not the aggregate
  stack-home owner for this idea.

Fresh Step 1 extracts:
- `build/agent_state/633_step1_current_diagnostics.tsv`
- `build/agent_state/633_step1_prepared_access_summary.tsv`
- `build/agent_state/633_step1_pointer_access_home_summary.tsv`
- `build/agent_state/633_step1_stack_home_rows.tsv`
- representative prepared dumps:
  `build/agent_state/633_step1_20020215-1.prepared.txt`,
  `build/agent_state/633_step1_pr30185.prepared.txt`,
  `build/agent_state/633_step1_pr38969.prepared.txt`,
  `build/agent_state/633_step1_struct-ret-1.prepared.txt`, plus adjacent
  stack-home/F128 mixed representatives under the same prefix.

In-scope stack-home rows with visible stack-slot pointer homes:
- `src/20020215-1.c`: `sret(size=24, align=8) %ret.sret` and
  `byval(size=24, align=8) %p.s`; visible stack homes `%ret.sret` and `%p.s`;
  offsets `0,2,3,4,5,6,7,8,16,18,19,20,21,22,23`; access sizes `1,2,8`;
  current diagnostic `unsupported_local_memory_access: ... requires prepared
  frame-slot or pointer-value base-plus-offset local memory addressing`.
- `src/921117-1.c`: `byval(size=16, align=4) %p.p`; visible stack home
  `%p.p`; offsets `0..12`; access sizes `1,4`; same local-memory diagnostic.
- `src/950628-1.c`: `sret` stack home `%ret.sret`; offsets `0..4`; access
  sizes `1,2`; same local-memory diagnostic.
- `src/pr30185.c`: `sret(size=16, align=8) %ret.sret` plus byval stack homes
  `%p.x` and `%p.y`; offsets `0..8`; access sizes `1,8`; same local-memory
  diagnostic.
- `src/pr38969.c`: byval stack home `%p.x` and return home `%ret.sret`;
  offsets `0,4`; access size `4`; same local-memory diagnostic.
- `src/pr52129.c`: byval stack home `%p.s`; offsets `0,8,12,13,14,15`;
  access sizes `1,4,8`; same local-memory diagnostic.
- `src/pr60017.c`: return home `%ret.sret`; offsets `0,4,5,6,7,8,10,12,14`;
  access sizes `1,2,4`; same local-memory diagnostic.

Mixed or out-of-scope stack-home-adjacent rows:
- `src/20010605-2.c`: stack home `%p.x`, but first rejection is unsupported
  16-byte local-memory width; route to F128/16-byte width policy before this
  stack-home consumer.
- `src/complex-7.c`: stack homes `%p.a1` through `%p.a5`, but the row mixes
  4/8-byte accesses with 16-byte accesses and global-symbol traffic; keep as a
  mixed aggregate/global/16-byte residual until Step 2 proves a clean shared
  carrier.
- `src/pr58984.c`: stack home `%p.p` plus register pointer home `%t14` and
  global-symbol accesses; classify as mixed local/global publication until a
  carrier trace separates the stack-home accesses.
- `src/struct-ret-1.c`: byval homes `%p.a`, `%p.d` and `sret` home
  `%ret.sret`, but also heavy global aggregate-object traffic; useful as a
  later negative/mixed representative, not the first clean packet.

Other inspected rows classified out of Step 2 scope:
- Move-bundle fan-in or stack-destination authority: `src/20011109-2.c`,
  `src/20021204-1.c`, `src/920429-1.c`, `src/930429-1.c`,
  `src/pr34415.c`, `src/ptr-arith-1.c`.
- 16-byte/F128 width: `src/20040208-1.c`, `src/ieee/inf-1.c`.
- Large-offset/register pointer homes: `src/ipa-sra-2.c`, `src/pr60822.c`.
- String/global/direct-global/pointer-loaded-from-global/global-object rows:
  `src/20000722-1.c`, `src/20010123-1.c`, `src/20030920-1.c`,
  `src/20021204-1.c`, `src/920429-1.c`, `src/pr35800.c`,
  `src/pr46309.c`, `src/pr49073.c`, `src/pr57861.c`, `src/pr58431.c`,
  `src/pr66556.c`, `src/pr68185.c`, `src/pr68321.c`,
  `src/pr70005.c`, `src/pr88739.c`.
- `src/941110-1.c` remains a plain frame-slot-looking local-memory rejection
  with no stack-home pointer row in the refreshed pointer-home summary; do not
  use it as aggregate stack-home evidence without a separate trace.

First missing/rejecting boundary for the clean in-scope family:
- The prepared text already exposes source value names, destination homes,
  stack-slot homes, offsets, and access sizes through aggregate/byval/sret copy
  instructions such as `addr %p.s+8` and `addr %ret.sret+8`.
- The current RV64 diagnostic still rejects these as not being prepared
  frame-slot or pointer-value base-plus-offset local memory.
- Step 1 did not find an explicit memory-use authority fact tying those
  byval/sret stack-home fields to a consumable prepared local-memory authority.
  That is the producer/carrier boundary to trace next.

## Suggested Next

Step 2 executor packet: trace the stack-home authority carriers for the clean
byval/sret stack-slot family, using `src/20020215-1.c` as the first
representative and cross-checking `src/pr30185.c` and `src/pr38969.c`.
Find the producer functions and carrier fields that should publish source
value, destination home, stack slot, selected offset, size, ABI role, and
memory-use authority before RV64 object emission.

## Watchouts

Do not treat the first failing row (`src/20000722-1.c`) as the Step 2 target;
it is string/register-pointer adjacent rather than aggregate stack-home.
Keep 16-byte/F128 rows, global-object rows, pointer-loaded-from-global rows,
move-bundle fan-in rows, large-offset pointer rows, and plain frame-slot rows
outside the first stack-home carrier trace unless fresh evidence shows the
same explicit authority shape.

## Proof

Ran the delegated evidence command:

```sh
cmake --build --preset default && ALLOWLIST=build/agent_state/614_step3_residual_refresh/local_memory_candidates.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/633_step1_aggregate_stack_home.log 2>&1
```

The build succeeded; the focused residual probe returned nonzero because 32 of
35 rows still fail by design for this evidence refresh. No new root-level
`.log` file was created.
