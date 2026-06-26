# RV64 Object Route 20000112 Runtime Join Publication

Status: Closed
Type: Repair idea
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
Split From: `ideas/closed/378_rv64_object_route_20000112_instruction_fragment_lowering.md`

## Goal

Audit and repair the RV64 object-route runtime mismatch now exposed by
`src/20000112-1.c` after the instruction-fragment compile blocker from idea
378 was repaired.

## Why This Exists

Idea 378 moved `src/20000112-1.c` past the audited ordinary instruction
fragment:

```text
%t8 = bir.zext i32 %t7 to i32
```

The representative now compiles and links, then fails under qemu as:

```text
RV64_BACKEND_RUNTIME_MISMATCH
clang_exit=0 c4c_exit=Segmentation fault
```

The current object-dump evidence points at a distinct join/select publication
owner in `special_format`: skip blocks materialize the short-circuit value in
`t0`, while later join blocks read `s2` before comparing. This needs its own
runtime/value-publication contract rather than expanding the closed
instruction-fragment compile-blocker idea.

## In Scope

- Audit the first incorrect runtime value-publication or join-publication shape
  for `src/20000112-1.c` using prepared CFG, value-home, publication, and
  object-route evidence.
- Identify whether the failure is caused by block-entry publication, edge
  publication, select/short-circuit publication, or join ownership on the RV64
  object route.
- Implement the first semantic supportable repair for the audited publication
  shape.
- Add focused backend tests that prove the admitted publication behavior and
  reject adjacent unsupported or ambiguous publication shapes.
- Rerun `src/20000112-1.c` and record whether it passes or advances to a
  documented distinct next owner.

## Out of Scope

- Reopening idea 369 terminator-fragment lowering.
- Reopening idea 378 same-width `ZExt` instruction-fragment lowering.
- Broad CFG reconstruction, register-allocation replacement, assembler/object
  replacement, globals, strings, byval aggregate homes, aggregate `va_arg`, or
  unrelated call-argument ownership.
- Fixes inferred from testcase names, C source spelling, hard-coded value ids,
  registers, block labels, instruction indexes, or object-dump addresses.

## Acceptance Criteria

- The first runtime publication mismatch for `src/20000112-1.c` is documented
  from prepared/object-route evidence.
- Focused backend tests prove any admitted join or edge publication repair and
  keep neighboring unsupported shapes fail-closed.
- `src/20000112-1.c` is rerun and either passes or advances to a documented
  distinct next owner because of semantic publication repair.
- Existing focused backend and prepared-contract coverage remains green.

## Closure Summary

Closed after the RV64 select/publication repair for the first
`src/20000112-1.c` runtime publication mismatch landed and the representative
advanced to a distinct owner. Step 4 evidence shows the remaining segfault is a
call-argument preservation/reload failure in a later short-circuit RHS call:
the generated c4c binary calls `strchr` with the string argument register
holding NULL, while the clang reference preserves or reloads the incoming
string parameter before each call.

The remaining owner is intentionally handed off to
`ideas/open/380_rv64_object_route_short_circuit_call_argument_reload.md`.

Close-gate evidence: `test_before.log` and `test_after.log` are matching
`ctest --test-dir build -j --output-on-failure -R '^backend_'` runs with
324/324 tests passing in both logs. The monotonic regression guard passed with
`--allow-non-decreasing-passed`.

## Evidence

- `build/rv64_gcc_c_torture_backend/src_20000112-1.c/case.log`
- `build/agent_state/378_step5_20000112.c4c_objdump.log`
- `build/agent_state/378_step5_20000112.qemu_strace.err`
- `build/agent_state/378_step5_20000112.qemu_strace.out`
- `build/agent_state/379_step4_20000112.classification.txt`
- `build/agent_state/379_step4_20000112.c4c_o_objdump.txt`
- `build/agent_state/379_step4_20000112.c4c_bin_objdump.txt`
- `build/agent_state/379_step4_20000112.clang_bin_objdump.txt`
- `build/agent_state/379_step4_20000112.c4c_qemu_L_strace.err`
- `build/agent_state/379_step4_20000112.c4c_qemu_trace.log`
- `build/agent_state/379_step4_20000112.clang_qemu_L_strace.err`
- `test_before.log`: close-gate backend CTest before log.
- `test_after.log`: close-gate backend CTest after log.

## Reviewer Reject Signals

- Reject testcase-name shortcuts for `src/20000112-1.c`, `special_format`, or
  exact C source expression shapes.
- Reject hard-coded value ids, registers such as `t0` or `s2`, block labels,
  instruction indexes, object addresses, or objdump text matching as the basis
  for publication behavior.
- Reject changes that only rewrite diagnostics, allowlists, expected
  classifications, or runtime expectations while preserving the same incorrect
  publication behavior.
- Reject reopening terminator lowering, same-width `ZExt` lowering, globals,
  strings, aggregate ABI, non-register parameter homes, or call arguments
  inside this idea.
- Reject broad CFG/register-allocation rewrites unless the audit proves the
  current prepared publication contract cannot represent the needed semantic
  repair.
