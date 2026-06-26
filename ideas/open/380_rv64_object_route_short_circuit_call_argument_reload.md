# RV64 Object Route Short-Circuit Call Argument Reload

Status: Open
Type: Repair idea
Parent: `ideas/open/379_rv64_object_route_20000112_runtime_join_publication.md`

## Goal

Repair RV64 object-route call-argument preservation or reload across
short-circuit RHS calls, as exposed by the remaining `src/20000112-1.c`
runtime segfault after the select/publication repair from idea 379.

## Why This Exists

Idea 379 repaired the first select/publication mismatch for
`src/20000112-1.c`. The representative then advanced to a distinct runtime
owner: inside `special_format`, the generated c4c binary enters a later
short-circuit RHS and calls `strchr` with the string argument register holding
NULL. The clang reference preserves or reloads the incoming string parameter
before each `strchr` call.

This is not the select/publication owner repaired by idea 379. It needs its own
call-argument liveness/reload contract on the RV64 object route.

## In Scope

- Audit how RV64 object lowering models incoming call arguments used by
  multiple short-circuit RHS calls.
- Identify where the original string argument home is lost, clobbered, or not
  reloaded before later calls.
- Implement a semantic preservation/reload rule based on prepared call,
  argument, and value-home facts.
- Add focused backend coverage for repeated RHS calls that require reusing an
  incoming pointer argument after an earlier call clobbers argument registers.
- Rerun `src/20000112-1.c` and record whether it passes or advances to another
  distinct owner.

## Out of Scope

- Reopening idea 379 select/publication lowering once its repaired behavior
  remains green.
- Testcase-name handling for `src/20000112-1.c` or `special_format`.
- Broad register-allocation replacement, full ABI redesign, assembler
  replacement, byval aggregate homes, aggregate `va_arg`, globals, or strings
  unless the call-argument audit proves they are the direct owner.
- Weakening runtime expectations, external allowlists, or supported-path
  contracts.

## Acceptance Criteria

- The first call-argument preservation/reload failure is documented from
  prepared/object-route and qemu evidence.
- Focused backend tests prove the admitted repeated-call argument behavior and
  keep adjacent unsupported or ambiguous shapes fail-closed.
- `src/20000112-1.c` is rerun and either passes or advances to a documented
  distinct next owner after the call-argument repair.
- Existing focused backend coverage for idea 379's select/publication repair
  remains green.

## Starting Evidence

- `build/rv64_gcc_c_torture_backend/src_20000112-1.c/case.log`
- `build/agent_state/379_step4_20000112.classification.txt`
- `build/agent_state/379_step4_20000112.c4c_o_objdump.txt`
- `build/agent_state/379_step4_20000112.c4c_bin_objdump.txt`
- `build/agent_state/379_step4_20000112.clang_bin_objdump.txt`
- `build/agent_state/379_step4_20000112.c4c_qemu_L_strace.err`
- `build/agent_state/379_step4_20000112.c4c_qemu_trace.log`
- `build/agent_state/379_step4_20000112.clang_qemu_L_strace.err`

## Reviewer Reject Signals

- Reject testcase-shaped shortcuts for `src/20000112-1.c`, `special_format`, a
  specific `strchr` call, exact C expression spelling, block labels, value ids,
  instruction indexes, registers, or object addresses.
- Reject fixes that only rewrite diagnostics, expected classifications,
  allowlists, or runtime contracts while preserving the NULL call-argument
  fault.
- Reject claiming progress from helper renames, comment updates, or
  classification-only changes without a semantic call-argument preservation or
  reload repair.
- Reject reopening idea 379 select/publication lowering as this idea's primary
  route unless new evidence proves the repaired publication rule regressed.
- Reject broad register-allocation or ABI rewrites unless the audit proves the
  existing prepared call/value-home contract cannot express the required
  repair.
