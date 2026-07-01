Status: Active
Source Idea Path: ideas/open/511_static_local_object_data_contract_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Publish Static-Local Data And Zero-Fill Ranges

# Current Packet

## Just Finished

Step 4 of `plan.md` made static-local data and zero-fill publication explicit
through focused ordinary-C backend coverage.

No additional implementation change was needed beyond the existing Step 3
producer identity fix. Once `Lowerer::lower_static_local_global` publishes a
valid static-local `link_name_id`, the existing
`populate_prepared_object_data_plans` path publishes coherent selected
object-data records for supported function-scope static storage:

- zero/default-initialized mutable static locals flow to BSS with
  `zero_fill_byte_count == object_size_bytes` and no emitted data
- nonzero scalar static locals flow to `.data` with initialized bytes and no
  zero-fill

Added `riscv64_prepared_object_data_static_local_initialized_storage.c` and
registered object, assembly-route, and object-runtime tests for it. The
assembly route proves `.data`, `.balign 4`,
`__static_local_rv64_step4_static_initialized_counter_0:`, and `.word 13`.
The object test checks the initialized payload bytes in the ELF image, and the
runtime test returns 24 to prove static retention across calls. The existing
static-local zero-fill fixture is now also covered by an object-runtime test,
returning 11, in addition to its object and `.bss`/`.zero 4` route coverage.

The representative `tests/c/external/gcc_torture/src/930513-2.c` still passes
`--codegen obj` after this packet.

## Suggested Next

Execute Step 5: add or consolidate ordinary-C static-local backend coverage for
the repaired producer contract, or confirm that the Step 3 and Step 4 backend
tests already satisfy the Step 5 coverage intent.

## Watchouts

- `populate_prepared_object_data_plans` remains the selected object-data
  publication entry point; RV64 still consumes `PreparedGlobalObjectData` and
  does not infer labels, extents, bytes, zero-fill, or relocations from direct
  global access facts.
- Prepared object-data records are not currently printed in
  `--dump-prepared-bir`; Step 4 therefore proves static-local data and
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
  'prepared_object_data|backend_riscv_object_emission' | tee test_after.log`
  passed: 12/12 tests.
- `./build/c4cll --codegen obj --target riscv64-unknown-linux-gnu
  tests/c/external/gcc_torture/src/930513-2.c -o
  build/agent_state/511_step4_static_local_object_data/930513-2/930513-2.o`
  passed with `rc=0`.
- `git diff --check -- tests/backend/CMakeLists.txt
  tests/backend/case/riscv64_prepared_object_data_static_local_initialized_storage.c
  todo.md` passed.
