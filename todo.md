# Current Packet

Status: Active
Source Idea Path: ideas/open/336_target_object_emitter_scalar_scan_expansion.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add AArch64 Scalar Object Instruction Support

## Just Finished

- Step 3 added focused AArch64 object-emitter support for the same-module
  scalar call shape in `return_add.c`.
- `src/backend/mir/aarch64/codegen/object_emission.cpp` now encodes selected
  GPR register-to-register call-boundary moves and the narrow fixed-size
  non-leaf frame setup/teardown shape used by `return_add.c`: stack adjust
  with `sp`, link-register save/restore at a prepared stack offset, no dynamic
  stack, no frame pointer, and no additional callee-save records.
- Existing MOVZ and ADD/SUB immediate scalar object support remains intact, and
  direct-call relocation handling is reused for the same-module `add_three`
  call.
- `tests/backend/mir/backend_aarch64_object_emission_test.cpp` now covers the
  selected `add_three`/`main` object byte layout directly, including the BL
  relocation offset and defined symbol sizes.
- `tests/backend/CMakeLists.txt` restored
  `backend_cli_aarch64_return_add_writes_elf_obj` as a selectable AArch64
  object-byte scan case beside `return_add_sub_chain`.

## Suggested Next

- If the supervisor wants more Step 3 target-emitter capability before scan
  expansion resumes, choose the next scalar/no-global AArch64 case and inspect
  its first selected instruction rejection. Otherwise, hand back to idea 334's
  object-route scan expansion with AArch64 `return_add.c` and
  `return_add_sub_chain.c` now available as object-byte scan cases.

## Watchouts

- Do not use textual assembly, external assemblers, or asm fallback to satisfy
  `--codegen obj`.
- Do not add expected-failure scan labels for the blocked scalar cases.
- Keep broad RV64 globals/data, x86 object output, c-testsuite defaults, object
  stdout, and object semantic-BIR mode out of this focused child.
- AArch64 support added in this packet is intentionally limited to selected
  GPR moves and a fixed-size link-register frame shape. It does not cover
  general load/store memory records, dynamic stack frames, frame-pointer
  frames, extra callee-saves, negative constants, MOVK/MOVN sequences,
  register-register ALU, globals/data, or runtime execution.

## Proof

- Passed:
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_object_emission|backend_cli_aarch64_return_add_writes_elf_obj|backend_cli_aarch64_return_add_sub_chain_writes_elf_obj|backend_cli_aarch64_asm_external_return_add_smoke)$' --output-on-failure) > test_after.log 2>&1`
- Result: 4/4 tests passed.
- Proof log: `test_after.log`.
- Passed: `git diff --check`.
