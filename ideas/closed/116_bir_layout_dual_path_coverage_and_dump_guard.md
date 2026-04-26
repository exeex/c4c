# BIR Layout Dual Path Coverage And Dump Guard

Status: Closed
Created: 2026-04-26
Last Updated: 2026-04-26

Parent Ideas:
- [112_lir_backend_legacy_type_surface_readiness_audit.md](/workspaces/c4c/ideas/closed/112_lir_backend_legacy_type_surface_readiness_audit.md)
- [113_backend_struct_decl_layout_table_dual_path.md](/workspaces/c4c/ideas/closed/113_backend_struct_decl_layout_table_dual_path.md)
- [115_hir_to_lir_layout_legacy_decl_demotion.md](/workspaces/c4c/ideas/closed/115_hir_to_lir_layout_legacy_decl_demotion.md)

## Goal

Strengthen the BIR aggregate layout dual path before any legacy type text
removal. Keep legacy `type_decls` available as fallback and parity evidence,
but close the known structured-path holes and add focused BIR dump guards so
later printer and legacy-removal work has a stable proof base.

## Why This Idea Exists

Idea 113 introduced `BackendStructuredLayoutTable` from
`LirModule::struct_decls`, but the current lookup still prefers structured
layout only when legacy parity is checked and matching. If legacy `type_decls`
is removed too early, paths that currently rely on parity gates or fallback
will lose layout authority.

`src/backend/bir/bir_printer.cpp` does not currently query layout tables. It
prints the already-lowered `bir::Module`. That means dump safety depends on
LIR-to-BIR lowering producing complete, stable BIR facts under both legacy and
structured layout routes.

This idea is the conservative bridge: improve dual-path coverage and add dump
tests, but do not switch BIR printer authority and do not remove legacy text.

## Scope

Focus on BIR lowering and tests around:

- `BackendStructuredLayoutTable` lookup behavior
- aggregate size, alignment, and field offset derivation
- nested, packed, empty, array, and pointer-containing aggregate layouts
- globals and aggregate initializers
- local aggregate slots, GEP/addressing, loads, stores, memcpy, and memset
- sret, byval/byref, HFA-like aggregate call classification, variadic
  aggregate arguments, and `va_arg` aggregate paths
- `--dump-bir` tests that observe the semantic BIR output produced by those
  paths

Out of scope:

- Removing `LirModule::type_decls`.
- Removing legacy parsing helpers.
- Reworking `src/backend/bir/bir_printer.cpp` to use a new render context.
- Migrating `src/backend/mir/`; MIR remains planned-rebuild residue.

## Execution Rules

- Preserve legacy fallback and parity notes.
- Add structured-first behavior only where missing coverage is understood and
  behavior-preserving.
- Do not require `parity_checked` as the only route for structured layout when
  a structured-only proof can be added safely.
- If a path still needs legacy body parsing, label it clearly in `todo.md`
  during execution rather than pretending it is ready for removal.
- Treat `--dump-bir` output as a guardrail for the lowered BIR facts, not as a
  printer-authority migration.
- Preserve backend output and existing dump text unless the runbook explicitly
  calls out an intentional diagnostic addition.

## Acceptance Criteria

- Structured aggregate layout can serve selected BIR paths without depending
  on legacy body parsing as the only successful route.
- Legacy `type_decls` remains available as fallback and parity evidence.
- Focused BIR lowering tests cover nested, packed, empty, array, global,
  local-memory, call ABI, variadic, and `va_arg` aggregate cases.
- Focused `--dump-bir` tests cover at least one layout-sensitive aggregate
  path and prove semantic BIR dump output does not drift.
- Remaining fallback-only legacy layout users are explicitly inventoried for
  the later printer or legacy-removal ideas.

## Deferred

- BIR printer structured render context belongs to idea 117.
- Active legacy removal belongs to idea 118.

## Closure

Closed: 2026-04-26

Completed through the active runbook Steps 1-6. Structured aggregate layout now
serves the selected BIR declaration/signature, storage/addressing, call ABI,
variadic, and layout-sensitive dump guard paths while legacy `type_decls`
remains available as fallback and parity evidence.

Remaining fallback-only legacy layout users were inventoried in `todo.md` for
later printer or legacy-removal ideas rather than treated as solved by this
bridge.
