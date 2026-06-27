Status: Active
Source Idea Path: ideas/open/411_rv64_object_route_rhs_bitfield_boolean_select_abort.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Prove Representative And Residual Ownership

# Current Packet

## Just Finished

Step 3 proved active 411 representative closure. The exact delegated proof reran
focused RV64 object-emission coverage, refreshed semantic/prepared/object/qemu
artifacts for `src/20000113-1.c` and the 402 guard `src/20070212-2.c`, and kept
`test_after.log` as the canonical backend subset proof.

Results:

- `src/20000113-1.c`: `dump_bir_status=0`, `prepared_status=0`,
  `c4c_bin_objdump_status=0`, `clang_qemu_status=0`, `c4c_qemu_status=0`.
- `src/20070212-2.c`: `dump_bir_status=0`, `prepared_status=0`,
  `c4c_bin_objdump_status=0`, `clang_qemu_status=0`, `c4c_qemu_status=0`.
- Runner: `runner_status=0`; total=2, passed=2, failed=0.
- Focused `backend_riscv_object_emission`: PASS.
- Backend subset in `test_after.log`: PASS, 326/326.

Classification: the old `20000113-1.c` `SIGIOT`/abort runtime residual is absent
under clang-vs-c4c qemu comparison. The 402 guard `20070212-2.c` also remains
passing, so the frame-slot local-address repair did not regress. No fresh
residual appeared in this Step 3 proof.

## Suggested Next

Plan-owner lifecycle review: active 411 appears closure-ready. The representative
runtime owner is repaired, focused object-emission coverage is green, both qemu
representatives pass, and the backend subset remains green.

## Watchouts

- Do not claim compile-only proof; this closure proof includes qemu comparison
  for both representatives.
- Triage grep still finds source/prepared `abort` text because the test program
  contains abort paths, but runtime evidence shows neither representative aborts
  under c4c.
- Preserve `test_after.log` as the canonical executor proof log for this packet.

## Proof

Ran the exact delegated Step 3 proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$' && mkdir -p build/agent_state/411_step3_rhs_select_closure && printf 'src/20000113-1.c\nsrc/20070212-2.c\n' > build/agent_state/411_step3_rhs_select_closure/representatives.allowlist && (set +e; ALLOWLIST=build/agent_state/411_step3_rhs_select_closure/representatives.allowlist VERBOSE_FAILURES=1 CASE_TIMEOUT_SEC=20 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/411_step3_rhs_select_closure/representatives.runner.log 2>&1; printf 'runner_status=%s\n' "$?" > build/agent_state/411_step3_rhs_select_closure/representatives.runner.status) && for case in 20000113-1 20070212-2; do dir="build/agent_state/411_step3_rhs_select_closure/${case}"; work="build/rv64_gcc_c_torture_backend/src_${case}.c"; src="tests/c/external/gcc_torture/src/${case}.c"; mkdir -p "$dir"; cp "$work/case.log" "$dir/case.log" 2>/dev/null || true; (build/c4cll --dump-bir --target riscv64-linux-gnu "$src" > "$dir/semantic.bir.txt" 2> "$dir/semantic.bir.err"; printf 'dump_bir_status=%s\n' "$?" > "$dir/semantic.bir.status"); (build/c4cll --dump-prepared-bir --target riscv64-linux-gnu "$src" > "$dir/prepared.bir.txt" 2> "$dir/prepared.bir.err"; printf 'prepared_status=%s\n' "$?" > "$dir/prepared.bir.status"); (llvm-objdump -d "$work/c4c.bin" > "$dir/c4c.bin.objdump.txt" 2> "$dir/c4c.bin.objdump.err"; printf 'c4c_bin_objdump_status=%s\n' "$?" > "$dir/c4c_bin_objdump.status"); (qemu-riscv64 -L /usr/riscv64-linux-gnu "$work/clang.bin" > "$dir/clang.qemu.out" 2> "$dir/clang.qemu.err"; printf 'clang_qemu_status=%s\n' "$?" > "$dir/clang.qemu.status"); (qemu-riscv64 -L /usr/riscv64-linux-gnu "$work/c4c.bin" > "$dir/c4c.qemu.out" 2> "$dir/c4c.qemu.err"; printf 'c4c_qemu_status=%s\n' "$?" > "$dir/c4c.qemu.status"); cat "$dir"/*.status > "$dir/status.summary"; rg -n 'foobar|logic_rhs|logic_end|logic\.rhs|logic\.end|select_chain|join_transfer|authoritative_branch_pair|%t43|%t44|%t48|bf\.|abort|SIGIOT|Subprocess aborted|addi\s+[^,]+,\s*sp|sd|ld|lw|andi|srl|sll|mul|bne|beq|jal|jalr|mv\s+t3|mv\s+s2|lw\s+s2|address_materialization|lv\.param|lv\.f1|RV64_BACKEND_RUNTIME_MISMATCH' "$dir/case.log" "$dir/semantic.bir.txt" "$dir/semantic.bir.err" "$dir/prepared.bir.txt" "$dir/prepared.bir.err" "$dir/c4c.bin.objdump.txt" "$dir/c4c.qemu.err" > "$dir/triage_hits.txt" 2>/dev/null || true; done && cat build/agent_state/411_step3_rhs_select_closure/representatives.runner.status build/agent_state/411_step3_rhs_select_closure/20000113-1/status.summary build/agent_state/411_step3_rhs_select_closure/20070212-2/status.summary && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Artifacts are under `build/agent_state/411_step3_rhs_select_closure/`. Proof
completed without blockers.
