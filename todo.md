Status: Active
Source Idea Path: ideas/open/423_rv64_runtime_mismatch_triage.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Select Representatives And Reproduce

# Current Packet

## Just Finished

Completed `plan.md` Step 2 by reproducing the selected RV64 gcc_torture
runtime mismatch representatives with the single-case backend object runner.
Evidence is recorded in
`docs/rv64_gcc_torture_post_contract/runtime_mismatch_representatives.md`,
with per-case logs under
`build/agent_state/423_step2_runtime_reproduction/`. Six representatives
still reproduce as runtime mismatches; `src/20000314-2.c` now passes the
single-case rerun and should be treated as stale or non-deterministic for the
next packet unless a later scan reproduces it again.

## Suggested Next

Execute Step 3 first-wrong-edge work on still-reproducing representatives:
one abort case such as `src/pr38533.c` or `src/pr83298.c`, one segfault case
such as `src/20080506-2.c`, `src/pr43008.c`, or `src/zerolen-1.c`, and the
exit-255 singleton `src/pr81503.c`. Preserve the distinction between runtime
ABI/RV64 lowering evidence and unsupported compile/codegen bucket work.

## Watchouts

- This is a triage runbook, not an expectation or allowlist route.
- Do not weaken runtime comparison, expected output, unsupported markers, or
  pass/fail accounting.
- `src/20000314-2.c` was an abort row in the Step 1 full inventory but passed
  this Step 2 single-case reproduction.
- Still-reproducing modes are abort (`src/pr38533.c`, `src/pr83298.c`),
  segfault (`src/20080506-2.c`, `src/pr43008.c`, `src/zerolen-1.c`), and
  exit 255 (`src/pr81503.c`).
- If Step 3 finds missing prepared/BIR authority rather than RV64 target
  behavior, split that as producer-owned follow-up instead of inferring facts
  in RV64 lowering.

## Proof

Passed:
`bash -lc 'set -u; out=build/agent_state/423_step2_runtime_reproduction; rm -rf "$out"; mkdir -p "$out"; : > test_after.log; cmake --build build --target c4cll >> test_after.log 2>&1 || exit 1; printf "case\trc\tlog\n" > "$out/summary.tsv"; cases="src/20000314-2.c src/pr38533.c src/pr83298.c src/20080506-2.c src/pr43008.c src/zerolen-1.c src/pr81503.c"; for case in $cases; do id=$(printf "%s" "$case" | sed "s#[^A-Za-z0-9_.-]#_#g"); log="$out/${id}.case.log"; set +e; cmake -DCOMPILER="$(pwd)/build/c4cll" -DCLANG="$(command -v clang)" -DQEMU_RISCV64="$(command -v qemu-riscv64)" -DSRC="$(pwd)/tests/c/external/gcc_torture/$case" -DROOT="$(pwd)/tests/c/external/gcc_torture" -DTARGET_TRIPLE=riscv64-linux-gnu -DSYSROOT=/usr/riscv64-linux-gnu -DOUT_CLANG_BIN="$(pwd)/$out/${id}.clang.bin" -DOUT_OBJECT="$(pwd)/$out/${id}.c4c.o" -DOUT_C4C_BIN="$(pwd)/$out/${id}.c4c.bin" -DCASE_TIMEOUT_SEC=20 -P tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake > "$log" 2>&1; rc=$?; set -e; printf "%s\t%s\t%s\n" "$case" "$rc" "$log" | tee -a "$out/summary.tsv" >> test_after.log; tail -n 16 "$log" >> test_after.log; done; rg -n "RV64_BACKEND_RUNTIME_MISMATCH" "$out"/*.case.log >> test_after.log; rg -n "clang_exit=0 c4c_exit=(Subprocess aborted|Segmentation fault|255)" "$out"/*.case.log >> test_after.log; git diff --check -- docs/rv64_gcc_torture_post_contract/runtime_mismatch_representatives.md todo.md >> test_after.log 2>&1'`.
Proof output is preserved in `test_after.log`.
