Status: Active
Source Idea Path: ideas/open/282_cpp_dependent_template_cast_return_ir_type_repair.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Dependent Cast And Return Substitution

# Current Packet

## Just Finished

Completed Step 2 repair. `substitute_signature_template_type()` now accepts the
structured and `TextId` binding mirrors already used by parameter lowering, and
both callable return preparation and `lower_cast_expr()` pass those mirrors when
resolving dependent template type carriers. Callable lowering now initializes
the function/method binding context before return-type preparation, so
materialized `convert_l_i` / `convert_l_s` HIR carries `-> long` and
`return ((long)val#P0)` instead of unresolved `?`.

## Suggested Next

Supervisor should decide whether this Step 2 slice is sufficient to advance the
plan or whether to run broader regression coverage before treating the now-green
target LLVM/CTest proof as Step 3/4 evidence.

## Watchouts

- The delegated proof command builds the `default` preset under `build/` but
  runs `./build_debug/c4cll`; `build_debug` had to be rebuilt separately before
  the requested `build_debug` dump and CTest reflected this patch.
- A compatibility overload keeps direct test callers of
  `prepare_callable_return_type()` on the old signature.
- No test expectations, runtime harnesses, or backend return lowering were
  changed.

## Proof

Ran the delegated proof shape and preserved its output in `test_after.log`:
`( set -o pipefail; cmake --build --preset default && ./build_debug/c4cll --dump-hir tests/cpp/internal/postive_case/runtime/template_default_args.cpp && ./build_debug/c4cll --codegen llvm tests/cpp/internal/postive_case/runtime/template_default_args.cpp ) > test_after.log 2>&1`.

Because that command does not rebuild `build_debug/c4cll`, also ran
`cmake --build build_debug` before the final proof refresh. The final HIR dump
shows `fn convert_l_s(val: short) -> long` and `return ((long)val#P0)`, and the
LLVM output emits `define i64 @convert_l_s(i16 %p.val)` with `sext i16` before
`ret i64`.

Ran `ctest --test-dir build_debug -R '^cpp_positive_sema_template_default_args_cpp$' --output-on-failure`;
the subset passed, 1/1 tests.

Supervisor validation also ran
`cmake --build build_debug && ctest --test-dir build_debug -R
'^(cpp_hir_template_function_signature_binding|cpp_hir_template_method_signature_binding|cpp_hir_expr_scalar_control_helper|cpp_hir_template_function_deduction_binding|cpp_hir_template_function_pack_signature_binding|cpp_hir_template_function_recursive_body_binding|cpp_positive_sema_template_default_args_cpp|cpp_positive_sema_template_type_context_nttp_parse_cpp|cpp_positive_sema_template_typedef_nttp_variants_parse_cpp)$'
--output-on-failure`;
the focused HIR/template subset passed, 9/9 tests.
