# HIR Struct Method Member Identity Dual Lookup

Status: Closed
Created: 2026-04-25
Last Updated: 2026-04-25
Closed: 2026-04-25

Parent Ideas:
- [100_hir_compile_time_template_consteval_dual_lookup.md](/workspaces/c4c/ideas/closed/100_hir_compile_time_template_consteval_dual_lookup.md)
- [101_hir_enum_const_int_dual_lookup.md](/workspaces/c4c/ideas/open/101_hir_enum_const_int_dual_lookup.md)

## Goal

Plan and implement the larger HIR structured-identity migration for struct,
method, member, and static-member lookup while preserving rendered names for
codegen, diagnostics, ABI/link output, and legacy parity proof.

## Why This Idea Exists

The remaining HIR string identity problem is concentrated around record/type
ownership:

- `TypeSpec::tag`
- `Module::struct_defs`
- struct layout and declaration order
- base-class and member owner lookup
- method lookup
- static-member declaration and value lookup
- struct template specialization names
- codegen-facing LLVM type names

This surface is intentionally larger than ideas 100 and 101. It should be
handled only after narrower HIR dual lookup slices have proven the migration
pattern, because changing owner identity can affect HIR lowering, templates,
method resolution, layout, and downstream codegen.

## Scope

Inventory and migrate HIR-owned record identity lookup around:

- `Module::struct_defs`
- `struct_def_order`
- `TypeSpec::tag` semantic consumers
- lowerer `struct_def_nodes_`
- `template_struct_defs_`
- `template_struct_specializations_`
- `struct_methods_`
- `struct_method_link_name_ids_`
- `struct_method_ret_types_`
- `struct_static_member_decls_`
- `struct_static_member_const_values_`
- base-class and member lookup paths that use rendered owner names

The implementation should define durable HIR owner keys before demoting any
legacy path. Candidate key components include:

- namespace context ID
- global-qualified bit
- qualifier segment text IDs
- unqualified struct `TextId`
- template argument identity where specialization ownership matters
- concrete struct declaration pointer or declaration ID as a bridge only when
  durable source identity is incomplete

Rendered strings remain required for diagnostics, HIR dumps, mangled names,
template specialization names, and codegen-facing type/link output.

## Out Of Scope

- Big-bang deletion of `TypeSpec::tag`.
- Rewriting codegen type naming before HIR semantic lookup has a stable
  structured owner key.
- Parser or sema rewrites unless the HIR audit proves a missing metadata
  handoff is the blocker.
- Expectation downgrades or testcase-shaped shortcuts.

## Suggested Execution Decomposition

1. Audit every `TypeSpec::tag`, `Module::struct_defs`, method, member, and
   static-member lookup in HIR and downstream codegen handoff.
2. Classify each string use as semantic lookup, codegen spelling, diagnostic
   spelling, bridge-required, or legacy parity proof.
3. Define a HIR record-owner structured key that can coexist with rendered
   names.
4. Add structured mirrors for struct definitions before touching methods or
   members.
5. Add structured mirrors for method/member/static-member lookup by owner key.
6. Keep all legacy maps as fallback and mismatch proof during the migration.
7. Only after parity proof is stable, identify which legacy APIs can be
   demoted or deleted in a follow-up idea.

## Acceptance Criteria

- HIR has a documented record-owner structured key strategy.
- Struct definition lookup can dual-write and dual-read structured owner keys
  beside rendered-name maps where metadata is available.
- Method, member, and static-member lookup can use structured owner identity
  without changing rendered output.
- `TypeSpec::tag` remains available for bridge output until downstream codegen
  and dump consumers have a stable replacement.
- Focused HIR and downstream proof passes without baseline drift, expectation
  downgrades, or testcase-shaped shortcuts.

## Closure Notes

Closed after active runbook Step 6 validation. The structured HIR record-owner
key path, struct definition mirrors, method mirrors, member mirrors, and
static-member mirrors were implemented with legacy rendered-name fallbacks and
parity probes retained.

Close gate: matching HIR regression guard passed using `test_before.log` and
`test_after.log` with 73/73 tests passing before and after. The accepted
full-suite baseline evidence in `test_baseline.log` records 2976/2976 tests
passing at commit `b9d55400bd518d2461f0eb787450d9f699252b18`.

Follow-up work, if authorized separately, is legacy rendered-owner API demotion
or deletion after downstream output spelling requirements have their own stable
replacement. That work is intentionally outside this idea.
