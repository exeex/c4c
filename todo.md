Status: Active
Source Idea Path: ideas/open/633_aggregate_stack_home_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 9
Current Step Title: Trace Remaining Stack-Home Local-Memory Bucket

# Current Packet

## Just Finished

Step 8 reran the aggregate stack-home residual probe after the Step 7 RV64
consumer repair and recorded fresh evidence under
`build/agent_state/633_step8_*`.

Probe result:
- Command returned nonzero for residual failures after a clean build.
- Count changed by owner, but not by aggregate total:
  Step 5 was `total=35 passed=3 failed=32`; Step 8 is
  `total=35 passed=3 failed=32` in the script summary.
- A fresh copied allowlist was recorded at
  `build/agent_state/633_step8_aggregate_stack_home.allowlist`.
- A fresh diagnostic table was recorded at
  `build/agent_state/633_step8_current_diagnostics.tsv`.

Representative movement:
- `src/20000722-1.c` remains a runtime mismatch row rather than a stack-home
  local-memory compile rejection.
- `src/pr38969.c` moved past the stack-home local-memory gate and now stops at
  `unsupported_call_abi` for the ordinary same-module call from `bar` to
  `foo`.
- `src/struct-ret-1.c` moved past the local-memory gate and now stops at an
  unsupported `CallInst` instruction fragment.
- `src/921117-1.c`, `src/pr58984.c`, `src/pr52129.c`, and `src/complex-7.c`
  also moved from the Step 5 generic local-memory owner to later call ABI,
  branch stack-load authority, or aggregate global-data owners.
- `src/pr30185.c`, `src/950628-1.c`, and `src/20020215-1.c` still stop at
  `unsupported_local_memory_access: RV64 object route requires prepared
  frame-slot or pointer-value base-plus-offset local memory addressing`.
  Fresh extracts show byval/sret pointer-value accesses with
  `base_plus_offset=yes` and proven ranges still exist in these rows.

Residual buckets from Step 8:
- Passing rows: `src/20010123-1.c`, `src/20030920-1.c`,
  `src/pr35800.c`.
- Later non-stack-home owners: call ABI/result lowering
  (`src/921117-1.c`, `src/pr38969.c`, `src/pr58984.c`), call instruction
  fragment (`src/struct-ret-1.c`), branch stack-load authority
  (`src/pr52129.c`), aggregate global data width (`src/complex-7.c`),
  move-bundle authority (`src/20011109-2.c`, `src/20021204-1.c`,
  `src/920429-1.c`, `src/930429-1.c`, `src/pr34415.c`,
  `src/ptr-arith-1.c`), and F128/16-byte local-memory width
  (`src/20010605-2.c`, `src/20040208-1.c`, `src/ieee/inf-1.c`), plus runtime
  mismatch (`src/20000722-1.c`).
- Remaining local-memory bucket: `src/20020215-1.c`, `src/941110-1.c`,
  `src/950628-1.c`, `src/ipa-sra-2.c`, `src/pr30185.c`, `src/pr46309.c`,
  `src/pr49073.c`, `src/pr57861.c`, `src/pr58431.c`, `src/pr60017.c`,
  `src/pr60822.c`, `src/pr66556.c`, `src/pr68185.c`, `src/pr68321.c`,
  `src/pr70005.c`, `src/pr88739.c`.

## Suggested Next

Delegate Step 9 as a narrow trace packet for the remaining local-memory
bucket. Primary targets are `src/pr30185.c`, `src/950628-1.c`, and
`src/20020215-1.c`, starting from the fresh Step 8 prepared extracts and
object-route diagnostics. The packet should compare those rows against the
now-moved `src/pr38969.c` route, identify the first diverging prepared fact or
RV64 dispatch boundary, and classify the other remaining local-memory rows
only enough to decide whether they share the same stack-home route.

## Watchouts

Do not close idea 633 yet: the clean `pr38969` floating route moved, but the
integer byte-sliced and wider byval/sret families still contain in-scope
stack-home local-memory rows.

Keep later call ABI, call-instruction, branch stack-load, move-bundle,
aggregate global-data, F128/16-byte, runtime, and mixed local/global rows out
of the next stack-home packet.

## Proof

Evidence command:
`cmake --build --preset default && ALLOWLIST=build/agent_state/614_step3_residual_refresh/local_memory_candidates.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/633_step8_aggregate_stack_home.log 2>&1`

Result: build succeeded; residual probe returned nonzero for classification
with `total=35 passed=3 failed=32`.

Extra check: `git diff --check` passed.

Evidence logs:
- `build/agent_state/633_step8_aggregate_stack_home.log`
- `build/agent_state/633_step8_current_diagnostics.tsv`
- `build/agent_state/633_step8_*.prepared.txt`
- `build/agent_state/633_step8_*.extract.txt`

No new root-level `.log` file was created.
