Status: Active
Source Idea Path: ideas/open/404_prepared_wide_rematerialized_immediate_admission.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Prove Representative And Residual Ownership

# Current Packet

## Just Finished

Step 3 proved representative closure for 404 after Step 2 commit `29d6c33c`
(`Admit prepared rematerialized binary immediates`). The primary guard
`tests/c/external/gcc_torture/src/int-compare.c` still publishes the wide
rematerialized immediate producer facts in prepared output and now compiles
through the RV64 object route and runner.

Representative result for
`tests/c/external/gcc_torture/src/int-compare.c`:

- `dump_bir_status=0`
- `prepared_status=0`
- `codegen_obj_status=0`
- `runner_status=0`
- Runner summary: `[rv64-gcc-torture] total=1 passed=1 failed=0`
- `case.log`: `[PASS][rv64-gcc-torture-backend-obj] .../int-compare.c`
- `test_after.log`: backend subset PASS, 326/326.

The old `RV64_C4C_OBJ_COMPILE_FAIL` / `unsupported_instruction_fragment` for
the `%t6/%t7`-style rematerialized I32 wide-immediate producer chain remains
absent. No fresh residual appeared for this representative in the Step 3 proof.
404 is closure-ready for plan-owner lifecycle review from the executor side.

## Suggested Next

Plan-owner lifecycle review for 404: close if the source idea only requires the
proved representative and committed focused coverage, or decide whether any
broader same-idea proof should be requested before closure.

## Watchouts

- Step 3 was proof/classification only; no implementation files were edited in
  this packet.
- Keep future 404-adjacent work limited to explicit prepared
  rematerializable-immediate facts. Non-immediate, missing-home, or separate
  unsupported producer shapes should be split or routed separately.

## Proof

Ran the exact delegated Step 3 proof command:

```sh
cmake --build --preset default && mkdir -p build/agent_state/404_step3_representative_closure && src="tests/c/external/gcc_torture/src/int-compare.c" && dir="build/agent_state/404_step3_representative_closure/int-compare" && mkdir -p "$dir" && (build/c4cll --dump-bir --target riscv64-linux-gnu "$src" > "$dir/semantic.bir.txt" 2> "$dir/semantic.bir.err"; printf 'dump_bir_status=%s\n' "$?" > "$dir/semantic.bir.status") && (build/c4cll --dump-prepared-bir --target riscv64-linux-gnu "$src" > "$dir/prepared.bir.txt" 2> "$dir/prepared.bir.err"; printf 'prepared_status=%s\n' "$?" > "$dir/prepared.bir.status") && (build/c4cll --codegen obj --target riscv64-linux-gnu "$src" -o "$dir/int-compare.o" > "$dir/codegen_obj.out" 2> "$dir/codegen_obj.err"; printf 'codegen_obj_status=%s\n' "$?" > "$dir/codegen_obj.status") && printf 'src/int-compare.c\n' > build/agent_state/404_step3_representative_closure/representative.allowlist && (set +e; ALLOWLIST=build/agent_state/404_step3_representative_closure/representative.allowlist VERBOSE_FAILURES=1 CASE_TIMEOUT_SEC=20 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/404_step3_representative_closure/representative.runner.log 2>&1; printf 'runner_status=%s\n' "$?" > build/agent_state/404_step3_representative_closure/representative.runner.status) && cp build/rv64_gcc_c_torture_backend/src_int-compare.c/case.log "$dir/case.log" 2>/dev/null || true && cat "$dir"/*.status build/agent_state/404_step3_representative_closure/representative.runner.status > "$dir/status.summary" && rg -n 'rematerial|immediate|2147483647|2147483648|-2147483648|sub i32 0|storage %t6|storage %t7|home %t6|home %t7|unsupported|fatal|error|RV64|object|materialization|prepared|slt|sltu|xori|addi|lui|PASS|FAIL|RV64_C4C_OBJ_COMPILE_FAIL|RV64_BACKEND_RUNTIME_MISMATCH|total=|passed=|failed=' "$dir/semantic.bir.txt" "$dir/semantic.bir.err" "$dir/prepared.bir.txt" "$dir/prepared.bir.err" "$dir/codegen_obj.out" "$dir/codegen_obj.err" "$dir/case.log" build/agent_state/404_step3_representative_closure/representative.runner.log > "$dir/triage_hits.txt" 2>/dev/null || true && cat "$dir/status.summary" && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Artifacts are under `build/agent_state/404_step3_representative_closure/`.
Proof completed without tooling blockers. `test_after.log` is the canonical
backend subset proof log.
