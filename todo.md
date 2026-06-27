Status: Active
Source Idea Path: ideas/open/411_rv64_object_route_rhs_bitfield_boolean_select_abort.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Classify The RHS Bitfield Boolean Abort

# Current Packet

## Just Finished

Step 1 classified the active 411 representative
`tests/c/external/gcc_torture/src/20000113-1.c`. The compile/object pipeline is
clear, but the qemu comparison still fails at runtime: `clang_qemu_status=0`,
`c4c_qemu_status=134`, and `c4c_qemu_strace_status=134` with `tgkill(...,
SIGIOT)` followed by `SIGIOT`. The runner reports
`RV64_BACKEND_RUNTIME_MISMATCH` and `clang_exit=0 c4c_exit=Subprocess aborted`.

Producer facts are present enough to route this to RV64 object lowering rather
than producer fact routing. Semantic BIR computes the RHS bitfield chain inside
`logic.rhs.37` before joining through `%t48`: `%t43.bf.unit`, `%t43.bf.shr`,
`%t43.bf.mask`, `%t43`, `%t44 = ne %t43, 5`, then `%t48` at `logic.end.40`.
Prepared BIR rewrites the join as an authoritative select materialization:
`join_transfer logic.end.40 result=%t48 kind=phi_edge
carrier=select_materialization ownership=authoritative_branch_pair`, with
`edge_transfer logic.rhs.end.39 -> logic.end.40 incoming=%t44 destination=%t48`
and storage for `%t43/%t44/%t48` all in GPRs.

The first observable bad object fact is value scheduling for the published
select/join RHS edge. In `c4c.o.objdump`, `.Lfoobar_logic_rhs_end_39` computes
`%t44` from stale `s2` (`mv t3, s2`, compare against 5, publish `t0`) before
`.Lfoobar_logic_end_40` emits the producer chain for `%t43`
(`mv t3, s1`, `srl s2, t3, 3`, `andi s1, s2, 7`, `mv s2, s1`). The final branch
then consumes stale `t0` and reaches `abort`. This makes the first Step 2 owner
RV64 object select/join materialization and value scheduling, not branch target
placement and not producer fact routing.

## Suggested Next

Execute Step 2: repair RV64 object lowering so published select/join
edge-copy carriers materialize any owned source producer chain before the edge
copy or fused branch consumes it. Start in
`src/backend/mir/riscv/codegen/object_emission.cpp`, especially
`fragment_for_prepared_select_edge_source_producer`,
`prepared_binary_is_select_edge_owned_source`,
`prepared_select_publication_move_is_rv64_object_admitted`, and the select
handling in `fragment_for_prepared_instruction`. Add focused coverage in
`tests/backend/mir/backend_riscv_object_emission_test.cpp` near
`make_prepared_join_transfer_select_with_edge_compare_source_module()` and
`materializes_published_prepared_join_transfer_select_edge_compare_source_object()`
for a dependent source-producer chain, not a `foobar`-specific shortcut.

## Watchouts

- Do not claim compile-only proof; this representative reaches qemu.
- Current prepared facts look explicit and coherent; do not mask a target
  scheduling bug by adding producer-side testcase handling.
- Keep branch placement separate from value scheduling unless a new proof shows
  label/successor emission is wrong. The observed branch targets are present;
  the consumed value is stale.
- Do not use filename-specific handling, expectation rewrites, qemu weakening,
  unsupported downgrades, or allowlist filtering as progress.

## Proof

Ran the exact delegated Step 1 proof/classification command:

```sh
cmake --build --preset default && mkdir -p build/agent_state/411_step1_rhs_bitfield_boolean_abort && printf 'src/20000113-1.c\n' > build/agent_state/411_step1_rhs_bitfield_boolean_abort/representative.allowlist && (set +e; ALLOWLIST=build/agent_state/411_step1_rhs_bitfield_boolean_abort/representative.allowlist VERBOSE_FAILURES=1 CASE_TIMEOUT_SEC=20 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/411_step1_rhs_bitfield_boolean_abort/representative.runner.log 2>&1; printf 'runner_status=%s\n' "$?" > build/agent_state/411_step1_rhs_bitfield_boolean_abort/representative.runner.status) && dir="build/agent_state/411_step1_rhs_bitfield_boolean_abort/20000113-1" && work="build/rv64_gcc_c_torture_backend/src_20000113-1.c" && src="tests/c/external/gcc_torture/src/20000113-1.c" && mkdir -p "$dir" && cp "$work/case.log" "$dir/case.log" 2>/dev/null || true && (build/c4cll --dump-bir --target riscv64-linux-gnu "$src" > "$dir/semantic.bir.txt" 2> "$dir/semantic.bir.err"; printf 'dump_bir_status=%s\n' "$?" > "$dir/semantic.bir.status") && (build/c4cll --dump-prepared-bir --target riscv64-linux-gnu "$src" > "$dir/prepared.bir.txt" 2> "$dir/prepared.bir.err"; printf 'prepared_status=%s\n' "$?" > "$dir/prepared.bir.status") && (llvm-objdump -dr "$work/c4c.o" > "$dir/c4c.o.objdump.txt" 2> "$dir/c4c.o.objdump.err"; printf 'c4c_objdump_status=%s\n' "$?" > "$dir/c4c.o.objdump.status") && (llvm-objdump -d "$work/c4c.bin" > "$dir/c4c.bin.objdump.txt" 2> "$dir/c4c.bin.objdump.err"; printf 'c4c_bin_objdump_status=%s\n' "$?" > "$dir/c4c.bin.objdump.status") && (llvm-objdump -d "$work/clang.bin" > "$dir/clang.bin.objdump.txt" 2> "$dir/clang.bin.objdump.err"; printf 'clang_bin_objdump_status=%s\n' "$?" > "$dir/clang.bin.objdump.status") && (qemu-riscv64 -L /usr/riscv64-linux-gnu "$work/clang.bin" > "$dir/clang.qemu.out" 2> "$dir/clang.qemu.err"; printf 'clang_qemu_status=%s\n' "$?" > "$dir/clang.qemu.status") && (qemu-riscv64 -L /usr/riscv64-linux-gnu "$work/c4c.bin" > "$dir/c4c.qemu.out" 2> "$dir/c4c.qemu.err"; printf 'c4c_qemu_status=%s\n' "$?" > "$dir/c4c.qemu.status") && (qemu-riscv64 -strace -L /usr/riscv64-linux-gnu "$work/c4c.bin" > "$dir/c4c.qemu_strace.out" 2> "$dir/c4c.qemu_strace.err"; printf 'c4c_qemu_strace_status=%s\n' "$?" > "$dir/c4c.qemu_strace.status") && cat "$dir"/*.status > "$dir/status.summary" && rg -n 'foobar|logic\.rhs|logic\.end|logic_skip|logic_rhs|logic_end|select_chain|join_transfer|authoritative_branch_pair|%t43|%t44|%t48|bf\.|abort|tgkill|SIGIOT|Subprocess aborted|bne|beq|jal|jalr|mv\s+t3|mv\s+s2|lw\s+s2|andi|srl|sll|mul|store_source|spill_reload|storage %t43|storage %t44|storage %t48' "$dir/case.log" "$dir/semantic.bir.txt" "$dir/semantic.bir.err" "$dir/prepared.bir.txt" "$dir/prepared.bir.err" "$dir/c4c.o.objdump.txt" "$dir/c4c.bin.objdump.txt" "$dir/clang.bin.objdump.txt" "$dir/c4c.qemu.err" "$dir/c4c.qemu_strace.err" > "$dir/triage_hits.txt" 2>/dev/null || true && cat build/agent_state/411_step1_rhs_bitfield_boolean_abort/representative.runner.status "$dir/status.summary" && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result: proof completed. Statuses were `runner_status=1`,
`dump_bir_status=0`, `prepared_status=0`, `c4c_objdump_status=0`,
`c4c_bin_objdump_status=0`, `clang_bin_objdump_status=0`,
`clang_qemu_status=0`, `c4c_qemu_status=134`, and
`c4c_qemu_strace_status=134`. Canonical backend proof log:
`test_after.log`, PASS with 326/326 backend tests.
