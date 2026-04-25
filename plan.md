# HIR Safe Legacy Lookup Demotion Runbook

Status: Active
Source Idea: ideas/open/104_hir_safe_legacy_lookup_demotion.md
Activated from: ideas/open/104_hir_safe_legacy_lookup_demotion.md
Depends on: ideas/closed/103_hir_post_dual_path_legacy_readiness_audit.md
Primary Review Artifact: review/103_hir_post_dual_path_legacy_readiness_audit.md

## Purpose

Demote only the HIR-internal rendered-name lookup paths that idea 103 classified
as `safe-to-demote` or `legacy-proof-only`, while leaving bridge-required,
diagnostic, printer, ABI/link, and HIR-to-LIR paths intact.

## Goal

Make structured HIR lookup the primary authority for safe internal call sites,
with focused parity proof and without downstream LIR/codegen bridge redesign.

## Core Rule

Do not delete or weaken a legacy path unless the idea 103 review artifact
classifies the exact path as safe for demotion, proof-only, or ready after the
local parity proof in this runbook.

## Read First

- `ideas/open/104_hir_safe_legacy_lookup_demotion.md`
- `ideas/closed/103_hir_post_dual_path_legacy_readiness_audit.md`
- `review/103_hir_post_dual_path_legacy_readiness_audit.md`
- `src/frontend/hir/hir_ir.hpp`
- `src/frontend/hir/impl/lowerer.hpp`
- HIR implementation files named by the selected review-artifact rows

## Current Scope

- HIR-internal `ModuleDeclLookupKey` lookup from complete `DeclRef` metadata.
- Concrete `FunctionId`, `GlobalId`, and `LinkNameId` lookup surfaces.
- Owner-aware HIR record lookup by `HirRecordOwnerKey` where the owner key is
  already available.
- Direct HIR method/member probes that can be routed through existing
  parity-recording helper APIs before any authority flip.
- Parity/debug counters and dumps needed to prove the migration.

## Non-Goals

- Do not redesign HIR-to-LIR, LIR, shared codegen, or backend string identity.
- Do not remove `fn_index`, `global_index`, `struct_defs`, `TypeSpec::tag`,
  template name registries, enum/const-int name maps, method/member rendered
  maps, or ABI/link spelling as an early cleanup packet.
- Do not rewrite diagnostics, dumps, HIR printers, mangling text helpers, or
  final textual emission unless adjacent code uses the same string as semantic
  lookup authority.
- Do not downgrade tests, expectations, or supported-path contracts to claim
  demotion progress.

## Working Model

- `safe-to-demote` means structured lookup can become primary when the call
  site already carries complete structured identity.
- `legacy-proof-only` means legacy lookup may remain as parity observation, but
  it should not be the semantic authority once proof is green.
- `bridge-required`, `blocked-by-hir-to-lir`, `diagnostic-only`,
  `printer-only`, and `abi-link-spelling` are not idea 104 deletion targets.
- HIR-to-LIR bridge cleanup belongs to idea 105 after this runbook lands enough
  HIR-only progress.

## Execution Rules

- Start each code packet by quoting the relevant classification row from
  `review/103_hir_post_dual_path_legacy_readiness_audit.md` in `todo.md`.
- Prefer structured-first helper calls over direct rendered-map probes when the
  existing data already includes `DeclRef`, `LinkNameId`, `HirRecordOwnerKey`,
  member IDs, or text IDs.
- Preserve legacy fallback when metadata is incomplete or when parity proof
  still needs mismatch observation.
- Keep packet size narrow enough to validate with focused HIR tests before any
  broader regression guard.
- Escalate to plan review instead of expanding into LIR/codegen bridge scope.

## Step 1: Select The First Safe HIR-Only Demotion Packet

Goal: choose one narrow, review-backed target that stays entirely inside HIR.

Primary target:
- `review/103_hir_post_dual_path_legacy_readiness_audit.md`, especially the
  recommended idea 104 scope and final classification summary.

Actions:
- Re-read the review rows for `safe-to-demote` and `legacy-proof-only` paths.
- Pick one first packet from the current HIR-only scope.
- Confirm the selected call sites do not depend on HIR-to-LIR rendered fallback,
  ABI/link spelling, diagnostics, printer output, or raw `TypeSpec::tag`.
- Record the selected packet, owned files, and proof command in `todo.md`.

Completion check:
- `todo.md` names one selected HIR-only packet, the review classification that
  authorizes it, and a focused proof command.

## Step 2: Prefer Structured Decl Lookup Where Metadata Is Complete

Goal: demote HIR-internal function/global decl resolution that already carries
complete structured identity.

Primary targets:
- `Module::find_function_by_decl_ref_structured`
- `Module::find_global_by_decl_ref_structured`
- `Module::classify_function_decl_lookup`
- `Module::classify_global_decl_lookup`
- HIR call sites that already hold complete `DeclRef`, `FunctionId`,
  `GlobalId`, or `LinkNameId`

Actions:
- Replace direct rendered-name authority with structured-first lookup only when
  metadata is complete.
- Keep legacy fallback or parity observation for incomplete metadata and
  mismatch reporting.
- Preserve dump and diagnostic text as observation, not lookup authority.

Completion check:
- Focused HIR proof covers qualified refs, namespaced refs, overload-sensitive
  calls, template-instantiated refs, and extern-visible refs touched by the
  packet.

## Step 3: Route Direct Method And Member Probes Through Helpers

Goal: remove bypasses around existing parity-recording helper APIs before
flipping owner-key authority.

Primary targets:
- Direct probes of `struct_methods_`, `struct_method_link_name_ids_`,
  `struct_method_ret_types_`, `struct_static_member_decls_`, and
  `struct_static_member_const_values_`
- Existing helper APIs such as `find_struct_method_mangled`,
  `find_struct_method_return_type`, `find_struct_static_member_decl`, and
  `find_struct_static_member_const_value`

Actions:
- Convert direct HIR probes to helper calls where inputs are already available.
- Preserve rendered fallback and parity counters in helper implementations.
- Do not alter trait/static evaluation or downstream codegen assumptions.

Completion check:
- Focused HIR proof covers inherited members, const and non-const methods,
  range-for methods, operator overloads, object members, and static members
  touched by the packet.

## Step 4: Use Owner-Aware Record Lookup Where Owner Keys Exist

Goal: make `HirRecordOwnerKey` lookup primary only for HIR paths that already
carry owner identity.

Primary targets:
- `Module::find_struct_def_by_owner_structured`
- owner-aware HIR record lookup call sites

Actions:
- Prefer `find_struct_def_by_owner_structured` where a valid owner key is
  already present.
- Keep raw `TypeSpec::tag`, `Module::struct_defs`, layout, base traversal, and
  codegen consumers untouched unless the selected call site is HIR-only and
  owner-aware.
- Preserve rendered tag spelling for diagnostics, dumps, and later LIR/codegen
  bridge work.

Completion check:
- Focused HIR proof covers every owner-aware record lookup path touched by the
  packet, with no change to emitted type spelling.

## Step 5: Preserve Or Retire Parity Proof Deliberately

Goal: keep migration observability until proof is strong enough to retire a
specific legacy authority path.

Primary targets:
- decl lookup parity records and dumps
- owner lookup parity counters
- helper mismatch recording

Actions:
- Keep parity instrumentation when a packet only reroutes callers or flips one
  helper to structured-first authority.
- Retire parity-only legacy authority only after the relevant focused proof is
  green and the review classification supports removal.
- Document any retained fallback as `bridge-required`, `needs-more-parity-proof`,
  or `legacy-proof-only` in `todo.md`.

Completion check:
- The final packet summary separates removed authority, retained fallback, and
  retained proof-only instrumentation.

## Step 6: Final Validation And Handoff To Idea 105

Goal: close the HIR-only demotion runbook without absorbing downstream bridge
work.

Actions:
- Run the supervisor-selected focused HIR proof for the last packet.
- Run a broader regression guard if any authority flip spans multiple HIR
  families or if several narrow packets have accumulated.
- Leave HIR-to-LIR string identity, `TypeSpec::tag` codegen consumers, extern
  filtering, global best-object selection, and layout/ABI handoff notes for
  idea 105.

Completion check:
- Safe HIR-internal demotions are complete or explicitly blocked, all retained
  legacy paths have classifications, and idea 105 remains the downstream bridge
  follow-up.
