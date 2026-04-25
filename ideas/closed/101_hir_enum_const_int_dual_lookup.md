# HIR Enum Const-Int Dual Lookup

Status: Closed
Created: 2026-04-25
Last Updated: 2026-04-25

Parent Ideas:
- [100_hir_compile_time_template_consteval_dual_lookup.md](/workspaces/c4c/ideas/closed/100_hir_compile_time_template_consteval_dual_lookup.md)

## Goal

Add structured dual lookup for HIR enum constants and const-int bindings while
preserving existing string-keyed maps as compatibility and parity-check paths.

## Why This Idea Exists

HIR still carries several value-like semantic lookup maps keyed by rendered
strings, especially around enum constants, const-int bindings, consteval state,
and lowering-time constant propagation. These are smaller than the struct/type
identity problem, but broader than the module function/global mirror from idea
99 because they cross normal lowering and compile-time evaluation.

This idea isolates that value-binding surface so it can be migrated with the
same dual-write / dual-read strategy before any legacy lookup is removed.

## Scope

Audit and update HIR-owned enum and const-int lookup paths around:

- lowerer enum constant registration and lookup
- lowerer const-int binding registration and lookup
- compile-time engine enum constant and const-int binding mirrors
- consteval environment handoff where HIR-owned state is queried by name
- switch/case, constant expression, and template value argument paths that
  depend on these bindings

The structured key should use source identity when available, such as:

- namespace context ID
- qualifier segment text IDs
- unqualified `TextId`
- owner type identity when the constant is member-scoped
- stable AST declaration pointer or declaration ID as a temporary bridge when
  the durable structured owner key is not available yet

## Out Of Scope

- Removing legacy string maps.
- Migrating template or consteval definition registries already covered by idea
  100.
- Migrating struct layout, `TypeSpec::tag`, `Module::struct_defs`, method
  lookup, or static-member lookup.
- Changing constant folding semantics.
- Parser or sema rewrites.

## Suggested Execution Decomposition

1. Inventory enum constant and const-int binding maps across lowerer and
   compile-time engine state.
2. Classify each string map as semantic lookup, diagnostic spelling, legacy
   parity path, or bridge-required.
3. Add structured key types for source-identifiable enum/const-int bindings.
4. Dual-write registrations beside existing string maps.
5. Dual-read lookups with legacy fallback and mismatch observation.
6. Preserve string spellings for diagnostics and debug output.
7. Prove switch/case, consteval, template value argument, and normal constant
   expression behavior remain stable.

## Acceptance Criteria

- HIR enum constant and const-int semantic lookup paths have structured mirrors
  wherever source identity is available.
- Registration and lookup paths dual-write and dual-read without behavior
  changes.
- Missing structured metadata remains a compatibility fallback, not a reason
  to remove existing behavior.
- String maps remain only as bridge, diagnostic, compatibility, or parity proof
  paths.
- Focused HIR tests pass without expectation downgrades or testcase-shaped
  shortcuts.

## Closure Summary

Closed: 2026-04-25

Implemented structured `CompileTimeValueBindingKey` mirrors for global enum
constants and global const-int bindings, dual-write registration overloads,
lowerer and compile-time-engine handoff into `ConstEvalEnv`, and structured map
debug stats while preserving legacy rendered-name maps as compatibility,
diagnostic, and selected semantic-result paths.

Intentional fallback remains for string-only APIs, owner/member-scoped
constants without durable owner identity, and constant-expression or NTTP paths
that only carry rendered names. No new metadata blocker was found during the
close runbook; durable owner/member identity is outside this idea's scope.

Close-gate proof used matching canonical logs:
`test_before.log` and `test_after.log` both report 1106/1106 selected tests
passing for `^(frontend_hir_tests|frontend_hir_lookup_tests|cpp_)`, and the
regression guard passed with non-decreasing pass-count comparison.
