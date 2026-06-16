Status: Active
Source Idea Path: ideas/open/282_cpp_dependent_template_cast_return_ir_type_repair.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate The Unresolved Type Boundary

# Current Packet

## Just Finished

Completed Step 1 investigation. The unresolved type boundary is in HIR
materialization, before LIR/LLVM return lowering: `lower_function()` creates
`convert_l_i` / `convert_l_s` with substituted parameter types but an unresolved
`Function.return_type`, and `lower_cast_expr()` records the dependent
`static_cast<T>` target as `CastExpr.to_type == ?`. `--dump-hir` shows
`convert_l_s(val: short) -> ?` and `return ((?)val#P0)`; `--codegen llvm` then
maps that unresolved return to `i32` and emits `ret i32 %p.val` for an `i16`
parameter.

## Suggested Next

Execute Step 2 by repairing the HIR template-substitution boundary for
dependent callable signatures and cast targets. Start at
`src/frontend/hir/hir_functions.cpp` around `prepare_callable_return_type()` /
`substitute_signature_template_type()` and
`src/frontend/hir/impl/expr/scalar_control.cpp::lower_cast_expr()`: the return
path currently substitutes with only the legacy `TypeBindings`, while the
parameter path has the stronger `append_explicit_callable_param()` /
`apply_signature_template_binding_by_text()` context using structured and
TextId binding mirrors. Make return type and `CastExpr.to_type` substitution use
the same structured/TextId-aware binding authority as parameters, then confirm
`convert_l_s` prints `-> long` and `((long)val#P0)`.

## Watchouts

- Fix the general dependent-template cast/return path, not a named testcase.
- Do not weaken the positive-case runtime harness, expected output, or test
  mode.
- Do not hide unresolved `?` type state in the IR printer.
- Do not fold in reference alias C-style cast repair, C aggregate function
  pointer ABI work, or AArch64 fp128/vararg crash triage.
- `get_value<T, N>` already resolves its dependent C-style cast and return
  type, while `convert<T, U = int>` resolves `U` for the parameter but not `T`
  for the return or `static_cast<T>` target. Preserve the working NTTP/default
  materialization behavior while generalizing the substitution boundary.
- LIR return coercion is not the first failing boundary for this packet:
  `src/codegen/lir/hir_to_lir/stmt.cpp` consumes the already-unresolved
  `ctx.fn->return_type.spec`, so fixing LIR alone would paper over bad HIR.

## Proof

Ran `cmake --build --preset default` successfully; Ninja reported no work.

Ran `ctest --test-dir build_debug -R '^cpp_positive_sema_template_default_args_cpp$' --output-on-failure 2>&1 | tee test_after.log`.
The target CTest failed as expected for this investigation packet with
`[BACKEND_FAIL]` and LLVM rejecting `ret i32 %p.val` because `%p.val` is `i16`.
`test_after.log` is preserved.

Additional dumps examined:
`./build_debug/c4cll --dump-hir-summary tests/cpp/internal/postive_case/runtime/template_default_args.cpp`,
`./build_debug/c4cll --dump-hir tests/cpp/internal/postive_case/runtime/template_default_args.cpp`,
`./build_debug/c4cll --codegen llvm tests/cpp/internal/postive_case/runtime/template_default_args.cpp`,
and `./build_debug/c4cll --parse-only tests/cpp/internal/postive_case/runtime/template_default_args.cpp`.
