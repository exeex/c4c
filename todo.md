Status: Active
Source Idea Path: ideas/open/511_static_local_object_data_contract_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish Static-Local Object Identity And Extents

# Current Packet

## Just Finished

Step 3 of `plan.md` published stable static-local object identity at the
producer boundary.

`Lowerer::lower_static_local_global` now interns the generated storage spelling
`__static_local_<function>_<id>` into the module text table and link-name table
before the synthetic `GlobalVar` is indexed and appended. The existing
HIR-to-LIR, LIR-to-BIR, and `populate_prepared_object_data_plans` path now sees
a valid `bir::Global::link_name_id` for function-scope static storage, so
selected object-data publication can carry `object_label`, label text,
object size, and alignment from producer facts instead of RV64 reconstructing
them from access records.

Focused static-local coverage was renamed from the stale missing-label fixture
to `riscv64_prepared_object_data_static_local_storage.c`. The new route test
checks the generated label text plus `.balign 4` and `.zero 4`; the object test
checks that the RV64 object route accepts the same source through prepared
object-data publication. The existing synthetic RV64 object-emission test still
preserves the `missing_object_label` fail-closed path when prepared object-data
facts are absent.

The representative `tests/c/external/gcc_torture/src/930513-2.c` now advances
past the old diagnostic. Before this packet it reported
`unsupported_global_data: prepared selected object-data contract
status=missing_object_label object_size_bytes=0 emitted_byte_count=0
zero_fill_byte_count=0`. After this packet `--codegen obj` returns `rc=0`, and
the asm probe shows `.bss`, `.balign 4`, `__static_local_eq_0:`, and `.zero 4`.

## Suggested Next

Execute Step 4: add focused coverage for initialized static-local data and
zero-fill range publication, including unsupported initializer or ambiguous
relocation shapes that must remain fail-closed.

## Watchouts

- `populate_prepared_object_data_plans` remains the selected object-data
  publication entry point; RV64 still consumes `PreparedGlobalObjectData` and
  does not infer labels, extents, bytes, zero-fill, or relocations from direct
  global access facts.
- The producer identity fix also lets current scalar zero-initialized static
  locals flow through existing zero-fill publication. Step 4 should make that
  data/zero-fill coverage explicit rather than moving authority into RV64.
- BIR still does not carry internal/global binding for object-data publication,
  so the object route cannot yet distinguish static-local symbol visibility
  from public globals without a separate producer contract extension.
- Externs, TLS/thread-local globals, GOT-required globals, marker-only
  unsupported data, dynamic initialization, ambiguous symbol-pointer
  relocation facts, and unrelated aggregate/ABI/F128 work remain outside this
  route unless a later plan explicitly owns them.

## Proof

- `cmake --build build --target c4cll` passed.
- `ctest --test-dir build -j --output-on-failure -R
  'prepared_object_data|backend_riscv_object_emission' | tee test_after.log`
  passed: 8/8 tests.
- `./build/c4cll --codegen obj --target riscv64-unknown-linux-gnu
  tests/c/external/gcc_torture/src/930513-2.c -o
  build/agent_state/511_step3_static_local_object_identity/930513-2/930513-2.o`
  passed with `rc=0`.
- `./build/c4cll --codegen asm --target riscv64-unknown-linux-gnu
  tests/c/external/gcc_torture/src/930513-2.c -o
  build/agent_state/511_step3_static_local_object_identity/930513-2/930513-2.s`
  showed the static-local label, 4-byte alignment, and 4-byte zero-fill record.
- `git diff --check -- src/frontend/hir/hir_types.cpp
  tests/backend/CMakeLists.txt
  tests/backend/case/riscv64_prepared_object_data_static_local_storage.c
  todo.md` passed.
