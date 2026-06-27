Status: Active
Source Idea Path: ideas/open/411_rv64_object_route_rhs_bitfield_boolean_select_abort.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Repair The First Supported Runtime Owner

# Current Packet

## Just Finished

Step 2 repaired the first supported 411 runtime owner. RV64 object emission now
prepends supported same-block binary producer dependencies before materializing a
published select/join edge-copy source compare. This covers the live shape where
`%t44` depends on `%t43`, and `%t43` is produced by a bitfield chain before the
select carrier publishes `%t44 -> %t48` on the RHS predecessor edge.

Changed files:

- `src/backend/mir/riscv/codegen/object_emission.cpp`: added a fail-closed
  dependency emitter for published select edge binary source producers. It walks
  same-block binary producers before the edge compare, emits them in dependency
  order through the existing `fragment_for_prepared_binary()` lowering, then
  emits the compare into the select destination register.
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`: added focused
  coverage for a dependent edge compare source chain (`lshr`, `and`, `add`,
  then `ne`) and asserts the predecessor edge materializes that chain before the
  compare.
- `todo.md`: recorded this Step 2 proof and outcome.

Representative outcome: `src/20000113-1.c` now passes the runner and qemu
comparison (`clang_qemu_status=0`, `c4c_qemu_status=0`), so the old
`SIGIOT`/abort residual is cleared for this packet. The 402 guard
`src/20070212-2.c` also remains green (`clang_qemu_status=0`,
`c4c_qemu_status=0`). Runner summary: total=2, passed=2, failed=0.

## Suggested Next

Execute Step 3: rerun the representative proof/classification for active 411 and
record closure readiness or any fresh residual owner. Include `20000113-1.c` as
the primary proof and keep `20070212-2.c` as the regression guard if delegated.

## Watchouts

- The repair is intentionally target-side scheduling for already-published
  select/join facts; do not add producer-side testcase handling for this shape.
- The dependency emitter currently supports same-block binary producer chains and
  fails closed if a needed dependency cannot lower through existing binary
  object emission.
- The join block may still emit the producer chain on other paths; this packet
  only required the predecessor edge to have a correct value before publishing
  the select edge copy.
- Do not weaken qemu comparison, expected exits, allowlists, or diagnostics.

## Proof

Ran the exact delegated Step 2 proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$' && mkdir -p build/agent_state/411_step2_rhs_select_runtime && printf 'src/20000113-1.c\nsrc/20070212-2.c\n' > build/agent_state/411_step2_rhs_select_runtime/representatives.allowlist && (set +e; ALLOWLIST=build/agent_state/411_step2_rhs_select_runtime/representatives.allowlist VERBOSE_FAILURES=1 CASE_TIMEOUT_SEC=20 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/411_step2_rhs_select_runtime/representatives.runner.log 2>&1; printf 'runner_status=%s\n' "$?" > build/agent_state/411_step2_rhs_select_runtime/representatives.runner.status) && for case in 20000113-1 20070212-2; do dir="build/agent_state/411_step2_rhs_select_runtime/${case}"; work="build/rv64_gcc_c_torture_backend/src_${case}.c"; src="tests/c/external/gcc_torture/src/${case}.c"; mkdir -p "$dir"; cp "$work/case.log" "$dir/case.log" 2>/dev/null || true; (build/c4cll --dump-prepared-bir --target riscv64-linux-gnu "$src" > "$dir/prepared.bir.txt" 2> "$dir/prepared.bir.err"; printf 'prepared_status=%s\n' "$?" > "$dir/prepared.bir.status"); (llvm-objdump -d "$work/c4c.bin" > "$dir/c4c.bin.objdump.txt" 2> "$dir/c4c.bin.objdump.err"; printf 'c4c_bin_objdump_status=%s\n' "$?" > "$dir/c4c_bin_objdump.status"); (qemu-riscv64 -L /usr/riscv64-linux-gnu "$work/clang.bin" > "$dir/clang.qemu.out" 2> "$dir/clang.qemu.err"; printf 'clang_qemu_status=%s\n' "$?" > "$dir/clang.qemu.status"); (qemu-riscv64 -L /usr/riscv64-linux-gnu "$work/c4c.bin" > "$dir/c4c.qemu.out" 2> "$dir/c4c.qemu.err"; printf 'c4c_qemu_status=%s\n' "$?" > "$dir/c4c.qemu.status"); cat "$dir"/*.status > "$dir/status.summary"; rg -n 'foobar|logic_rhs|logic_end|logic\.rhs|logic\.end|select_chain|join_transfer|authoritative_branch_pair|%t43|%t44|%t48|bf\.|abort|SIGIOT|Subprocess aborted|addi\s+[^,]+,\s*sp|sd|ld|lw|andi|srl|sll|mul|bne|beq|jal|jalr|mv\s+t3|mv\s+s2|lw\s+s2|address_materialization|lv\.param|lv\.f1' "$dir/case.log" "$dir/prepared.bir.txt" "$dir/prepared.bir.err" "$dir/c4c.bin.objdump.txt" "$dir/c4c.qemu.err" > "$dir/triage_hits.txt" 2>/dev/null || true; done && cat build/agent_state/411_step2_rhs_select_runtime/representatives.runner.status build/agent_state/411_step2_rhs_select_runtime/20000113-1/status.summary build/agent_state/411_step2_rhs_select_runtime/20070212-2/status.summary && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Results:

- Focused `backend_riscv_object_emission`: PASS.
- Runner: `runner_status=0`; `src/20000113-1.c` pass;
  `src/20070212-2.c` pass.
- `20000113-1`: `prepared_status=0`, `c4c_bin_objdump_status=0`,
  `clang_qemu_status=0`, `c4c_qemu_status=0`.
- `20070212-2`: `prepared_status=0`, `c4c_bin_objdump_status=0`,
  `clang_qemu_status=0`, `c4c_qemu_status=0`.
- Canonical backend proof log: `test_after.log`, PASS with 326/326 backend
  tests.
