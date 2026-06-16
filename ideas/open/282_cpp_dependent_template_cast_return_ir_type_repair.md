# 282 C++ dependent template cast return IR type repair

## Goal

Repair C++ template materialization / HIR-to-LIR lowering so dependent
`static_cast<T>(value)` return expressions produce valid LLVM IR after `T` and
parameter types are substituted.

## Why This Exists

Full `ctest` currently fails
`cpp_positive_sema_template_default_args_cpp`. The C4CLL frontend emits IR for
`convert<long, short>(short val)` with an `i16` parameter but returns it as
`i32` without an extension:

```llvm
define i32 @convert_l_s(i16 %p.val)
  ret i32 %p.val
```

`--dump-hir` shows the materialized function as `convert_l_s(val: short) -> ?`
and the return expression as `((?)val#P0)`, so the dependent cast target or
return type is not fully resolved before LIR return coercion.

## In Scope

- Inspect template substitution for dependent cast target types and function
  return types.
- Ensure materialized `CastExpr` and `ReturnStmt` lowering carry enough type
  information to emit the required `sext`/`zext`/`trunc` or other scalar cast.
- Fix the general dependent-template cast/return path, not only
  `template_default_args.cpp`.
- Preserve existing non-template casts, default template arguments, NTTP
  materialization, and runtime positive-case behavior.

## Out of Scope

- Do not weaken the positive-case runtime harness.
- Do not rewrite unrelated template instantiation machinery unless the direct
  type-substitution boundary requires it.
- Do not paper over invalid IR in the printer.
- Do not fold in reference alias C-style cast repair, C aggregate call ABI, or
  AArch64 fp128/vararg crash work.

## Acceptance Criteria

- `ctest --test-dir build_debug -R '^cpp_positive_sema_template_default_args_cpp$' --output-on-failure`
  passes.
- The generated IR for `convert<long, short>` contains a valid scalar
  conversion before returning a widened value.
- Nearby dependent template cast/default argument tests still pass.
- No expectation downgrade, skipped runtime mode, or harness weakening is used
  as progress.

## Reviewer Reject Signals

- Reject testcase-shaped special cases for `template_default_args.cpp` or
  `convert_l_s`.
- Reject changes that only modify expected output or test mode.
- Reject IR-printer string hacks that hide unresolved HIR/template type state.
- Reject broad rewrites that destabilize unrelated template materialization.
- Reject any route that leaves dependent cast target types rendered as `?`
  while claiming the repair is complete.
