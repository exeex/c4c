# AArch64 Object Emitter Local Frame And Scalar Frontier

## Goal

Expand AArch64 target-owned native object emission for the next object-route
scan frontier discovered by parent idea 334, without changing CLI behavior,
object defaults, or c-testsuite default routing.

The immediate target is to unblock balanced c-testsuite/backend object evidence
after the current 37/37 object smoke baseline:

- c-testsuite `src/00003.c`
- c-testsuite `src/00011.c`
- backend `param_slot.c` or one `two_arg_*_rewrite.c` scalar local rewrite
  case
- c-testsuite `src/00012.c` for selected scalar multiply, if it can be handled
  as a focused instruction-family extension

## Why This Exists

Parent idea 334 triaged the next scan failures after adding c-testsuite object
smokes for `00001.c` and `00002.c`. The failures are target object-emitter
capability gaps, not scan harness, CLI, object writer, linker/toolchain, or
runtime failures.

AArch64 blocks the next shared c-testsuite frontier on selected local
frame-memory records and one selected scalar multiply family. RV64 has some
additional green candidates, but default-readiness needs balanced target
evidence and should not proceed based on RV64-only growth.

## In Scope

- Inspect the selected AArch64 machine records emitted for:
  - `tests/c/external/c-testsuite/src/00003.c`
  - `tests/c/external/c-testsuite/src/00011.c`
  - `tests/backend/case/param_slot.c`
  - one representative `tests/backend/case/two_arg_*_rewrite.c` case if it
    shares the same local/frame-memory shape
  - `tests/c/external/c-testsuite/src/00012.c`
- Extend AArch64 object emission for the narrow selected fixed-frame local
  memory shapes needed by those no-global scalar cases.
- Extend AArch64 object emission for selected scalar multiply if `00012.c`
  requires only the `mul` instruction family and no broader lowering.
- Add focused AArch64 object-emitter unit tests for new encodings or selected
  frame/local-memory records.
- Restore passing object-byte scan registrations beside existing asm/object
  coverage for the accepted c-testsuite/backend cases.
- Preserve the existing 37/37 object-route smoke baseline from parent 334.

## Out Of Scope

- AArch64 runtime execution.
- Broad AArch64 frame/local-memory framework beyond the selected no-global
  scalar cases above.
- AArch64 branch/control-flow object support.
- AArch64 globals/data, aggregates, pointers, byval, indirect calls, or broad
  memory lowering.
- RV64 prepared object-shape expansion.
- x86 object output.
- object stdout.
- c-testsuite object defaults or default backend route changes.
- object `--backend-bir-stage semantic` mode.
- Textual assembly fallback, external assemblers, or expectation downgrades.

## Acceptance And Proof Expectations

- AArch64 object emission supports the selected local frame-memory object
  records needed by the accepted scalar no-global cases without matching
  filenames or function names.
- If included, selected scalar multiply support is instruction-family based and
  covers the machine record shape used by c-testsuite `00012.c`.
- Focused object-emitter unit tests cover new encodings and selected
  local-frame memory fragments.
- Added scan tests pass through native object bytes and preserve existing
  object/asm smoke coverage.
- Parent idea 334 can resume from an expanded green baseline and decide whether
  more target-emitter children are needed before default-readiness.

## Reviewer Reject Signals

- The implementation matches testcase names, source filenames, or exact
  function names instead of supporting selected AArch64 machine record shapes.
- The route claims progress by adding expected-failure labels, weakening object
  or asm expectations, deleting scan cases, or changing diagnostics only.
- Object output is produced through textual assembly, an external assembler, or
  asm fallback instead of target-native object emitters.
- Broad AArch64 branch/global/pointer/aggregate/runtime work, RV64 object
  support, x86 object output, object stdout, or c-testsuite default changes are
  mixed into this focused child.
- The same selected fixed-frame/local-memory or multiply rejection remains
  behind renamed helpers or a broader abstraction.
- The final proof omits the new object-byte scan cases or does not preserve the
  existing 37/37 object-route smoke baseline.

## Closure Note

Closed after completing the focused AArch64 object frontier and handing the
expanded baseline back to parent idea 334.

- Added AArch64 selected leaf fixed-frame stack adjustment object support
  without link-register preservation.
- Added selected frame-slot scalar load/store object support for prepared
  base-plus-offset frame-slot memory, including immediate stores through the
  reserved MIR scratch GPR.
- Restored `backend_cli_aarch64_cts_00011_writes_elf_obj`.
- Added selected GPR scalar multiply object support for register-register and
  small-immediate materialization through the reserved scratch GPR.
- Restored `backend_cli_aarch64_cts_00012_writes_elf_obj`.
- Final expanded proof:
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_object_model_records|backend_riscv_object_emission|backend_aarch64_object_emission|backend_cli_.*obj|backend_obj_runtime_.*|backend_rv64_runtime_(return_zero|return_add|two_arg_helper|two_arg_local_arg|two_arg_both_local_arg|two_arg_second_local_arg|local_arg_call|return_add_sub_chain|local_temp)|backend_cli_aarch64_asm_external_return_(zero|add|add_sub_chain)_smoke|backend_codegen_route_riscv64_external_no_storage_main_emits_return_path)$' --output-on-failure) > test_after.log 2>&1`
- Result: 39/39 tests passed. Close-time regression guard passed against the
  rolled 39/39 baseline.

Remaining object-route/default-readiness boundaries belong back in parent idea
334: saved-callee/local-call-frame expansion for `00003.c`, `param_slot.c`,
and `two_arg_*_rewrite.c`; branch/control-flow and branch/global c-testsuite
families; AArch64 runtime; globals/data; pointers; aggregates; byval;
indirect calls; RV64; x86; object stdout; c-testsuite defaults; and
semantic-BIR object mode.
