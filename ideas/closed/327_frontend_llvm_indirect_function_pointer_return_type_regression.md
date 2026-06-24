# Frontend LLVM Indirect Function Pointer Return Type Regression

## Completion Note

Closed after the indirect-call lowering repair preserved function-pointer
return metadata through `FnPtrSig::return_type.spec`. Focused regression
coverage now exercises a nested indirect function-pointer-returning call, and
the canonical regression guard resolved `c_testsuite_src_00124_c` with no new
failures in the matching focused scope.

## Goal

Repair the frontend LLVM-route lowering for indirect calls whose callee returns
a function pointer.

The immediate failing case is `c_testsuite_src_00124_c`, where the generated
LLVM IR declares `f1` as returning `ptr` but emits an indirect call through a
local function pointer as returning `i32`, then tries to use that `i32` as a
pointer.

## Why This Exists

After the variadic target-IR work, the full test suite is almost green:

```text
3314 passed
2 failed
```

The remaining failures are:

- the old known `backend_riscv_prepared_edge_publication` baseline failure;
- a new or newly exposed `c_testsuite_src_00124_c` LLVM-route failure.

The focused failing test currently reports:

```text
Test #1712: c_testsuite_src_00124_c ... Failed
[BACKEND_FAIL] /workspaces/c4c/tests/c/external/c-testsuite/src/00124.c
<stdin>:33:23: error: '%t1' defined with type 'i32' but expected 'ptr'
   33 |   %t2 = load i32, ptr %t1
      |                       ^
```

The source shape is:

```c
int f2(int c, int b) { return c - b; }

int (*f1(int a, int b))(int c, int b) {
  if (a != b)
    return f2;
  return 0;
}

int main() {
  int (* (*p)(int a, int b))(int c, int d) = f1;
  return (*(*p)(0, 2))(2, 2);
}
```

A direct LLVM-route probe shows the type inconsistency:

```llvm
define ptr @f1(i32 %p.a, i32 %p.b)
...
define i32 @main()
{
entry:
  %lv.p = alloca ptr, align 8
  store ptr @f1, ptr %lv.p
  %t0 = load ptr, ptr %lv.p
  %t1 = call i32 (i32, i32) %t0(i32 0, i32 2)
  %t2 = load i32, ptr %t1
  %t3 = call i32 %t2(i32 2, i32 2)
  ret i32 %t3
}
```

The first indirect call should preserve the function-pointer return type. It
must not collapse the result to `i32` and then treat it as a pointer.

## In Scope

- Reproduce `c_testsuite_src_00124_c` through the frontend LLVM-route test.
- Identify where indirect-call result typing loses the callee's function
  pointer return type.
- Repair the semantic/LIR/LLVM-route type propagation for indirect calls whose
  result is a function pointer.
- Add focused regression coverage for a function pointer returning another
  function pointer and a chained indirect call.
- Keep the fix semantic and type-driven, not tied to `00124.c`, `f1`, `f2`,
  or the exact local variable name `p`.
- Re-run the focused test and a narrow neighboring frontend/LIR subset.

## Out Of Scope

- Fixing `backend_riscv_prepared_edge_publication`; that is the separate known
  RV64 prepared edge baseline failure.
- Broad variadic ABI work, RV64 prepared variadic support, or external
  `printf` runtime support.
- Reworking all function-pointer lowering paths unless the focused regression
  proves the shared type path is wrong.
- Registering a new broad c-testsuite sweep as the proof for this small fix.
- Weakening `c_testsuite_src_00124_c` or marking it unsupported.

## Suggested Execution Order

1. Capture the current bad LLVM IR for `tests/c/external/c-testsuite/src/00124.c`
   under `build/` as evidence.
2. Inspect the call lowering path for direct and indirect calls, especially the
   point that chooses the LLVM function type and call result type.
3. Add a focused regression test before or with the fix. Prefer a frontend/LIR
   or codegen LLVM-route test that asserts:
   - the first indirect call result is `ptr`;
   - the returned function pointer is invoked as a function pointer;
   - no `load i32, ptr %t1`-style mismatch is emitted.
4. Implement the smallest type-propagation repair needed for indirect call
   results that are function pointers.
5. Prove:
   - `ctest --test-dir build -R '^c_testsuite_src_00124_c$' --output-on-failure`
     passes;
   - the focused regression test passes;
   - a narrow frontend/LIR or c-testsuite subset has no new failures.
6. Leave the full suite state no worse than before. The only tolerated remaining
   full-suite failure should be the pre-existing
   `backend_riscv_prepared_edge_publication`, unless new evidence says
   otherwise.

## Acceptance Criteria

- `c_testsuite_src_00124_c` no longer fails LLVM verification/backend compile
  with `'%t1' defined with type 'i32' but expected 'ptr'`.
- Generated LLVM IR preserves the function-pointer return type for the first
  indirect call in the source shape.
- Focused regression coverage prevents the indirect function-pointer return
  result from collapsing to `i32`.
- The fix is type-driven and applies to neighboring function-pointer-returning
  indirect calls, not just this filename.
- The old known `backend_riscv_prepared_edge_publication` failure is not mixed
  into this route.

## Reviewer Reject Signals

- The patch special-cases `00124.c`, `f1`, `f2`, or a literal local variable
  name instead of repairing indirect-call result typing.
- The generated IR still contains an indirect call with the wrong return type,
  but the test is weakened or rerouted to avoid verification.
- The fix rewrites the source test expectation, marks the c-testsuite case
  unsupported, or hides the failure behind a later runtime mismatch.
- The route expands into RV64 prepared edge publication, variadic ABI, or broad
  function-pointer refactors without evidence that they are required for this
  regression.
- It proves only a direct function pointer call while the nested
  function-pointer-returning-function-pointer shape remains broken.

## Notes For The Agent

- This is currently a frontend LLVM-route type regression, not a qemu/runtime
  bug.
- The strongest first clue is that `define ptr @f1(...)` is correct, but the
  indirect call through `%t0` is emitted as `call i32`.
- Keep proof logs canonical if running broader validation: use
  `test_before.log` and `test_after.log` only at the repo root.
