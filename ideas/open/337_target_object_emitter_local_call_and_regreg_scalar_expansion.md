# Target Object Emitter Local Call And RegReg Scalar Expansion

## Goal

Extend target-owned native object emission just enough to unblock the next
object-route scan candidates discovered while resuming idea 334:

- RV64 object runtime for scalar local-backed call arguments:
  `two_arg_local_arg.c`, `two_arg_second_local_arg.c`,
  `two_arg_both_local_arg.c`, and `local_arg_call.c`.
- AArch64 object-byte emission for the no-local scalar call case
  `two_arg_helper.c`, including the selected register-register scalar
  arithmetic or call-result movement needed by that case.

## Why This Exists

Idea 334 resumed from the green scalar object-route baseline after child 336,
but its first resumed scan inspection found that the next obvious object scan
candidates are known-red at target object emission. Adding CTest labels now
would only create expected failures or weaken scan expectations. The target
emitters need focused capability work before the scan/default-readiness
umbrella can broaden again.

## In Scope

- Inspect the rejected prepared RV64 records and selected AArch64 machine
  records for the named scalar candidates.
- Extend RV64 prepared object emission for scalar local homes used as direct
  call arguments and return values in the named no-global cases.
- Extend AArch64 object emission for the instruction family needed by
  `two_arg_helper.c`, expected to be selected register-register scalar add
  and any narrow call-result/register movement needed by that object-byte case.
- Add focused object-emitter tests for the new target-owned capability.
- Restore passing object-route scan tests beside existing asm-route tests:
  RV64 object runtime for the local-backed call-argument cases and AArch64
  object-byte output for `two_arg_helper.c`.
- Preserve the green 28-test scalar baseline from child 336 while adding the
  new cases.

## Out Of Scope

- RV64 globals/data, arrays, pointers, aggregates, broad memory operations, or
  broad control-flow expansion.
- AArch64 local/frame memory expansion beyond the narrow no-local
  `two_arg_helper.c` object-byte need.
- AArch64 object runtime.
- x86 object output.
- object stdout.
- c-testsuite object defaults or default backend route changes.
- object `--backend-bir-stage semantic` mode.
- textual assembly fallback, external assemblers, or expectation downgrades.

## Acceptance And Proof Expectations

- RV64 object emission supports the local-backed scalar call-argument shapes
  semantically enough for the named cases, without matching filenames or
  function names.
- AArch64 object emission supports the selected register-register scalar
  instruction family needed by `two_arg_helper.c`, without asm fallback.
- Focused target object-emitter unit tests cover the new instruction or
  prepared-record support.
- Restored object-route scan cases pass with native object bytes:
  `backend_obj_runtime_rv64_two_arg_local_arg`,
  `backend_obj_runtime_rv64_two_arg_second_local_arg`,
  `backend_obj_runtime_rv64_two_arg_both_local_arg`,
  `backend_obj_runtime_rv64_local_arg_call`, and
  `backend_cli_aarch64_two_arg_helper_writes_elf_obj`.
- Existing asm-route counterparts and the previous object-route scalar cases
  remain green.
- Final handoff records that idea 334 can resume scan/default-readiness work
  from the expanded green baseline.

## Reviewer Reject Signals

- The implementation matches testcase names, filenames, or exact function names
  instead of supporting the target-owned prepared or selected instruction
  shapes.
- The route claims progress by adding expected-failure labels, downgrading
  diagnostics, weakening existing object or asm tests, or removing scan cases.
- Object output is produced by textual assembly, an external assembler, or an
  asm fallback instead of target-native object emitters.
- Broad AArch64 frame/local-memory support, RV64 arrays/pointers/aggregates, or
  c-testsuite/default-route policy changes are mixed into this focused child.
- The same target object-emission rejection remains behind a renamed helper or
  broader abstraction.
- The final proof omits the restored scan cases or does not preserve the
  previous green object/asm baseline.
