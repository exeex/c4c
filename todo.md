Status: Active
Source Idea Path: ideas/open/282_cpp_dependent_template_cast_return_ir_type_repair.md
Source Plan Path: plan.md
Current Step ID: 3-5
Current Step Title: Prove Scalar Return Coercion, Focused Acceptance Proof, and Closeout Notes

# Current Packet

## Just Finished

Completed Steps 3-5 closeout evidence after the dependent cast/return
substitution repair. The current target HIR materializes
`fn convert_l_s(val: short) -> long` with `return ((long)val#P0)`, and LLVM
materializes `define i64 @convert_l_s(i16 %p.val)` followed by
`%t0 = sext i16 %p.val to i64` and `ret i64 %t0`. No invalid `ret i32 %p.val`
or equivalent return source/destination mismatch remains in the target IR.

## Suggested Next

Supervisor can hand this runbook to the plan owner for lifecycle disposition,
or request broader validation if a milestone-level proof is desired before
closure.

## Watchouts

- The repaired boundary is still the shared dependent-template signature/cast
  substitution path, not backend return lowering or an IR-printer workaround.
- No tests, expectations, runtime harnesses, or backend return lowering were
  weakened for this evidence-only packet.
- No reference alias, C aggregate ABI, or AArch64 fp128/vararg scope was folded
  into this runbook.

## Proof

Ran the delegated acceptance proof and preserved its output in `test_after.log`:
`cmake --build build_debug && ctest --test-dir build_debug -R '^cpp_positive_sema_template_default_args_cpp$' --output-on-failure 2>&1 | tee test_after.log`.
The build reported no work to do and the target CTest passed, 1/1 tests.

Also inspected current generated dumps for source-idea acceptance evidence:
`./build_debug/c4cll --dump-hir tests/cpp/internal/postive_case/runtime/template_default_args.cpp`
shows `fn convert_l_s(val: short) -> long` and `return ((long)val#P0)`;
`./build_debug/c4cll --codegen llvm tests/cpp/internal/postive_case/runtime/template_default_args.cpp`
shows `define i64 @convert_l_s(i16 %p.val)`, `%t0 = sext i16 %p.val to i64`,
and `ret i64 %t0`, with no `ret i32 %p.val` mismatch for the target
specialization.
