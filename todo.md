Status: Active
Source Idea Path: ideas/open/633_aggregate_stack_home_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Reclassify Aggregate Stack-Home Rows

# Current Packet

## Just Finished

Step 5 reran the aggregate stack-home residual probe after the Step 4 RV64
consumer gate and copied the probed allowlist to
`build/agent_state/633_step5_aggregate_stack_home.allowlist`.

Result stayed `total=35 passed=3 failed=32`. The refreshed probe did not move
any whole row past the prior Step 1 classification: `src/20010123-1.c`,
`src/20030920-1.c`, and `src/pr35800.c` still pass; the same 32 rows still
fail or mismatch.

Fresh evidence written under `build/agent_state/633_step5_*`:
- `633_step5_aggregate_stack_home.log`: exact focused residual probe output.
- `633_step5_current_diagnostics.tsv`: current status and first diagnostic for
  all 35 probed rows.
- `633_step5_*.prepared.txt` / `.err`: prepared dumps for the Step 1
  stack-home representatives and additional residual rows that still stop at
  `unsupported_local_memory_access`.

Row classification:

| Row | Step 5 result | Current owner |
| --- | --- | --- |
| `src/20000722-1.c` | fail | Runtime mismatch: object builds and runs to a segmentation fault, not a current local-memory compile rejection. |
| `src/20010123-1.c` | pass | Already clear. |
| `src/20010605-2.c` | fail | F128/16-byte prepared local-memory width; outside idea 633. |
| `src/20011109-2.c` | fail | Prepared move-bundle stack-destination fan-in authority. |
| `src/20020215-1.c` | fail | In-scope aggregate stack-home local-memory still stops at the generic prepared frame-slot or pointer-value base-plus-offset gate. |
| `src/20021204-1.c` | fail | Prepared move-bundle non-parallel register fan-in to stack destination. |
| `src/20030920-1.c` | pass | Already clear. |
| `src/20040208-1.c` | fail | F128/16-byte prepared local-memory width; outside idea 633. |
| `src/920429-1.c` | fail | Prepared move-bundle non-parallel register fan-in to stack destination. |
| `src/921117-1.c` | fail | Mixed byval stack-home plus global aggregate object materialization; first diagnostic remains generic local-memory gate. |
| `src/930429-1.c` | fail | Prepared move-bundle non-parallel register fan-in to stack destination. |
| `src/941110-1.c` | fail | Local-memory residual with no byval/sret stack-home representative evidence; split or classify outside idea 633. |
| `src/950628-1.c` | fail | In-scope sret/aggregate stack-home local-memory still stops at the generic prepared frame-slot or pointer-value base-plus-offset gate. |
| `src/complex-7.c` | fail | Byval aggregate stack-home with complex/F128-adjacent lanes; still stops at the generic local-memory gate. |
| `src/ieee/inf-1.c` | fail | F128/16-byte prepared local-memory width; outside idea 633. |
| `src/ipa-sra-2.c` | fail | Local-memory residual without a clean Step 1 stack-home authority lane; split or classify outside idea 633. |
| `src/pr30185.c` | fail | In-scope byval/sret stack-home local-memory still stops at the generic prepared frame-slot or pointer-value base-plus-offset gate. |
| `src/pr34415.c` | fail | Prepared move-bundle non-parallel register fan-in to stack destination. |
| `src/pr35800.c` | pass | Already clear. |
| `src/pr38969.c` | fail | In-scope byval/sret stack-home local-memory still stops at the generic prepared frame-slot or pointer-value base-plus-offset gate. |
| `src/pr46309.c` | fail | Pointer/global residual already split from direct global-symbol work; not a clean idea 633 stack-home lane. |
| `src/pr49073.c` | fail | Local-memory residual without a clean Step 1 stack-home authority lane; split or classify outside idea 633. |
| `src/pr52129.c` | fail | Byval aggregate stack-home residual still stops at the generic local-memory gate. |
| `src/pr57861.c` | fail | Mixed local/global publication authority and global-object traffic; not a clean idea 633 stack-home lane. |
| `src/pr58431.c` | fail | Mixed local/global publication authority and global-object traffic; not a clean idea 633 stack-home lane. |
| `src/pr58984.c` | fail | Mixed stack-home/register and aggregate copy residual; still stops at generic local-memory gate. |
| `src/pr60017.c` | fail | Mixed global aggregate loads plus sret copy; not solely idea 633 stack-home consumer work. |
| `src/pr60822.c` | fail | Local-memory residual without a clean Step 1 stack-home authority lane; split or classify outside idea 633. |
| `src/pr66556.c` | fail | Local-memory residual without a clean Step 1 stack-home authority lane; split or classify outside idea 633. |
| `src/pr68185.c` | fail | Local-memory residual without a clean Step 1 stack-home authority lane; split or classify outside idea 633. |
| `src/pr68321.c` | fail | Local-memory residual without a clean Step 1 stack-home authority lane; split or classify outside idea 633. |
| `src/pr70005.c` | fail | Local-memory residual without a clean Step 1 stack-home authority lane; split or classify outside idea 633. |
| `src/pr88739.c` | fail | Local-memory residual without a clean Step 1 stack-home authority lane; split or classify outside idea 633. |
| `src/ptr-arith-1.c` | fail | Prepared move-bundle non-parallel register fan-in to stack destination. |
| `src/struct-ret-1.c` | fail | Sret/byval aggregate stack-home local-memory still stops at the generic prepared frame-slot or pointer-value base-plus-offset gate. |

Representative prepared evidence:
- `src/pr38969.c`: prepared dump still has byval and sret objects with
  pointer-value accesses such as `pointer=%p.x offset=0/4` and sret stores
  such as `pointer=%ret.sret offset=0/4`, all with `base_plus_offset=yes` and
  proven ranges, but the full object route still rejects the row.
- `src/pr30185.c`: prepared dump still exposes byte-sliced byval pointer-value
  loads from `%p.x` and `%p.y` plus frame-slot stores with proven ranges, but
  the row still rejects before object emission completes.
- `src/950628-1.c`: prepared dump exposes sret pointer-value stores from
  frame-slot aggregate copies, but the row still rejects at the same generic
  local-memory diagnostic.

## Suggested Next

Continue idea 633 with a narrow follow-up packet for the remaining aggregate
copy stack-home lane. Start from `src/pr38969.c` and cross-check
`src/pr30185.c` and `src/950628-1.c`; determine why prepared accesses with
`base=pointer_value`, byval/sret source objects, `base_plus_offset=yes`, and
`range_verdict=proven_in_bounds` still fall through to the generic
`unsupported_local_memory_access` diagnostic instead of the Step 4
`prepared_stack_home_local_memory_has_authority(...)` consumer.

## Watchouts

Idea 633 is not close-ready: the clean byval/sret representative rows did not
move past the compile-time local-memory gate. This should not be treated as a
residual-only split yet.

Do not fold F128/16-byte local-memory rows, move-bundle rows, mixed
local/global publication rows, or the runtime mismatch into the next packet.
Those are separate owners or out-of-scope buckets.

## Proof

Proof command:
`cmake --build --preset default && ALLOWLIST=build/agent_state/614_step3_residual_refresh/local_memory_candidates.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/633_step5_aggregate_stack_home.log 2>&1`

Result: build succeeded; focused probe returned nonzero for residual
classification, `total=35 passed=3 failed=32`.

No root-level `.log` file was created by this packet.
