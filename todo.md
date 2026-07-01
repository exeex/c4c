Status: Active
Source Idea Path: ideas/open/510_rv64_selected_object_data_emission.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Contract Admission And Fail-Closed Guards

# Current Packet

## Just Finished

Step 3 of `plan.md` implemented RV64 selected object-data contract admission
and fail-closed guards without changing expectations, scan accounting, or broad
emission behavior.

- Changed files:
  `src/backend/mir/riscv/codegen/object_emission.cpp`,
  `tests/backend/mir/backend_riscv_object_emission_test.cpp`, and `todo.md`.
- Admission boundary: coherent prepared records are admitted only when the
  selected object-data record has explicit label text, publication identity,
  zero object offset, nonzero extent, nonzero alignment, and the payload
  authority required by its prepared section kind. `.data` and `.rodata`
  require prepared emitted bytes exactly matching object extent; `.bss`
  requires prepared zero-fill count exactly matching object extent.
- Existing partial support remains limited to coherent initialized globals,
  coherent BSS/zero-fill, coherent constants/rodata, selected symbol-pointer
  relocation globals, and selected zero-pointer/no-initializer BSS.
- The two existing `unsupported_but_coherent` fallback admissions now require
  explicit prepared label/extent/alignment/publication identity, unsupported
  marker authority, no emitted-byte/zero-fill/relocation authority flags, and
  reject TLS and GOT-required globals.
- Focused tests cover fail-closed variants for marker-only selected records,
  TLS, GOT-required globals, missing object byte range, missing emitted bytes,
  missing zero-fill, missing relocation, conflicting relocation, invalid
  pre-prepared initializer semantics, and the existing missing-label and
  producer-incoherent diagnostics.
- Representative `tests/c/external/gcc_torture/src/20000412-1.c` remains
  fail-closed with
  `unsupported_global_data: prepared selected object-data contract status=unsupported_but_coherent object_label_id=2 object_size_bytes=1656 emitted_byte_count=0 zero_fill_byte_count=0`;
  this is expected for Step 3 because the prepared facts still lack selected
  byte, zero-fill, or relocation authority.

## Suggested Next

Execute Step 4: emit prepared object data only for records admitted by the new
guard boundary, using prepared object labels, extents, section authority,
emitted bytes, zero-fill counts, and relocation facts when those facts exist.

## Watchouts

- Do not generalize from `src/20000412-1.c`, object label `2`, object size
  `1656`, raw global spelling, C source shape, or testcase identity.
- Do not treat `PreparedGlobalObjectData::section_kind`'s default `Data` value
  as section authority for an `unsupported_but_coherent` marker-only record.
- Current relocation support for selected symbol-pointer globals remains the
  pre-existing narrow fallback from raw BIR initializer shape; Step 4 should
  not broaden relocation emission unless prepared relocation records exist.
- Thread-local storage, GOT-required globals, missing static-local object
  labels, parameter homes, unrelated global access widths, and F128 stay out
  of this RV64 consumer slice.
- Keep `unsupported_but_coherent` marker-only records fail-closed unless the
  prepared producer publishes explicit bytes, zero-fill, section, or relocation
  authority for the selected object-data record.

## Proof

- `scripts/plan_review_state.py set-step --step-id 3 --step-title 'Implement Contract Admission And Fail-Closed Guards'`
  passed.
- `cmake --build build --target c4cll` passed.
- `cmake --build build --target backend_riscv_object_emission_test` passed.
- `ctest --test-dir build --output-on-failure -R '^backend_riscv_object_emission$'`
  passed.
- Focused representative probe:
  `build/c4cll -I /workspaces/c4c --codegen obj --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000412-1.c -o build/agent_state/510_step3_object_data_admission/20000412-1.o`
  failed closed with the expected marker-only selected object-data diagnostic.
- `git diff --check -- src/backend/mir/riscv/codegen/object_emission.cpp tests/backend/mir/backend_riscv_object_emission_test.cpp todo.md`
  passed.
- `ctest --test-dir build -j --output-on-failure -R '^backend_'` passed and
  wrote canonical proof log `test_after.log`.
