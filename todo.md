Status: Active
Source Idea Path: ideas/open/402_rv64_gcc_torture_runtime_abort_and_segfault_mismatches.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Classify Current Runtime Mismatch Families

# Current Packet

## Just Finished

Step 1 refreshed and classified the current runtime mismatch representatives.

Artifacts:

- `build/agent_state/402_step1_runtime_triage/representatives.runner.log`
- `build/agent_state/402_step1_runtime_triage/20000113-1/`
- `build/agent_state/402_step1_runtime_triage/20070212-2/`

Shared stage status:

- Runner status: `runner_status=1`, expected for this classification packet
  because both runnable representatives still mismatch clang under qemu.
- Both representatives clear compile/object evidence before runtime:
  `dump_bir_status=0`, `prepared_status=0`, `c4c_objdump_status=0`,
  `c4c_bin_objdump_status=0`, `clang_bin_objdump_status=0`.
- Clang qemu exits cleanly for both: `clang_qemu_status=0`.

Representative classification:

- `20000113-1.c`: c4c exits via abort, `c4c_qemu_status=134` and
  `c4c_qemu_strace_status=134`; strace ends in `tgkill(..., SIGIOT)`.
  Semantic/prepared BIR is present and reaches object route. The source builds
  packed unsigned bitfield updates in `foobar`; semantic BIR computes the RHS
  boolean operand as `%t43`/`%t44` before joining through `%t48`. The generated
  c4c binary instead compares a stale `s2` value against `5` at `foobar+0x3c8`
  before the later bitfield extraction at `foobar+0x3e4`, so the branch reaches
  `abort()`. This looks like an RV64 object/control-flow or value-scheduling
  bug around RHS block value materialization, not a compile-stage admission
  blocker.
- `20070212-2.c`: c4c segfaults, `c4c_qemu_status=139` and
  `c4c_qemu_strace_status=139`; strace reports `SIGSEGV si_addr=0x1`.
  Semantic BIR stores `&i1`/`&j1` in local pointer `%lv.f1` and then loads
  through it. Prepared output has explicit frame-slot address materialization:
  `address_materialization block=block_1 ... result=%lv.param.i1 offset=0`
  and `address_materialization block=block_2 ... result=%lv.param.j1 offset=4`.
  The c4c binary does not materialize those frame-slot addresses; it stores
  uninitialized register values (`mv t1, s1` / `mv t1, t0`) into `%lv.f1` and
  then executes `lw s1, 0(t0)`, producing the null/near-null segfault. This is
  the clearer first runtime family: frame-slot local-address materialization for
  address-exposed parameter homes/local pointer stores.

Abort and segfault are distinct families in the current evidence. Do not fold
the stale RHS/value-scheduling abort into the local-address segfault repair.

## Suggested Next

Execute Step 2 against the `20070212-2.c` family first: repair/support RV64
object lowering of prepared frame-slot address materialization used as a pointer
value for local pointer stores and indirect local loads. A focused proof should
show `&i1`/`&j1` lower to `sp+offset` addresses instead of uninitialized `s1` or
`t0`, and should keep the qemu comparison runnable.

## Watchouts

- Do not claim compile-only proof for 402; these cases require clang-vs-c4c
  qemu runtime comparison.
- Keep `20000113-1.c` as a separate residual: likely RV64 object value
  scheduling/control-flow around RHS boolean materialization after bitfield
  loads.
- The `20070212-2.c` prepared facts already expose frame-slot address
  materialization; avoid reconstructing local-address meaning from source names
  or testcase shape.
- Do not weaken expectations or route by filename-specific handling.

## Proof

Exact delegated proof command run:

```sh
cmake --build --preset default &&
mkdir -p build/agent_state/402_step1_runtime_triage &&
printf 'src/20000113-1.c\nsrc/20070212-2.c\n' > build/agent_state/402_step1_runtime_triage/representatives.allowlist &&
(set +e; ALLOWLIST=build/agent_state/402_step1_runtime_triage/representatives.allowlist VERBOSE_FAILURES=1 CASE_TIMEOUT_SEC=20 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/402_step1_runtime_triage/representatives.runner.log 2>&1; printf 'runner_status=%s\n' "$?" > build/agent_state/402_step1_runtime_triage/representatives.runner.status) &&
for case in 20000113-1 20070212-2; do
  src="tests/c/external/gcc_torture/src/${case}.c";
  dir="build/agent_state/402_step1_runtime_triage/${case}";
  work="build/rv64_gcc_c_torture_backend/src_${case}.c";
  mkdir -p "$dir";
  cp "$work/case.log" "$dir/case.log" 2>/dev/null || true;
  (build/c4cll --dump-bir --target riscv64-linux-gnu "$src" > "$dir/semantic.bir.txt" 2> "$dir/semantic.bir.err"; printf 'dump_bir_status=%s\n' "$?" > "$dir/semantic.bir.status");
  (build/c4cll --dump-prepared-bir --target riscv64-linux-gnu "$src" > "$dir/prepared.bir.txt" 2> "$dir/prepared.bir.err"; printf 'prepared_status=%s\n' "$?" > "$dir/prepared.bir.status");
  (llvm-objdump -dr "$work/c4c.o" > "$dir/c4c.o.objdump.txt" 2> "$dir/c4c.o.objdump.err"; printf 'c4c_objdump_status=%s\n' "$?" > "$dir/c4c.o.objdump.status");
  (llvm-objdump -d "$work/c4c.bin" > "$dir/c4c.bin.objdump.txt" 2> "$dir/c4c.bin.objdump.err"; printf 'c4c_bin_objdump_status=%s\n' "$?" > "$dir/c4c.bin.objdump.status");
  (llvm-objdump -d "$work/clang.bin" > "$dir/clang.bin.objdump.txt" 2> "$dir/clang.bin.objdump.err"; printf 'clang_bin_objdump_status=%s\n' "$?" > "$dir/clang.bin.objdump.status");
  (qemu-riscv64 -L /usr/riscv64-linux-gnu "$work/clang.bin" > "$dir/clang.qemu.out" 2> "$dir/clang.qemu.err"; printf 'clang_qemu_status=%s\n' "$?" > "$dir/clang.qemu.status");
  (qemu-riscv64 -L /usr/riscv64-linux-gnu "$work/c4c.bin" > "$dir/c4c.qemu.out" 2> "$dir/c4c.qemu.err"; printf 'c4c_qemu_status=%s\n' "$?" > "$dir/c4c.qemu.status");
  (qemu-riscv64 -strace -L /usr/riscv64-linux-gnu "$work/c4c.bin" > "$dir/c4c.qemu_strace.out" 2> "$dir/c4c.qemu_strace.err"; printf 'c4c_qemu_strace_status=%s\n' "$?" > "$dir/c4c.qemu_strace.status");
  cat "$dir"/*.status > "$dir/status.summary";
  rg -n 'abort|__builtin_abort|main|test|foo|bar|call|ret|jal|jalr|sp|sd|ld|sw|lw|sext|zext|store|load|prepared|unsupported|fatal|error|Segmentation fault|Subprocess aborted' "$dir/case.log" "$dir/semantic.bir.txt" "$dir/semantic.bir.err" "$dir/prepared.bir.txt" "$dir/prepared.bir.err" "$dir/c4c.o.objdump.txt" "$dir/c4c.bin.objdump.txt" "$dir/clang.bin.objdump.txt" "$dir/c4c.qemu.err" "$dir/c4c.qemu_strace.err" > "$dir/triage_hits.txt" 2>/dev/null || true;
done &&
cat build/agent_state/402_step1_runtime_triage/representatives.runner.status build/agent_state/402_step1_runtime_triage/20000113-1/status.summary build/agent_state/402_step1_runtime_triage/20070212-2/status.summary &&
ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result:

- Command completed successfully.
- `representatives.runner.status`: `runner_status=1`, with
  `[RV64_BACKEND_RUNTIME_MISMATCH]` for both representatives.
- `20000113-1.c`: clang qemu `0`, c4c qemu `134`, c4c strace `134`.
- `20070212-2.c`: clang qemu `0`, c4c qemu `139`, c4c strace `139`.
- `test_after.log`: backend subset passed, `100% tests passed, 0 tests failed
  out of 326`.
