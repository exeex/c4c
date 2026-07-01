Status: Active
Source Idea Path: ideas/open/511_static_local_object_data_contract_publication.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Add Ordinary-C Static-Local Backend Coverage

# Current Packet

## Just Finished

Step 5 of `plan.md` audited the ordinary-C static-local backend coverage
matrix. No new tests were needed.

Existing Step 3 and Step 4 coverage already satisfies the Step 5 intent:

- initialized function-scope static object:
  `riscv64_prepared_object_data_static_local_initialized_storage.c` has object,
  assembly-route, and object-runtime coverage; the route proves `.data`,
  `.balign 4`, the generated static-local label, and `.word 13`, while runtime
  returns 24 to prove static retention across calls
- zero/default-initialized function-scope static object:
  `riscv64_prepared_object_data_static_local_storage.c` has object,
  assembly-route, and object-runtime coverage; the route proves `.bss`,
  `.balign 4`, the generated static-local label, and `.zero 4`, while runtime
  returns 11
- representative route:
  `tests/c/external/gcc_torture/src/930513-2.c` now passes `--codegen obj`,
  advancing past the old `missing_object_label` selected object-data rejection
- missing-authority guard:
  `backend_riscv_object_emission` still covers synthetic prepared object-data
  missing-authority and fail-closed variants; no expected-failure contract was
  weakened or moved

## Suggested Next

Execute Step 6: validate the full idea 511 slice and prepare closure or a
narrowed follow-up recommendation.

## Watchouts

- `populate_prepared_object_data_plans` remains the selected object-data
  publication entry point; RV64 still consumes `PreparedGlobalObjectData` and
  does not infer labels, extents, bytes, zero-fill, or relocations from direct
  global access facts.
- Prepared object-data records are not currently printed in
  `--dump-prepared-bir`; Step 5 therefore proves static-local data and
  zero-fill through object bytes, assembly storage directives, runtime behavior,
  and the existing `backend_riscv_object_emission` contract checks.
- BIR still does not carry internal/global binding for object-data publication,
  so the object route cannot yet distinguish static-local symbol visibility
  from public globals without a separate producer contract extension.
- Unsupported initializer semantics, missing/conflicting emitted bytes,
  missing/conflicting zero-fill, missing/conflicting relocation facts,
  TLS/thread-local globals, GOT-required globals, marker-only unsupported data,
  externs, dynamic initialization, and ambiguous symbol-pointer relocations
  remain covered by the existing synthetic object-emission and verifier
  fail-closed tests; this packet did not change those paths.

## Proof

- `cmake --build build --target c4cll` passed.
- `ctest --test-dir build -j --output-on-failure -R
  'prepared_object_data_static_local|backend_riscv_object_emission' | tee
  test_after.log` passed: 7/7 tests.
- `./build/c4cll --codegen obj --target riscv64-unknown-linux-gnu
  tests/c/external/gcc_torture/src/930513-2.c -o
  build/agent_state/511_step5_ordinary_c_static_local_backend_coverage/930513-2/930513-2.o`
  passed with `rc=0`.
- `git diff --check -- todo.md` passed.
