# Current Packet

Status: Active
Source Idea Path: ideas/open/338_aarch64_object_emitter_local_frame_and_scalar_frontier.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add AArch64 Selected Scalar Multiply Object Support

## Just Finished

- Completed Step 3 by adding semantic AArch64 selected scalar multiply object
  support for the isolated c-testsuite `00012.c` shape.
- `src/backend/mir/aarch64/codegen/object_emission.cpp` now emits selected GPR
  register-register `Mul` records and selected one-register-plus-small-immediate
  `Mul` records by materializing the immediate into an available reserved MIR
  scratch GPR before the native `mul` instruction.
- Existing selected add/sub immediate and register-register paths remain in the
  same scalar object fragment helper; immediate forms are still limited to the
  selected add/sub/mul subset.
- Added focused AArch64 object-emitter unit coverage for a selected
  `MOV/ADD/MOV/MUL/SUB/RET` scalar sequence matching the record family behind
  c-testsuite `00012.c`.
- Restored the selectable AArch64 object-byte smoke
  `backend_cli_aarch64_cts_00012_writes_elf_obj` with the existing c-testsuite
  object/CLI/dual-route/smoke/scan label style.

## Suggested Next

- Step 4 should run a focused child validation/handoff pass that includes the
  restored AArch64 c-testsuite object smokes through `00012.c`, the AArch64
  object-emitter unit, and the parent 334 object-route baseline selectors that
  should remain green.
- Suggested Step 4 owned file: `todo.md` only unless validation exposes a
  blocker inside this child.

## Watchouts

- Do not add expected-failure scan labels or weaken object/asm expectations.
- Keep AArch64 runtime, broad branch/control-flow, globals/data, pointers,
  aggregates, byval, indirect calls, RV64, x86, object stdout, c-testsuite
  defaults, and semantic-BIR object mode out of this child.
- `00003.c`, `param_slot.c`, and the `two_arg_*_rewrite.c` family still need
  saved-callee/local-call-frame expansion beyond the leaf frame-slot memory
  slice completed here.
- Selected scalar multiply for the isolated `00012.c` shape is now supported;
  broader branch/control-flow and branch/global c-testsuite families are still
  outside this child.
- Parent 334 should resume only after this child repairs the AArch64 target
  object frontier or records a precise blocker.

## Proof

- Ran the delegated proof command:
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_object_emission|backend_cli_aarch64_(cts_00001|cts_00002|cts_00011|cts_00012|return_zero|return_add|return_add_sub_chain|two_arg_helper)_writes_elf_obj|backend_cli_aarch64_asm_external_return_add_sub_chain_smoke)$' --output-on-failure) > test_after.log 2>&1`
- Result: passed, 10/10 tests.
- Proof log: `test_after.log`.
