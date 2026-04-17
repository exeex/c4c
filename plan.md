# Remaining C++ Positive Cast And Specialization Member Fixes

Status: Active
Source Idea: ideas/open/17_remaining_cpp_positive_cast_and_specialization_member_fixes.md
Supersedes: ideas/open/16_ref_overload_method_link_name_binding.md

## Purpose

Finish the remaining two positive-case failures by repairing one bounded
cast/reference lowering bug and one bounded specialized-member owner/type bug.

## Goal

Make the function-pointer cast testcase and the template-struct specialization
member testcase both execute with the intended semantics.

## Core Rule

Do not downgrade expectations or add testcase-shaped shortcuts. Each fix must
restore the underlying lowering/owner recovery behavior.

## Read First

- [ideas/open/17_remaining_cpp_positive_cast_and_specialization_member_fixes.md](/workspaces/c4c/ideas/open/17_remaining_cpp_positive_cast_and_specialization_member_fixes.md)
- [src/frontend/hir/hir_expr_operator.cpp](/workspaces/c4c/src/frontend/hir/hir_expr_operator.cpp)
- [src/frontend/hir/hir_stmt_decl.cpp](/workspaces/c4c/src/frontend/hir/hir_stmt_decl.cpp)
- [src/frontend/hir/hir_templates_value_args.cpp](/workspaces/c4c/src/frontend/hir/hir_templates_value_args.cpp)

## Current Targets

- `try_lower_rvalue_ref_storage_addr` for cast/reference local-init handling
- member-expression owner/type recovery for realized template struct instances
- focused runtime-positive proof for the remaining two failures

## Non-Goals

- ref-overload method dispatch fixes already proven in the previous slice
- unrelated parser cleanup
- template-system refactors beyond what these two failing paths require

## Working Model

- the cast testcase regresses because rvalue-reference storage-address lowering
  unwraps a cast that should remain a casted callable/pointer value
- the specialization testcase regresses because member owner/type recovery can
  overwrite an already-realized struct tag or trust a stale AST field type
  instead of the realized HIR struct field contract

## Execution Rules

- keep each repair local to the failing lowering surface
- preserve already-realized struct tags before attempting template-family
  recovery
- validate with `build -> focused two-test subset`

## Steps

### Step 1

Goal: keep callable/function-pointer cast initializers from collapsing into
address-of-operand artifacts.

Primary Targets:
- [src/frontend/hir/hir_expr_operator.cpp](/workspaces/c4c/src/frontend/hir/hir_expr_operator.cpp)
- [src/frontend/hir/hir_stmt_decl.cpp](/workspaces/c4c/src/frontend/hir/hir_stmt_decl.cpp)

Actions:
- narrow the `NK_CAST` fast path in `try_lower_rvalue_ref_storage_addr`
- let callable/function-pointer casts lower as values instead of unwrapping to
  the raw operand storage address

Completion Check:
- the cast testcase no longer lowers `raw` into `&raw` for the local init path

### Step 2

Goal: preserve realized specialization owner/type information for member
expressions.

Primary Targets:
- [src/frontend/hir/hir_templates_value_args.cpp](/workspaces/c4c/src/frontend/hir/hir_templates_value_args.cpp)
- [src/frontend/hir/hir_expr_operator.cpp](/workspaces/c4c/src/frontend/hir/hir_expr_operator.cpp)

Actions:
- short-circuit owner recovery when the current tag already names a realized
  struct in `module_->struct_defs`
- when a member owner is resolved, use the realized field type from that owner
  instead of blindly trusting stale AST member types

Completion Check:
- `sizeof(a.value)` / `sizeof(b.value)` use the specialized field contract

### Step 3

Goal: prove the remaining targeted regressions pass.

Actions:
- rebuild the compiler
- run:
  `ctest --test-dir build --output-on-failure -R 'cpp_positive_sema_(c_style_cast_template_fn_ptr_rvalue_ref_return_type_parse|template_struct_specialization_runtime)_cpp'`

Completion Check:
- both tests pass without test changes
