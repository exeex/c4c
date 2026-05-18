# AArch64 Backend Non-Leaf Call-Frame LR Preservation

Status: Open
Created: 2026-05-18
Origin: Split from ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md

## Intent

Repair the AArch64 backend's non-leaf function return path so functions that
emit calls preserve and restore the link register before returning.

## Why This Exists

The umbrella AArch64 c-testsuite inventory found 23 timeout cases whose
generated assembly contains `bl` calls and whose inspected generated functions
return without saving/restoring `x30`. That shape can cause a non-leaf function
to return to its own post-call address repeatedly instead of to its caller.

The cleanest initial evidence cases are:

- `tests/c/external/c-testsuite/src/00100.c`
- `tests/c/external/c-testsuite/src/00116.c`
- `tests/c/external/c-testsuite/src/00121.c`

These avoid stdio and isolate ordinary user-function calls before broader
timeout cases introduce printf, loop/control-flow, aggregate, pointer, static,
or goto defects.

## In Scope

- Identify the AArch64 frame/prologue/epilogue and call-lowering surfaces that
  decide whether a function is non-leaf.
- Emit ABI-consistent `x30` preservation for functions that can execute a
  call, using the existing backend frame model rather than testcase-specific
  assembly patches.
- Keep stack alignment and return lowering consistent with AAPCS64.
- Add or update focused backend tests that prove non-leaf functions preserve
  link-register state.
- Use `00100.c`, `00116.c`, and `00121.c` as runtime probes after the semantic
  frame repair is visible in assembly.

## Out of Scope

- Fixing string-literal addressing, variadic calls, printf setup, loop
  predicates, short-circuit control flow, aggregate/pointer lowering, static
  globals, or goto behavior.
- Reclassifying all AArch64 c-testsuite failures as part of the first repair
  packet.
- Weakening c-testsuite expected outputs, allowlists, unsupported
  classifications, CTest contracts, runner behavior, or timeout handling.
- Matching exact c-testsuite filenames, source shapes, or emitted text snippets
  instead of repairing the AArch64 non-leaf frame rule.

## Acceptance Criteria

- A focused unit or MIR/backend test proves that a function containing a call
  saves and restores `x30` or otherwise preserves its return address under the
  backend's chosen frame strategy.
- The generated assembly for `00100.c`, `00116.c`, and `00121.c` no longer has
  the old non-leaf `bl` plus bare `ret` failure shape.
- The narrow runtime proof for `00100.c`, `00116.c`, and `00121.c` no longer
  times out due to link-register clobbering. Any remaining failure must be a
  different, recorded semantic owner.
- The fix is expressed through the AArch64 backend's frame/call/return model,
  not by special-casing tests or function names.

## Reviewer Reject Signals

Reject the route if it:

- adds c-testsuite filename checks, function-name checks, or source-shape
  shortcuts to alter only `00100.c`, `00116.c`, `00121.c`, or the known timeout
  list;
- claims progress by changing expected outputs, allowlists, unsupported
  classifications, CTest registration, timeout values, or runner behavior;
- only rewrites assembly spelling while retaining a non-leaf function path
  where `x30` can be clobbered by `bl` before `ret`;
- broadens into unrelated AArch64 runtime repairs such as printf, string
  literals, loop predicates, aggregate lowering, or globals before the LR
  preservation rule is proven;
- proves only a named c-testsuite case while a nearby hand-written non-leaf
  backend test still lacks return-address preservation.
