# Current Packet

Status: Active
Source Idea Path: ideas/open/336_target_object_emitter_scalar_scan_expansion.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add AArch64 Scalar Object Instruction Support

## Just Finished

- Step 3 added minimal AArch64 object-emitter support for selected scalar
  immediate object instructions needed by `return_add_sub_chain.c`.
- `src/backend/mir/aarch64/codegen/object_emission.cpp` now encodes selected
  immediate GPR call-boundary moves as MOVZ and selected integer add/sub
  scalar records with an immediate RHS as AArch64 ADD/SUB immediate
  instructions. It supports W/X GPR views and handles an immediate LHS by
  materializing it into the result register before the ADD/SUB in the same
  object fragment.
- Unsupported scalar shapes still fail closed with object-emission diagnostics;
  there is no textual assembly, external assembler, asm fallback, filename
  shortcut, or broad frame/call support in this packet.
- `tests/backend/mir/backend_aarch64_object_emission_test.cpp` now covers the
  selected MOVZ/ADD/SUB/RET object byte sequence directly, with no
  relocations.
- `tests/backend/CMakeLists.txt` restored
  `backend_cli_aarch64_return_add_sub_chain_writes_elf_obj` as a selectable
  AArch64 object-byte scan case beside the existing return-zero object route
  and asm add-sub-chain smoke.

## Suggested Next

- Continue Step 3 with the next focused AArch64 scalar object gap only if the
  supervisor wants more target-emitter capability before returning to scan
  expansion; the known next candidate is the frame/call-heavy `return_add.c`,
  which needs a separate packet because it is not just immediate add/sub
  encoding.

## Watchouts

- Do not use textual assembly, external assemblers, or asm fallback to satisfy
  `--codegen obj`.
- Do not add expected-failure scan labels for the blocked scalar cases.
- Keep broad RV64 globals/data, x86 object output, c-testsuite defaults, object
  stdout, and object semantic-BIR mode out of this focused child.
- AArch64 support added here is intentionally limited to single-MOVZ
  nonnegative immediates and ADD/SUB immediate encodings. It does not cover
  negative constants, MOVK/MOVN sequences, register-register ALU, memory/frame
  instructions, calls, prologue/epilogue, or `return_add.c`.

## Proof

- Passed:
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_object_emission|backend_cli_aarch64_return_zero_writes_elf_obj|backend_cli_aarch64_return_add_sub_chain_writes_elf_obj|backend_cli_aarch64_asm_external_return_add_sub_chain_smoke)$' --output-on-failure) > test_after.log 2>&1`
- Result: 4/4 tests passed.
- Proof log: `test_after.log`.
- Passed: `git diff --check`.
