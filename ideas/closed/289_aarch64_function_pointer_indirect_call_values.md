# AArch64 Function Pointer Indirect Call Values

Status: Closed
Created: 2026-05-18
Source Inventory: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md
Split From: ideas/closed/288_aarch64_stack_frame_sp_alignment.md

## Intent

Repair the AArch64 backend path that materializes, stores, loads, returns, and
calls function-pointer values for indirect `blr` call sites.

## Why This Exists

After AArch64 stack-frame/SP alignment repair, the former bus-error cluster no
longer points at misaligned stack frames. Step 4 wider sampling found four
remaining bus-error cases with aligned generated frames (`sub sp, sp, #32` or
`sub sp, sp, #48`) and a different crash shape: bad or missing
function-pointer values before indirect `blr`.

Representative cases:

- `src/00087.c`: local struct field stores `foo` into `v.fptr`, then calls
  `v.fptr()`.
- `src/00089.c`: global struct initializer stores `&zero`; nested function
  pointer returns lead to `go()()->zerofunc()`.
- `src/00124.c`: function returns a function pointer, then caller invokes the
  returned value.
- `src/00210.c`: attributed function pointer casts through `void *` and calls
  the recovered function pointer.

This is backend function-pointer value semantics and indirect-call lowering
work. It is not residual SP/frame alignment.

## In Scope

- AArch64 backend lowering/emission for function symbol addresses when used as
  first-class pointer values.
- Function-pointer values stored to and loaded from local stack slots and
  aggregate fields.
- Global/static aggregate initialization containing function-pointer values.
- Function returns whose value is a function pointer.
- Indirect call setup where the callee operand must be materialized into the
  register used by `blr`.
- Focused proof using `src/00087.c`, `src/00089.c`, `src/00124.c`, and
  `src/00210.c`.

## Out of Scope

- Stack-frame/SP alignment; remaining frames in this family are already
  aligned.
- Runner behavior, expected outputs, allowlists, unsupported classifications,
  timeout policy, or CTest registration.
- Parser or sema changes.
- Named-case shortcuts for the four c-testsuite files, exact symbol names,
  exact aggregate field names, exact function names, or emitted instruction
  text.
- Broad aggregate ABI, struct layout rewrites, variadic ABI work, libc
  behavior, `_Generic`, compare/branch printer/lowering, and timeout-sensitive
  cases unless later evidence proves a direct dependency on this function
  pointer owner.

## Acceptance Criteria

- `src/00087.c` proves a local aggregate field can carry a function-pointer
  value into an indirect call.
- `src/00089.c` proves global/static aggregate initialization and nested
  function-pointer returns can carry a valid function-pointer value into an
  indirect call, or any unrelated remaining blocker is documented.
- `src/00124.c` proves a function returning a function pointer can feed an
  indirect call, or any unrelated remaining blocker is documented.
- `src/00210.c` proves attributed function pointer casts through `void *` do
  not lose the callable value, or any frontend/attribute-only blocker is
  documented separately.
- No progress is claimed through test expectation rewrites, allowlist changes,
  unsupported classifications, runner changes, timeout changes, or CTest
  contract changes.

## Closure Summary

Closed on 2026-05-18 after Step 5 closure review.

The function-pointer owner is complete:

- `src/00087.c` passes for local function-pointer value/callee materialization.
- `src/00089.c` passes for global/static function-pointer initialization.
- `src/00124.c` is separated into
  `ideas/open/290_aarch64_scalar_parameter_alu_authority.md`; generated code
  already returns and indirectly calls the selected function pointer, and the
  remaining failure is stale scalar parameter/ALU authority.
- `src/00210.c` is separated into
  `ideas/open/291_aarch64_call_argument_register_authority.md`; attributed
  function-pointer calls remain valid indirect calls, and the remaining
  mismatch is the `printf` format-pointer call-argument register move.

Close validation used a matching four-case close-scope regression guard in
temporary logs because root proof logs were intentionally left untouched:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before /tmp/c4c_fnptr_close_before.log --after /tmp/c4c_fnptr_close_after.log --allow-non-decreasing-passed
```

Result: passed with 2 passing and 2 separated non-owner failures in both logs.

## Reviewer Reject Signals

Reject the route if it:

- fixes only one c-testsuite filename, one function name such as `foo` or
  `zero`, one aggregate field name, or one emitted `blr` instruction shape
  instead of repairing semantic function-pointer value lowering;
- treats the remaining failures as SP/frame-alignment defects despite aligned
  generated frames;
- special-cases direct calls while leaving first-class function-pointer stores,
  loads, returns, or indirect-call callees broken;
- claims backend progress through expected-output rewrites, allowlist changes,
  unsupported classifications, runner changes, timeout changes, or CTest
  contract changes;
- broadens into parser/sema work, broad aggregate ABI, unrelated struct layout
  rewrites, libc behavior, compare/branch lowering, or timeout-case repair
  before the four focused function-pointer representatives are proven or
  cleanly separated;
- renames helpers or reorganizes emission while preserving the old failure mode
  where an indirect `blr` receives a bad or missing function-pointer value.
