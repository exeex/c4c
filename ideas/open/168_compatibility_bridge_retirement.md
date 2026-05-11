# Compatibility Bridge Retirement

Status: Open
Created: 2026-05-11

Depends On:
- `ideas/closed/167_whole_codebase_string_authority_final_audit.md`

Parent Ideas:
- `ideas/closed/158_sema_validate_string_authority_audit.md`
- `ideas/closed/159_sema_consteval_domain_table_authority.md`
- `ideas/closed/160_sema_canonical_symbol_template_key_authority.md`
- `ideas/closed/161_hir_template_binding_domain_key_authority.md`
- `ideas/closed/162_link_name_id_backend_symbol_authority.md`

## Goal

Retire or narrow compatibility bridges that the final audit proves are no
longer needed as ordinary code paths.

The project intentionally retained many rendered-string maps, overloads, and
fallbacks while parser/sema/HIR/backend domains were migrated to `TextId`,
structured keys, and `LinkNameId`. After those structured paths are stable, the
remaining compatibility layer should shrink.

## Why This Idea Exists

Compatibility bridges are useful while migrations are in flight, but they can
become a permanent second authority if left broad. Once idea 167 identifies
which bridges are still live, this idea converts the safe subset from
"fallback available everywhere" into one of:

- deleted
- restricted to tests or raw-input import
- guarded by explicit no-metadata checks
- documented as display/diagnostic-only

## In Scope

- Use the idea 167 audit inventory as the source of truth.
- Remove compatibility overloads that no production caller needs.
- Narrow rendered maps to construction/display storage when structured lookup
  is complete.
- Replace broad fallback helpers with explicit `*_legacy`,
  `*_compatibility`, or no-metadata APIs where the bridge must remain.
- Add assertions or tests proving covered structured misses do not fall through
  to retired rendered paths.
- Update comments and docs so retained bridges name their owner, limitation,
  and removal condition.

## Out Of Scope

- Discovering new unclassified string-authority areas; that belongs in idea
  167 or a new follow-up.
- Rewriting route-local id domains.
- Removing user-facing diagnostics, final output spelling, or debug dumps.
- Testcase expectation weakening.

## Acceptance Criteria

- Retired bridges are removed from production lookup paths.
- Remaining bridges have explicit no-metadata or compatibility names.
- Tests prove at least one formerly broad fallback now fails closed on complete
  structured metadata misses.
- No output/diagnostic strings are removed just to reduce grep count.
- Follow-up ideas exist for bridges that cannot be retired safely.

## Reviewer Reject Signals

- A rendered fallback remains reachable after a complete structured miss
  without an explicit compatibility boundary.
- The route deletes display strings while leaving semantic lookup unchanged.
- A bridge is removed by weakening tests or removing supported behavior.
- The implementation is based on grep count rather than the idea 167
  classification.
