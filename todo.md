Status: Active
Source Idea Path: ideas/open/510_rv64_selected_object_data_emission.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Add Ordinary-C Backend Coverage

# Current Packet

## Just Finished

Step 5 of `plan.md` added ordinary-C backend coverage for selected
object-data emission through prepared facts, without changing expectations,
unsupported markers, allowlists, runtime accounting, scan accounting, or
producer publication.

- Changed files:
  `tests/backend/case/riscv64_prepared_object_data_initialized_storage.c`,
  `tests/backend/case/riscv64_prepared_object_data_zero_fill_storage.c`,
  `tests/backend/case/riscv64_prepared_object_data_missing_static_label.c`,
  `tests/backend/CMakeLists.txt`, and `todo.md`.
- Coverage rationale: the initialized ordinary-C case uses a mixed global
  aggregate with prepared object-label, extent, alignment, section, and byte
  authority; the object-route test checks the emitted RV64 ELF and the prepared
  byte payload `050000000908070611000000`, and the object-runtime test links
  and runs the object under QEMU with exit code `29`.
- Zero-fill coverage uses a separate ordinary-C global aggregate with prepared
  BSS/zero-fill authority; the focused ASM route checks `.bss`, `.zero 8`, and
  global-memory instructions, while the object-route and object-runtime tests
  check RV64 ELF emission and QEMU exit code `11`.
- Fail-closed coverage uses an ordinary-C function-scope static object that
  still lacks selected object-label authority and is covered by an object-route
  expected-failure test for
  `unsupported_global_data: prepared selected object-data contract status=missing_object_label object_size_bytes=0 emitted_byte_count=0 zero_fill_byte_count=0`.
- Representative `tests/c/external/gcc_torture/src/20000412-1.c` remains
  fail-closed with
  `unsupported_global_data: prepared selected object-data contract status=unsupported_but_coherent object_label_id=2 object_size_bytes=1656 emitted_byte_count=0 zero_fill_byte_count=0`;
  this remains acceptable because its prepared facts are still marker-only and
  lack selected byte, zero-fill, or relocation authority.

## Suggested Next

Execute Step 6: validate the slice for handoff, confirm producer-missing rows
such as `src/930513-2.c` remain fail-closed, and prepare closure or a narrowed
producer follow-up recommendation.

## Watchouts

- The initialized ASM route for the mixed aggregate still reports
  `riscv prepared module emitter does not support this prepared global storage layout`;
  Step 5 covers that shape through the RV64 object route and object-runtime
  path instead of expanding the legacy ASM emitter.
- Current relocation support for selected symbol-pointer globals remains the
  pre-existing narrow fallback; do not broaden relocation emission unless
  prepared relocation-ready records exist.
- Missing static-local object labels remain a producer-contract gap. Do not
  repair them in RV64 by reconstructing storage from raw names or C source
  shape.
- Thread-local storage, GOT-required globals, parameter homes, unrelated global
  access widths, and F128 stay out of this RV64 consumer slice.

## Proof

- `scripts/plan_review_state.py set-step --step-id 5 --step-title 'Add Ordinary-C Backend Coverage'`
  passed.
- `cmake --build build --target c4cll` passed.
- Focused Step 5 CTest:
  `ctest --test-dir build --output-on-failure -R '^backend_(cli_riscv64_prepared_object_data_|codegen_route_riscv64_prepared_object_data_|obj_runtime_rv64_prepared_object_data_)'`
  passed.
- Focused representative probe:
  `build/c4cll -I /workspaces/c4c --codegen obj --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000412-1.c -o build/agent_state/510_step5_ordinary_c_backend_coverage/20000412-1/20000412-1.o`
  failed closed with the expected marker-only selected object-data diagnostic.
- `git diff --check -- tests/backend/CMakeLists.txt tests/backend/case/riscv64_prepared_object_data_initialized_storage.c tests/backend/case/riscv64_prepared_object_data_missing_static_label.c tests/backend/case/riscv64_prepared_object_data_zero_fill_storage.c todo.md`
  passed.
- No-index whitespace checks for the three new `tests/backend/case/*.c` files
  passed.
- `ctest --test-dir build -j --output-on-failure -R '^backend_'` passed and
  wrote canonical proof log `test_after.log`.
