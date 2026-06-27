Status: Active
Source Idea Path: ideas/open/404_prepared_wide_rematerialized_immediate_admission.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Classify Wide Immediate Producer Facts

# Current Packet

## Just Finished

Step 1 classified the current `tests/c/external/gcc_torture/src/int-compare.c`
wide-immediate failure. The semantic and prepared dumps complete, but object
codegen and the runner still fail before runtime:

- `dump_bir_status=0`
- `prepared_status=0`
- `codegen_obj_status=2`
- `runner_status=1`
- Runner diagnostic: `RV64_C4C_OBJ_COMPILE_FAIL` with
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64
  object lowering`.
- `test_after.log`: backend subset PASS, 326/326.

First bad fact: the wide literal call arguments are not the first unsupported
owner. Prepared call facts for `gt(i32 2147483647, i32 0)` at block 0,
instruction 4 publish complete immediate-source call-argument destinations:
`source_encoding=immediate source_literal=2147483647 ... dest_reg=a0
 dest_bank=gpr`, and RV64 call lowering already uses `append_rv64_load_immediate`
for immediate call arguments.

The first unsupported producer family starts at `%t6 = bir.sub i32 0,
2147483647`, followed by `%t7 = bir.sub i32 %t6, 1`. Prepared facts classify
those values as semantically complete rematerialized constants:

- `storage %t6 immediate bank=gpr imm=-2147483647`
- `storage %t7 immediate bank=gpr imm=-2147483648`
- `home %t6 value_id=27 kind=rematerializable_immediate imm_i32=-2147483647`
- `home %t7 value_id=28 kind=rematerializable_immediate imm_i32=-2147483648`

That means the current gap is not a missing register destination home and not a
need for RV64 to rediscover constant-expression semantics from BIR. The right
404 owner is a reusable prepared immediate-materialization/admission contract:
wide rematerialized immediate producers need a consumer-ready fact that says the
instruction is satisfied by the rematerializable immediate home, or must be
rejected earlier with a producer diagnostic. Current object emission reaches the
pure producer instruction and rejects it because the result has no register or
stack destination fragment to emit.

## Suggested Next

Execute Step 2: repair prepared wide immediate admission for named I32 producer
chains whose result home is `PreparedValueHomeKind::RematerializableImmediate`
with `imm_i32`, starting with the `%t6/%t7` `sub i32 0, 2147483647` and
subtract-one family. Prefer publishing/consuming an explicit rematerialized
immediate producer fact so RV64 object emission can skip the pure producer or
materialize it only when a later consumer asks for the value. Do not reconstruct
arbitrary BIR constant arithmetic in RV64 object emission.

Likely implementation/test surfaces to inspect:

- `src/backend/prealloc/regalloc/value_homes.cpp` for the current computed-value
  to `rematerializable_immediate` home publication.
- Prepared pure-instruction/object-consumer admission helpers around prepared
  object traversal and lookup support.
- `src/backend/mir/riscv/codegen/object_emission.cpp` only as the consumer of an
  explicit prepared rematerializable-immediate fact, not as a semantic inference
  engine.
- Focused tests in `tests/backend/bir/` for the prepared contract plus
  `tests/backend/mir/backend_riscv_object_emission_test.cpp` for RV64 consuming
  that explicit fact.

Suggested proof for Step 2: build, focused prepared/object-emission tests for the
new contract, then rerun the representative `int-compare.c` semantic/prepared,
object, runner, and backend subset proof with artifacts under
`build/agent_state/404_step2_wide_immediate_admission/`.

## Watchouts

- Do not special-case `int-compare.c`, `INT_MIN`, or exact `%t6/%t7` names.
- Do not treat the complete rematerializable immediate home as a register home;
  preserve immediate identity, signedness, and width.
- Do not move missing constant-expression semantics into RV64 object emission.
- If `int-compare.c` advances to a later unsupported instruction after Step 2,
  classify that residual separately instead of widening 404 silently.

## Proof

Ran the exact delegated Step 1 proof/classification command:

```sh
cmake --build --preset default && mkdir -p build/agent_state/404_step1_wide_immediate_classification && src="tests/c/external/gcc_torture/src/int-compare.c" && dir="build/agent_state/404_step1_wide_immediate_classification/int-compare" && mkdir -p "$dir" && (build/c4cll --dump-bir --target riscv64-linux-gnu "$src" > "$dir/semantic.bir.txt" 2> "$dir/semantic.bir.err"; printf 'dump_bir_status=%s\n' "$?" > "$dir/semantic.bir.status") && (build/c4cll --dump-prepared-bir --target riscv64-linux-gnu "$src" > "$dir/prepared.bir.txt" 2> "$dir/prepared.bir.err"; printf 'prepared_status=%s\n' "$?" > "$dir/prepared.bir.status") && (build/c4cll --codegen obj --target riscv64-linux-gnu "$src" -o "$dir/int-compare.o" > "$dir/codegen_obj.out" 2> "$dir/codegen_obj.err"; printf 'codegen_obj_status=%s\n' "$?" > "$dir/codegen_obj.status") && printf 'src/int-compare.c\n' > build/agent_state/404_step1_wide_immediate_classification/representative.allowlist && (set +e; ALLOWLIST=build/agent_state/404_step1_wide_immediate_classification/representative.allowlist VERBOSE_FAILURES=1 CASE_TIMEOUT_SEC=20 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/404_step1_wide_immediate_classification/representative.runner.log 2>&1; printf 'runner_status=%s\n' "$?" > build/agent_state/404_step1_wide_immediate_classification/representative.runner.status) && cp build/rv64_gcc_c_torture_backend/src_int-compare.c/case.log "$dir/case.log" 2>/dev/null || true && cat "$dir"/*.status build/agent_state/404_step1_wide_immediate_classification/representative.runner.status > "$dir/status.summary" && rg -n 'rematerial|immediate|INT_MIN|2147483647|2147483648|-2147483648|sub i32 0|add i32|constant|wide|no home|homeless|destination home|value_home|storage|producer|unsupported|fatal|error|RV64|object|materialization|prepared|compare|slt|sltu|xori|addi|lui' "$dir/semantic.bir.txt" "$dir/semantic.bir.err" "$dir/prepared.bir.txt" "$dir/prepared.bir.err" "$dir/codegen_obj.out" "$dir/codegen_obj.err" "$dir/case.log" build/agent_state/404_step1_wide_immediate_classification/representative.runner.log > "$dir/triage_hits.txt" 2>/dev/null || true && cat "$dir/status.summary" && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Artifacts are under `build/agent_state/404_step1_wide_immediate_classification/`.
Proof completed without tooling blockers; the representative remains blocked at
the classified 404 producer/prepared admission boundary.
