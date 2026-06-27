Status: Active
Source Idea Path: ideas/open/404_prepared_wide_rematerialized_immediate_admission.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Repair Prepared Wide Immediate Admission

# Current Packet

## Just Finished

Step 2 repaired the prepared wide-immediate object admission gap for named I32
non-compare pure binary producers whose result has an explicit
`PreparedValueHomeKind::RematerializableImmediate` home with `imm_i32`.
`src/backend/mir/riscv/codegen/object_emission.cpp` now consumes that prepared
home as the authority that the producer is already represented by an immediate
value and emits no standalone RV64 fragment for that pure producer. It does not
recompute constant-expression semantics from the BIR instruction.

Focused coverage added in
`tests/backend/mir/backend_riscv_object_emission_test.cpp` builds a traversed
control-flow block with `%wide.base = sub i32 0, 2147483647` and
`%wide.min = sub i32 %wide.base, 1`, publishes only rematerializable-immediate
homes for those producers, and verifies the return consumer materializes
`-2147483648` with the existing wide-immediate RV64 sequence.

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
the `%t6/%t7` rematerialized immediate producer chain is absent. No later
residual appeared for this representative in the delegated proof.

## Suggested Next

Execute Step 3: prove representative closure and residual ownership for 404.
Use `int-compare.c` as the guard that wide rematerializable-immediate producer
admission remains fixed and classify any fresh residual if one appears. If the
representative continues to pass compile and runner proof, 404 appears
closure-ready for plan-owner lifecycle review.

## Watchouts

- Do not special-case `int-compare.c`, `INT_MIN`, or exact `%t6/%t7` names.
- Do not treat the complete rematerializable immediate home as a register home;
  preserve immediate identity, signedness, and width.
- Do not move missing constant-expression semantics into RV64 object emission.
- Keep the new object-consumer behavior limited to explicit prepared
  rematerializable-immediate facts. If a later proof exposes a non-immediate or
  missing-home producer, classify it separately instead of widening 404
  silently.

## Proof

Ran the exact delegated Step 2 proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_object_consumer_contract|backend_riscv_object_emission)$' && mkdir -p build/agent_state/404_step2_wide_immediate_admission && src="tests/c/external/gcc_torture/src/int-compare.c" && dir="build/agent_state/404_step2_wide_immediate_admission/int-compare" && mkdir -p "$dir" && (build/c4cll --dump-bir --target riscv64-linux-gnu "$src" > "$dir/semantic.bir.txt" 2> "$dir/semantic.bir.err"; printf 'dump_bir_status=%s\n' "$?" > "$dir/semantic.bir.status") && (build/c4cll --dump-prepared-bir --target riscv64-linux-gnu "$src" > "$dir/prepared.bir.txt" 2> "$dir/prepared.bir.err"; printf 'prepared_status=%s\n' "$?" > "$dir/prepared.bir.status") && (build/c4cll --codegen obj --target riscv64-linux-gnu "$src" -o "$dir/int-compare.o" > "$dir/codegen_obj.out" 2> "$dir/codegen_obj.err"; printf 'codegen_obj_status=%s\n' "$?" > "$dir/codegen_obj.status") && printf 'src/int-compare.c\n' > build/agent_state/404_step2_wide_immediate_admission/representative.allowlist && (set +e; ALLOWLIST=build/agent_state/404_step2_wide_immediate_admission/representative.allowlist VERBOSE_FAILURES=1 CASE_TIMEOUT_SEC=20 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/404_step2_wide_immediate_admission/representative.runner.log 2>&1; printf 'runner_status=%s\n' "$?" > build/agent_state/404_step2_wide_immediate_admission/representative.runner.status) && cp build/rv64_gcc_c_torture_backend/src_int-compare.c/case.log "$dir/case.log" 2>/dev/null || true && cat "$dir"/*.status build/agent_state/404_step2_wide_immediate_admission/representative.runner.status > "$dir/status.summary" && rg -n 'rematerial|immediate|2147483647|2147483648|-2147483648|sub i32 0|storage %t6|storage %t7|home %t6|home %t7|unsupported|fatal|error|RV64|object|materialization|prepared|slt|sltu|xori|addi|lui|PASS|FAIL|RV64_C4C_OBJ_COMPILE_FAIL|RV64_BACKEND_RUNTIME_MISMATCH' "$dir/semantic.bir.txt" "$dir/semantic.bir.err" "$dir/prepared.bir.txt" "$dir/prepared.bir.err" "$dir/codegen_obj.out" "$dir/codegen_obj.err" "$dir/case.log" build/agent_state/404_step2_wide_immediate_admission/representative.runner.log > "$dir/triage_hits.txt" 2>/dev/null || true && cat "$dir/status.summary" && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Artifacts are under `build/agent_state/404_step2_wide_immediate_admission/`.
Focused tests passed:
`backend_prepared_object_consumer_contract` and
`backend_riscv_object_emission`. Proof completed without tooling blockers.
