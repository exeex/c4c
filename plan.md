# Ref-Overload Method Link-Name Binding

Status: Active
Source Idea: ideas/open/16_ref_overload_method_link_name_binding.md

## Purpose

Repair the direct-call binding path for resolved ref-qualified member calls so
the selected overload's concrete symbol identity survives into emitted call
metadata and runtime behavior.

## Goal

Make resolved member calls use the exact selected overload link name instead of
re-looking up a generic method key that can collapse `&` and `&&` overloads.

## Core Rule

Do not "fix" this by weakening tests, removing ref-qualified distinctions, or
adding testcase-specific dispatch hacks. The patch must preserve the real
selected overload end-to-end.

## Read First

- [ideas/open/16_ref_overload_method_link_name_binding.md](/workspaces/c4c/ideas/open/16_ref_overload_method_link_name_binding.md)
- [src/frontend/hir/hir_expr_call.cpp](/workspaces/c4c/src/frontend/hir/hir_expr_call.cpp)
- [src/frontend/hir/hir_types.cpp](/workspaces/c4c/src/frontend/hir/hir_types.cpp)

## Current Targets

- member-call lowering in `src/frontend/hir/hir_expr_call.cpp`
- direct-call carrier construction for already-resolved method overloads
- focused runtime-positive C++ regressions covering ref-qualified method calls

## Non-Goals

- parser/declarator fixes
- template specialization field/type fixes
- broad struct-method registry refactors unless this narrow repair proves
  insufficient

## Working Model

- ref-overload resolution already chooses a concrete mangled callee name in the
  affected member-call path
- a later generic `find_struct_method_link_name_id(tag, method, ...)` lookup
  can return the wrong overload's symbol id because the registry key is too
  coarse for ref-overloaded methods
- the narrow fix is to bind direct-call metadata from the already-resolved
  concrete callee name in the affected path

## Execution Rules

- keep the patch inside the smallest lowering surface that preserves the chosen
  overload identity
- do not rewrite unrelated call-lowering paths unless the focused proof shows
  they share the same bug
- validate with `build -> focused ref-qualified runtime subset`

## Steps

### Step 1

Goal: patch resolved member-call lowering to keep the chosen overload link name.

Primary Target:
- [src/frontend/hir/hir_expr_call.cpp](/workspaces/c4c/src/frontend/hir/hir_expr_call.cpp)

Actions:
- inspect the member-call path that already computes `resolved_method`
- replace the generic method-key link lookup with direct carrier binding from
  the concrete resolved callee name
- keep fallback behavior unchanged for non-overloaded or unresolved paths

Completion Check:
- emitted direct calls for resolved `&` member calls no longer point at the
  `__rref` overload

### Step 2

Goal: prove the narrow runtime regressions pass.

Actions:
- rebuild the compiler
- run the focused CTest subset:
  `cpp_positive_sema_c_style_cast_base_ref_qualified_method_cpp|cpp_positive_sema_c_style_cast_ref_qualified_method_cpp|cpp_positive_sema_forwarding_ref_qualified_member_dispatch_cpp|cpp_positive_sema_function_return_temporary_member_call_runtime_cpp|cpp_positive_sema_ref_overload_method_basic_cpp|cpp_positive_sema_ref_overload_method_reads_arg_cpp`

Completion Check:
- the focused subset passes without changing test expectations
