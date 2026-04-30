# HIR Legacy String Lookup Metadata Resweep

Status: Open
Created: 2026-04-30

Parent Ideas:
- [124_hir_legacy_string_lookup_removal_convergence.md](/workspaces/c4c/ideas/closed/124_hir_legacy_string_lookup_removal_convergence.md)
- [136_hir_structured_record_template_lookup_authority_cleanup.md](/workspaces/c4c/ideas/closed/136_hir_structured_record_template_lookup_authority_cleanup.md)
- [139_parser_sema_legacy_string_lookup_resweep.md](/workspaces/c4c/ideas/open/139_parser_sema_legacy_string_lookup_resweep.md)

## Goal

Run a second HIR cleanup pass for rendered-string lookup authority and metadata
propagation after the first HIR convergence and structured owner/template
cleanup work.

HIR should use declaration ids, `TextId`, namespace qualifiers,
`ModuleDeclLookupKey`, `HirRecordOwnerKey`, `LinkNameId`, and structured
record/template carriers as lookup authority where available. Rendered strings
should remain diagnostics, dumps, final spelling, mangling payload, or explicit
compatibility fallback only.

## Why This Idea Exists

The previous HIR pass closed with classified rendered-name surfaces and repaired
structured static-member behavior. A second frontend-to-backend sweep should
verify that HIR no longer drops parser/Sema metadata and later rediscovers
identity from rendered names.

This idea is the HIR leg of the second 123-to-126 style cleanup sequence.

## Scope

- Re-inventory `src/frontend/hir` for `std::string` keyed lookup maps,
  rendered-name fallback paths, and parity-only structured mirrors.
- Trace parser/Sema metadata through HIR lowering for records, methods,
  templates, static members, consteval, globals, and function declarations.
- Convert or demote HIR lookup paths where structured keys already exist.
- Preserve rendered names for diagnostics, dumps, mangling, link-visible final
  spelling, and explicit compatibility input.
- Add tests where HIR receives both structured metadata and drifted rendered
  spelling and must prefer the structured carrier.
- Split missing upstream parser/Sema metadata or downstream LIR/BIR carrier
  needs into separate `ideas/open/` files.

## Out Of Scope

- Parser/Sema implementation cleanup except as a blocker recorded in a new idea.
- LIR/BIR/backend implementation cleanup except as a downstream metadata bridge
  follow-up.
- Removing final link names, dumps, diagnostics, or ABI-visible text.
- Named-case shortcuts or unsupported expectation downgrades.

## Acceptance Criteria

- Covered HIR semantic lookup paths are structured-primary.
- HIR no longer relies on rendered strings to recover metadata that is already
  present in parser/Sema or HIR carriers.
- Remaining HIR string surfaces are classified as display, diagnostics, dump,
  final spelling, compatibility, local scratch, or unresolved metadata boundary.
- Cross-module metadata blockers are represented as open ideas.

