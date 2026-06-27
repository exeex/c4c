Status: Active
Source Idea Path: ideas/open/402_rv64_gcc_torture_runtime_abort_and_segfault_mismatches.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Prove Representatives And Residual Ownership

# Current Packet

## Just Finished

Step 3 proved the active 402 representatives after the frame-slot local-address
repair and classified the remaining runtime residual.

Artifacts:

- `build/agent_state/402_step3_runtime_residual_proof/representatives.runner.log`
- `build/agent_state/402_step3_runtime_residual_proof/20070212-2/`
- `build/agent_state/402_step3_runtime_residual_proof/20000113-1/`

Changed files:

- `todo.md`

Representative result:

- `20070212-2.c` remains fixed under clang-vs-c4c qemu comparison:
  `dump_bir_status=0`, `prepared_status=0`,
  `c4c_bin_objdump_status=0`, `clang_qemu_status=0`,
  `c4c_qemu_status=0`, `c4c_qemu_strace_status=0`.
- The fresh `20070212-2.c` prepared dump still publishes the repaired facts:
  `address_materialization block=block_1 inst_index=0 kind=frame_slot
  result=%lv.param.i1 ... offset=0` and
  `address_materialization block=block_2 inst_index=0 kind=frame_slot
  result=%lv.param.j1 ... offset=4`.
- The fresh `20070212-2.c` object still materializes those local addresses
  before storing `%lv.f1`: `mv t1, sp` for offset 0 and
  `addi t1, sp, 0x4` for offset 4, followed by `sd t1, 0x8(sp)`.
- `20000113-1.c` remains a distinct runtime abort:
  `dump_bir_status=0`, `prepared_status=0`,
  `c4c_bin_objdump_status=0`, `clang_qemu_status=0`,
  `c4c_qemu_status=134`, `c4c_qemu_strace_status=134`.
- `20000113-1.c` strace ends with `tgkill(..., SIGIOT)` and the runner reports
  `[RV64_BACKEND_RUNTIME_MISMATCH]` with `clang_exit=0` and
  `c4c_exit=Subprocess aborted`.

Residual classification:

- The remaining abort is not the Step 2 frame-slot local-address family.
- Prepared `20000113-1.c` reaches the relevant facts around `foobar`:
  `%t43.bf.unit/%t43.bf.shr/%t43.bf.mask/%t43`, `%t44`, `%t48`, and a
  `join_transfer logic.end.40 result=%t48 kind=phi_edge
  carrier=select_materialization ownership=authoritative_branch_pair`.
- Object evidence still shows the branch reaches `abort()` after stale or
  mis-scheduled value handling around the RHS bitfield/boolean materialization
  and select/join path. This should be routed as a concrete split or new active
  packet around RV64 object value scheduling/control-flow for RHS
  bitfield/boolean materialization, not as a continuation of local-address
  materialization.

## Suggested Next

Ask the plan owner to close the repaired local-address portion of 402 or split
the remaining `20000113-1.c` abort into a new/open idea named around RV64 object
value scheduling/control-flow for RHS bitfield/boolean materialization. Do not
continue 402 as another Step 2 implementation packet unless the plan owner
chooses to keep that residual inside this idea.

## Watchouts

- Do not claim compile-only proof for 402; these cases require clang-vs-c4c
  qemu runtime comparison.
- `runner_status=1` is expected in this proof because `20000113-1.c` remains
  mismatched; the runner summary is `total=2 passed=1 failed=1`.
- `20070212-2.c` is the acceptance proof for the repaired family and should not
  be reopened without fresh evidence.
- The remaining `20000113-1.c` route involves prepared select/join facts and
  RV64 object value scheduling/control-flow; avoid folding it into the
  frame-slot local-address repair.

## Proof

Exact delegated proof command run:

```sh
cmake --build --preset default &&
ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$' &&
mkdir -p build/agent_state/402_step3_runtime_residual_proof &&
printf 'src/20070212-2.c\nsrc/20000113-1.c\n' > build/agent_state/402_step3_runtime_residual_proof/representatives.allowlist &&
(set +e; ALLOWLIST=build/agent_state/402_step3_runtime_residual_proof/representatives.allowlist VERBOSE_FAILURES=1 CASE_TIMEOUT_SEC=20 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/402_step3_runtime_residual_proof/representatives.runner.log 2>&1; printf 'runner_status=%s\n' "$?" > build/agent_state/402_step3_runtime_residual_proof/representatives.runner.status) &&
for case in 20070212-2 20000113-1; do
  dir="build/agent_state/402_step3_runtime_residual_proof/${case}";
  work="build/rv64_gcc_c_torture_backend/src_${case}.c";
  src="tests/c/external/gcc_torture/src/${case}.c";
  mkdir -p "$dir";
  cp "$work/case.log" "$dir/case.log" 2>/dev/null || true;
  (build/c4cll --dump-bir --target riscv64-linux-gnu "$src" > "$dir/semantic.bir.txt" 2> "$dir/semantic.bir.err"; printf 'dump_bir_status=%s\n' "$?" > "$dir/semantic.bir.status");
  (build/c4cll --dump-prepared-bir --target riscv64-linux-gnu "$src" > "$dir/prepared.bir.txt" 2> "$dir/prepared.bir.err"; printf 'prepared_status=%s\n' "$?" > "$dir/prepared.bir.status");
  (llvm-objdump -d "$work/c4c.bin" > "$dir/c4c.bin.objdump.txt" 2> "$dir/c4c.bin.objdump.err"; printf 'c4c_bin_objdump_status=%s\n' "$?" > "$dir/c4c.bin.objdump.status");
  (qemu-riscv64 -L /usr/riscv64-linux-gnu "$work/clang.bin" > "$dir/clang.qemu.out" 2> "$dir/clang.qemu.err"; printf 'clang_qemu_status=%s\n' "$?" > "$dir/clang.qemu.status");
  (qemu-riscv64 -L /usr/riscv64-linux-gnu "$work/c4c.bin" > "$dir/c4c.qemu.out" 2> "$dir/c4c.qemu.err"; printf 'c4c_qemu_status=%s\n' "$?" > "$dir/c4c.qemu.status");
  (qemu-riscv64 -strace -L /usr/riscv64-linux-gnu "$work/c4c.bin" > "$dir/c4c.qemu_strace.out" 2> "$dir/c4c.qemu_strace.err"; printf 'c4c_qemu_strace_status=%s\n' "$?" > "$dir/c4c.qemu_strace.status");
  cat "$dir"/*.status > "$dir/status.summary";
  rg -n 'address_materialization|lv\.param|lv\.f1|addi\s+[^,]+,\s*sp|sd|ld|lw|sw|SIGSEGV|Subprocess aborted|RV64_BACKEND_RUNTIME_MISMATCH|abort|foobar|logic_rhs|logic_end|select_chain|%t43|%t44|%t48|tgkill|SIGIOT' "$dir/case.log" "$dir/semantic.bir.txt" "$dir/semantic.bir.err" "$dir/prepared.bir.txt" "$dir/prepared.bir.err" "$dir/c4c.bin.objdump.txt" "$dir/c4c.qemu.err" "$dir/c4c.qemu_strace.err" > "$dir/triage_hits.txt" 2>/dev/null || true;
done &&
cat build/agent_state/402_step3_runtime_residual_proof/representatives.runner.status build/agent_state/402_step3_runtime_residual_proof/20070212-2/status.summary build/agent_state/402_step3_runtime_residual_proof/20000113-1/status.summary &&
ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result:

- Command completed successfully.
- Focused `backend_riscv_object_emission` test passed.
- `representatives.runner.status`: `runner_status=1`, because only
  `20000113-1.c` still mismatches.
- `20070212-2.c`: `dump_bir_status=0`, `prepared_status=0`,
  `c4c_bin_objdump_status=0`, `clang_qemu_status=0`,
  `c4c_qemu_status=0`, `c4c_qemu_strace_status=0`.
- `20000113-1.c`: `dump_bir_status=0`, `prepared_status=0`,
  `c4c_bin_objdump_status=0`, `clang_qemu_status=0`,
  `c4c_qemu_status=134`, `c4c_qemu_strace_status=134`.
- `test_after.log`: backend subset passed, `100% tests passed, 0 tests failed
  out of 326`.
