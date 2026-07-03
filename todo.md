Status: Active
Source Idea Path: ideas/open/562_bir_direct_call_semantic_boundary_evidence.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prove Representative And Adjacent Call Behavior

# Current Packet

## Just Finished

Step 5: proved the original direct-call representative
`tests/c/external/gcc_torture/src/20000717-1.c` after the Step 4 BIR repair.

Observed route results:
- Semantic BIR succeeds for the RV64 target and emits direct byval aggregate
  calls:
  - `bir.call i32 bar(i32 %p.i, ptr byval(size=12, align=4) %lv.param.p.t)`
  - `bir.call i32 foo(ptr byval(size=12, align=4) %lv.t, i32 4)`
- Prepared BIR succeeds and records same-module call plans for both direct
  calls, including stack-copy aggregate transport for the byval payloads.
- RV64 object generation succeeds and writes
  `build/agent_state/562_step5_direct_call_representative/20000717-1.rv64.o`
  as a non-empty object file.
- No first bad fact remains for this representative in the semantic BIR,
  prepared BIR, or RV64 object routes covered by the delegated command.

## Suggested Next

Supervisor should decide whether Step 5 completion is enough to send the active
runbook to lifecycle review/closure, or whether another packet should add a
tracked regression surface for `20000717-1.c`.

## Watchouts

- The representative now reaches the RV64 object route, so any next failure
  family is outside the delegated direct-call proof command.
- The prepared dump still records select carrier publication limitations in
  `bar`:
  `missing_carrier_aliases` for the `%t15` predecessor and
  `unsupported_publication` for the immediate predecessor. These did not block
  the delegated direct-call/object proof.
- Evidence lives under
  `build/agent_state/562_step5_direct_call_representative/`.

## Proof

Ran delegated proof:
`cmake --build --preset default && mkdir -p build/agent_state/562_step5_direct_call_representative && ./build/c4cll --dump-bir --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/20000717-1.c > build/agent_state/562_step5_direct_call_representative/20000717-1.dump-bir.stdout 2> build/agent_state/562_step5_direct_call_representative/20000717-1.dump-bir.stderr && ./build/c4cll --dump-prepared-bir --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/20000717-1.c > build/agent_state/562_step5_direct_call_representative/20000717-1.dump-prepared-bir.stdout 2> build/agent_state/562_step5_direct_call_representative/20000717-1.dump-prepared-bir.stderr && ./build/c4cll --codegen obj --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/20000717-1.c -o build/agent_state/562_step5_direct_call_representative/20000717-1.rv64.o > build/agent_state/562_step5_direct_call_representative/20000717-1.codegen-obj.stdout 2> build/agent_state/562_step5_direct_call_representative/20000717-1.codegen-obj.stderr && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log && git diff --check -- todo.md`.

Result: passed. `test_after.log` contains the backend CTest subset output with
`346/346` selected tests passing and reports
`Total Test time (real) =   2.11 sec`.
Supervisor regression comparison also passed with:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Evidence artifacts:
- `build/agent_state/562_step5_direct_call_representative/20000717-1.dump-bir.stdout`
- `build/agent_state/562_step5_direct_call_representative/20000717-1.dump-bir.stderr`
- `build/agent_state/562_step5_direct_call_representative/20000717-1.dump-prepared-bir.stdout`
- `build/agent_state/562_step5_direct_call_representative/20000717-1.dump-prepared-bir.stderr`
- `build/agent_state/562_step5_direct_call_representative/20000717-1.codegen-obj.stdout`
- `build/agent_state/562_step5_direct_call_representative/20000717-1.codegen-obj.stderr`
- `build/agent_state/562_step5_direct_call_representative/20000717-1.rv64.o`

All three representative stderr files are empty; the object file is 1776 bytes.
