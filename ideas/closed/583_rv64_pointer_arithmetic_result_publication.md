# RV64 Pointer Arithmetic Result Publication

Status: Closed
Type: Focused RV64 object-emission repair
Parent: `ideas/closed/575_rv64_pointer_arithmetic_lowering.md`
Owning Layer: RV64 object lowering for pointer-valued binary arithmetic

## Goal

Teach the RV64 object route to materialize and publish pointer-valued BIR
add/sub results when a prepared pointer base is combined with an integer byte
offset, without relying on testcase-specific fallbacks.

## Why This Exists

Idea 575 closed on the narrower fail-closed diagnostic path rather than a
capability repair. The representative `src/20000819-1.c` route still stopped at
the same pointer arithmetic owner, but reported `unsupported_pointer_arithmetic`
instead of the old generic `unsupported_instruction_fragment`.

The retained evidence from 575 showed this was a late RV64 object-emission gap:
prepared BIR already exposed the pointer-valued binary instruction, its loaded
pointer base, the scaled integer byte offset, and the pointer result owner. The
missing tail was RV64 materialization plus publication of that pointer result
so later memory uses could consume the prepared owner.

## In Scope

- RV64 object emission for pointer-valued BIR add/sub where a prepared pointer
  base is combined with an integer byte offset.
- Publication of the resulting pointer value into the prepared destination
  home expected by later memory operations.
- Focused object-emission coverage for loaded-base plus scaled-offset pointer
  arithmetic, including frame/register destination homes where current
  evidence justified them.
- Representative route proof showing at least one retained pointer arithmetic
  row advances past the old `unsupported_pointer_arithmetic` owner or exposes a
  later, narrower owner.
- Precise fail-closed diagnostics for unsupported pointer arithmetic forms
  whose operand types, homes, or prepared facts are not yet semantically
  lowerable.

## Out Of Scope

- Reconstructing missing address provenance from raw source text, LIR, or
  target-shaped instruction fragments.
- Broad local-memory redesign beyond consuming the prepared pointer arithmetic
  facts already exposed to RV64 object emission.
- Select, call, inline asm, floating-point, variadic helper, or F128 work.
- Integer ALU expansion unrelated to pointer-result address arithmetic.
- Expectation rewrites, unsupported-marker changes, allowlist edits, or
  runtime-comparison changes as a substitute for capability repair.

## Acceptance Criteria

- Focused RV64 object-emission tests prove pointer-result add/sub
  materialization from a pointer base plus integer byte offset without matching
  a representative filename, function, block, value name, or diagnostic string.
- The implementation publishes the pointer result to the prepared destination
  home; later consumers must not observe the same missing pointer-owner failure
  through a renamed helper path.
- Unsupported pointer arithmetic shapes remain fail-closed with a narrower
  pointer-arithmetic diagnostic.
- A representative `src/20000819-1.c` route rerun no longer stops at the old
  `unsupported_pointer_arithmetic` owner for the loaded-base plus scaled-offset
  pointer add shape, or records a distinct downstream owner with concrete
  route evidence.
- Backend validation for the touched RV64 object-emission bucket passes.

## Closure Notes

Closed after RV64 object emission learned to materialize supported pointer
add/sub results from a prepared pointer base plus integer byte offset and
publish the pointer result into the prepared destination register or stack
home. Focused object-emission coverage now proves add/sub publication and
preserves fail-closed diagnostics for missing result homes.

The retained `src/20000819-1.c` representative advanced beyond the old
compile-time owner:
`function=foo`, `block=entry`, `instruction_index=7`, `owner=ptr %t4`. It now
builds the C4C object and linked binary, then fails downstream with
`[RV64_BACKEND_RUNTIME_MISMATCH]`, `clang_exit=0`, and
`c4c_exit=Subprocess aborted`.

The runtime mismatch is tracked separately in
`ideas/open/584_rv64_20000819_runtime_mismatch_after_pointer_publication.md`.

Close proof: Step 5 backend validation passed with
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`.
CTest reported `100% tests passed, 0 tests failed out of 346`.

## Reviewer Reject Signals

- Reject filename-, function-, block-, value-name-, or exact-source-shape
  matching for `src/20000819-1.c` or its `foo` representative.
- Reject raw diagnostic-string matching or unsupported-label rewrites claimed
  as pointer arithmetic progress.
- Reject a fix that materializes an address but fails to publish the pointer
  result into the prepared destination home required by later memory uses.
- Reject broad local-memory, select, call, or integer ALU rewrites that do not
  prove the pointer-valued add/sub publication contract.
- Reject a route that leaves loaded-base plus scaled-offset pointer arithmetic
  on the same `unsupported_pointer_arithmetic` owner behind a renamed helper.
- Reject expectation downgrades, unsupported-marker edits, allowlist changes,
  or weaker test contracts without explicit user approval.
