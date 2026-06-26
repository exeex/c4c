Status: Active
Source Idea Path: ideas/open/397_rv64_object_route_move_bundle_target_shapes.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Classify Current Move Bundle Rejections

# Current Packet

## Just Finished

Step 1 refreshed the current 397 move-bundle seed bucket. The exact delegated
proof completed successfully, and none of the selected representatives
currently reproduce `unsupported_move_bundle_target_shape`, the select
publication unsupported-move diagnostic, or
`prepared_consumer_category=missing_move_bundle`.

Representative statuses:

- `20080519-1.c`: `c4cll_status=0`, `prepared_status=0`; no failure
  diagnostic. Prepared BIR includes select materialization, authoritative
  `out_of_ssa_parallel_copy` move bundles, register destinations, stack
  destinations, and consumer register/stack crossing moves.
- `20010604-1.c`: `c4cll_status=0`, `prepared_status=0`; no failure
  diagnostic. Prepared BIR includes select/logic `join_transfer` facts,
  stack-slot phi joins, stack-to-stack moves, stack-to-register joins,
  register-to-stack moves, call result moves, and return stack-to-register
  moves.
- `930123-1.c`: `c4cll_status=0`, `prepared_status=0`; no failure
  diagnostic. Prepared BIR includes ternary select materialization,
  before/after-call move bundles, register-to-register, register-to-stack,
  stack-to-stack, and stack-to-register consumer moves. The prior
  missing-authority representative no longer reports
  `prepared_consumer_category=missing_move_bundle`.
- `20000808-1.c`: `c4cll_status=0`, `prepared_status=0`; no failure
  diagnostic. Prepared BIR includes many select-materialization joins,
  register move bundles, stack/register consumer moves, stack-slot
  preservation, and call argument stack/register routes.
- `20020510-1.c`: `c4cll_status=0`, `prepared_status=0`; no failure
  diagnostic. Prepared BIR includes register phi joins, immediate phi joins,
  edge consumer preservation, consumer stack/register crossings, and call
  argument register/stack routes.
- `20040309-1.c`: `c4cll_status=0`, `prepared_status=0`; no failure
  diagnostic. Prepared BIR includes ternary select-materialization
  `out_of_ssa_parallel_copy` bundles, consumer stack-to-register and
  register-to-register moves, call result moves, and return register moves.
- `20000503-1.c`: `c4cll_status=0`, `prepared_status=0`; no failure
  diagnostic. Prepared BIR includes ternary select-materialization moves,
  block-entry publications, edge consumer preservation, call result moves, and
  return register moves.
- `20000801-2.c`: `c4cll_status=0`, `prepared_status=0`; no failure
  diagnostic. Prepared BIR includes select-chain materialization,
  register-based phi moves, before/after-call move bundles, call argument/result
  moves, and return register moves.

Classification:

- No current seed fails with `unsupported_move_bundle_target_shape`.
- No current seed fails with `prepared select publication move bundle requires
  unsupported RV64 moves`.
- No current seed reports `prepared_consumer_category=missing_move_bundle` or
  `missing_move_bundle`.
- The refreshed seed set exercises register moves, stack moves,
  stack/register-crossing moves, call argument/result moves, return moves,
  select-materialization phi moves, and edge-preservation moves, but all are
  passing shapes in the current tree.
- Because the current artifacts do not expose a failing move family or a
  missing-authority producer gap, there is no justified 397 Step 2
  implementation target from this seed set.

## Suggested Next

Route 397 to lifecycle review: this refreshed representative set is stale as a
current failure bucket, so no concrete Step 2 move-lowering or producer
missing-authority packet should be started from these artifacts. If broader
RV64 torture validation finds a new move-bundle diagnostic, classify that fresh
representative first and route producer missing-authority separately from RV64
target move lowering.

## Watchouts

- Do not drop copies or publications to clear the diagnostic.
- Do not fabricate missing move-bundle authority in MIR/RV64 object emission.
- Route producer-side missing facts to lifecycle review instead of absorbing
  them into target lowering.
- Do not use filename-specific branches, expectation rewrites, or allowlist
  filtering as progress.
- Do not name a Step 2 family from these stale seeds; the observed move families
  are current passing coverage, not current failures.

## Proof

Exact command run:

```sh
cmake --build --preset default && mkdir -p build/agent_state/397_step1_move_refresh && for case in 20080519-1 20010604-1 930123-1 20000808-1 20020510-1 20040309-1 20000503-1 20000801-2; do src="tests/c/external/gcc_torture/src/${case}.c"; (build/c4cll --target riscv64-linux-gnu "$src" > "build/agent_state/397_step1_move_refresh/${case}.out" 2> "build/agent_state/397_step1_move_refresh/${case}.err"; printf 'c4cll_status=%s\n' "$?" > "build/agent_state/397_step1_move_refresh/${case}.status"); (build/c4cll --dump-prepared-bir --target riscv64-linux-gnu "$src" > "build/agent_state/397_step1_move_refresh/${case}.prepared.txt" 2> "build/agent_state/397_step1_move_refresh/${case}.prepared.err"; printf 'prepared_status=%s\n' "$?" > "build/agent_state/397_step1_move_refresh/${case}.prepared.status"); done && for case in 20080519-1 20010604-1 930123-1 20000808-1 20020510-1 20040309-1 20000503-1 20000801-2; do printf '== %s ==\n' "$case"; cat "build/agent_state/397_step1_move_refresh/${case}.status" "build/agent_state/397_step1_move_refresh/${case}.prepared.status"; rg -n 'unsupported_move_bundle_target_shape|prepared select publication move bundle requires unsupported RV64 moves|prepared_consumer_category=missing_move_bundle|missing_move_bundle|move bundle|move_bundle|parallel_copy|join_transfer|select|publication|source_|dest_|placement|value_bank|bank=none|bank=gpr|stack_slot|frame_slot|register|unsupported|error|fatal' "build/agent_state/397_step1_move_refresh/${case}.out" "build/agent_state/397_step1_move_refresh/${case}.err" "build/agent_state/397_step1_move_refresh/${case}.prepared.txt" "build/agent_state/397_step1_move_refresh/${case}.prepared.err" || true; done && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result:

- Command completed successfully.
- `cmake --build --preset default`: no work to do.
- All eight `c4cll_status` files contain `0`.
- All eight `prepared_status` files contain `0`.
- Diagnostic grep over `*.out`, `*.err`, and `*.prepared.err` found no
  `unsupported_move_bundle_target_shape`, select-publication unsupported move,
  missing-move-bundle category, `unsupported`, `error`, or `fatal` diagnostics.
- `test_after.log`: `100% tests passed, 0 tests failed out of 326`.
