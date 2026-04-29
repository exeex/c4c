# Parser Rendered Record Template Lookup Mirror Cleanup

Status: Closed
Created: 2026-04-29
Closed: 2026-04-29
Reopened: 2026-04-29
Reclosed: 2026-04-29

Parent Ideas:
- [129_parser_intermediate_carrier_boundary_labeling.md](/workspaces/c4c/ideas/closed/129_parser_intermediate_carrier_boundary_labeling.md)
- [131_cross_ir_string_authority_audit_and_followup_queue.md](/workspaces/c4c/ideas/closed/131_cross_ir_string_authority_audit_and_followup_queue.md)

## Goal

Remove semantic dependence on rendered record and template lookup mirror strings
inside parser-side record/template definition and instantiation paths.

## Why This Idea Exists

The parser now has structured identity carriers for record and template names,
but Step 3 of idea 129 found compatibility tables that still carry rendered
spelling beside those identities. Those strings are acceptable as transitional
mirrors only when they cannot decide semantic lookup or substitution after a
structured identity exists.

Suspicious paths:

- `ParserDefinitionState::defined_struct_tags`
- `ParserDefinitionState::struct_tag_def_map`
- `ParserTemplateState::template_struct_defs`
- `ParserTemplateState::template_struct_specializations`
- `ParserTemplateState::instantiated_template_struct_keys`
- `ParserTemplateState::nttp_default_expr_tokens`

## Scope

- Trace parser record-definition lookup, constant-evaluation, template
  specialization, and template instantiation paths that still consult rendered
  tag/template strings.
- Make `QualifiedNameKey`, `TextId`, parser symbol ids, direct `StructDef*`
  references, or equivalent structured carriers the semantic authority.
- Keep rendered names only as compatibility mirrors, diagnostics, debug output,
  or generated artifact spelling when they no longer decide lookup.
- Add focused tests for nearby same-feature cases, not only the first observed
  failing or suspicious path.

## Out Of Scope

- Redesigning parser template instantiation.
- Removing generated mangled names that are final artifact spelling.
- Bulk deleting compatibility fields before all consumers have structured
  replacements.
- Changing AST/Sema/HIR ingress behavior except where required by the parser
  producer contract.

## Acceptance Criteria

- Parser record/template semantic lookup no longer depends on rendered tag or
  template spelling when a structured identity is available.
- Compatibility mirrors are documented or named as non-authoritative, or are
  removed after all semantic users are gone.
- Mismatch counters or fallback diagnostics prove any remaining string path is
  compatibility-only.
- Focused parser/frontend tests cover namespace-qualified records, template
  specializations, and NTTP default-expression paths that previously crossed
  this boundary through rendered spelling.

## Closure Summary

Closed after the active runbook completed all five steps. Parser record
definition lookup, template primary/specialization lookup, template
instantiation, and NTTP default-expression handling now use structured identity
as the primary semantic path when available.

Remaining rendered record/template uses were audited as helper internals with
visible mismatch/fallback accounting, compatibility mirror registration/cache
writes, explicit rendered compatibility fallbacks after structured lookup, final
artifact/generated spelling, eval_const_int support APIs, or focused tests. No
unclassified semantic lookup path was found where rendered spelling silently
wins after structured identity is available.

Close proof originally included the canonical matching `frontend_parser_tests`
before/after guard with equal pass counts allowed, supplemental
`frontend_cxx_` coverage, and a final broad `ctest --test-dir build -j
--output-on-failure` run with all 3089 executed tests passing and 12 tests
disabled by CTest.

## Reopen Note

Reopened after baseline review of close commit `0e39decb` exposed a rebuilt
test failure that the earlier final broad CTest likely missed through stale
binaries:

`frontend_lir_function_signature_type_ref` fails because
`LirFunction.signature_return_type_ref` for function `declared_pair` names a
different structured return type than `%struct.Big`.

Repair work should treat this as an invalid close proof for this idea, not as a
new unrelated initiative. The fix must preserve the no-overfit rule: diagnose
and repair the structured identity propagation/mirror authority path that can
produce the mismatched return type-ref, rather than weakening the rebuilt test,
rewriting expectations, or matching only `declared_pair`.

## Reclose Summary

Reclosed after implementation commit `2c722228` repaired the rebuilt
`frontend_lir_function_signature_type_ref` regression by making LIR function
signature TypeSpec tags module-owned and deriving aggregate type-ref identity
from structured tag spelling rather than rendered mirror text.

Reviewer artifact `review/reviewA.md` found no blocking drift or overfit issue.
The close gate resolved the rebuilt regression under the canonical
`test_before.log`/`test_after.log` guard, and the accepted full baseline in
`test_baseline.log` passed all 3089 executed tests with 12 CTest-disabled tests
not run.
