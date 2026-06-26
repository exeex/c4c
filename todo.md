Status: Active
Source Idea Path: ideas/open/396_rv64_object_route_terminator_fragment_lowering_refresh.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Classify Current Terminator Fragment Rejections

# Current Packet

## Just Finished

Step 1 refreshed the current 396 terminator-fragment seed bucket. The exact
delegated proof completed successfully, and none of the selected seeds
currently reproduce `unsupported_terminator_fragment`.

Representative statuses:

- `20020206-2.c`: `c4cll_status=0`, `prepared_status=0`; no failure
  diagnostic. Prepared BIR exercises `bir.cond_br`, `bir.br`, loop branches,
  returns, fused compare `branch_condition` facts, and phi/select
  `join_transfer` plus `parallel_copy` facts.
- `20000412-1.c`: `c4cll_status=0`, `prepared_status=0`; no failure
  diagnostic. Prepared BIR includes `bir.cond_br i32 %t3, block_2, block_3`,
  `bir.br block_3`, return blocks, and a fused pointer-compare branch
  condition.
- `20020129-1.c`: `c4cll_status=0`, `prepared_status=0`; no failure
  diagnostic. Prepared BIR includes multiple conditional branches,
  unconditional loop/block branches, returns, and fused pointer/integer compare
  `branch_condition` facts.
- `20030403-1.c`: `c4cll_status=0`, `prepared_status=0`; no failure
  diagnostic. Prepared BIR includes `bir.cond_br i32 %t3, block_1, block_2`,
  `bir.br block_2`, a return block, and a fused `ugt i64` branch condition.
- `20060910-1.c`: `c4cll_status=0`, `prepared_status=0`; no failure
  diagnostic. Prepared BIR includes loop branches, ternary branches, returns,
  fused compare `branch_condition` facts, and ternary `join_transfer` /
  `parallel_copy` facts.
- `20050125-1.c`: `c4cll_status=0`, `prepared_status=0`; no failure
  diagnostic. Prepared BIR includes logical short-circuit conditional
  branches, unconditional branch joins, returns, fused compare branch facts,
  and multiple select-materialization join/parallel-copy facts.
- `20100209-1.c`: `c4cll_status=0`, `prepared_status=0`; no failure
  diagnostic. Prepared BIR includes logical conditional branches,
  unconditional branches, returns, fused compare branch facts, and
  select-materialization join/parallel-copy facts.

Classification:

- No current seed fails with `unsupported_terminator_fragment`; the searched
  `unsupported`, `error`, and `fatal` matches are absent from stderr/prepared
  stderr. The only `error` text in the proof grep is the generated declaration
  name `ferror` / `ferror_unlocked` / `perror` in `20050125-1.out`, not a
  diagnostic.
- The refreshed bucket does not expose an owned 396 implementation family for
  Step 2. Conditional branch, unconditional branch, return, loop branch, and
  short-circuit/ternary join-transfer shapes are present in prepared dumps and
  all compile through RV64 object routing for this seed set.
- No missing producer CFG facts surfaced. There is no evidence here for
  reconstructing CFG semantics in RV64 object emission.

## Suggested Next

Route 396 to lifecycle review: this refreshed representative set is stale as a
current failure bucket, so there is no concrete Step 2 lowering family to
implement from these artifacts. If broader RV64 torture validation finds a new
`unsupported_terminator_fragment`, classify that fresh representative before
starting implementation.

## Watchouts

- Do not drop or weaken terminators to make object compilation succeed.
- Do not reconstruct missing BIR CFG semantics inside RV64 object emission.
- Route producer CFG fact gaps to their owners instead of patching them under
  396.
- Do not name a Step 2 lowering target from these stale seeds; the prepared
  terminator families observed here are passing shapes.

## Proof

Exact command run:

```sh
cmake --build --preset default && mkdir -p build/agent_state/396_step1_terminator_refresh && for case in 20020206-2 20000412-1 20020129-1 20030403-1 20060910-1 20050125-1 20100209-1; do src="tests/c/external/gcc_torture/src/${case}.c"; (build/c4cll --target riscv64-linux-gnu "$src" > "build/agent_state/396_step1_terminator_refresh/${case}.out" 2> "build/agent_state/396_step1_terminator_refresh/${case}.err"; printf 'c4cll_status=%s\n' "$?" > "build/agent_state/396_step1_terminator_refresh/${case}.status"); (build/c4cll --dump-prepared-bir --target riscv64-linux-gnu "$src" > "build/agent_state/396_step1_terminator_refresh/${case}.prepared.txt" 2> "build/agent_state/396_step1_terminator_refresh/${case}.prepared.err"; printf 'prepared_status=%s\n' "$?" > "build/agent_state/396_step1_terminator_refresh/${case}.prepared.status"); done && for case in 20020206-2 20000412-1 20020129-1 20030403-1 20060910-1 20050125-1 20100209-1; do printf '== %s ==\n' "$case"; cat "build/agent_state/396_step1_terminator_refresh/${case}.status" "build/agent_state/396_step1_terminator_refresh/${case}.prepared.status"; rg -n 'unsupported_terminator_fragment|unsupported|error|fatal|terminator|bir\.(return|br|cond_br|jump|switch)|return |branch|successor|label|fallthrough|goto|if ' "build/agent_state/396_step1_terminator_refresh/${case}.out" "build/agent_state/396_step1_terminator_refresh/${case}.err" "build/agent_state/396_step1_terminator_refresh/${case}.prepared.txt" "build/agent_state/396_step1_terminator_refresh/${case}.prepared.err" || true; done && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result:

- Command completed successfully.
- `cmake --build --preset default`: no work to do.
- All seven `c4cll_status` files contain `0`.
- All seven `prepared_status` files contain `0`.
- `test_after.log`: `100% tests passed, 0 tests failed out of 326`.
