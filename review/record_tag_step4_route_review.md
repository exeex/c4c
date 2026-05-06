# Record Tag Step 4 Route Review

Active source idea path: `ideas/open/145_move_record_tag_authority_from_parser_to_sema.md`

Chosen base commit: `a6581efa7` (`[plan] Activate record tag authority migration plan`)

Why this base: `plan.md` clearly links `ideas/open/145_move_record_tag_authority_from_parser_to_sema.md`, and `a6581efa7` is the lifecycle activation commit for that active runbook. The prompt explicitly asks for the diff from the active plan activation point through `HEAD`; later commits are execution/doc packets, not a new activation checkpoint.

Commit count since base: 7

Reviewed diff: `git diff a6581efa7..HEAD`

## Findings

### High: `resolve_record_type_spec` reintroduces TextId-only parser-map authority for context-defaulted structured carriers

Evidence:
- `src/frontend/parser/impl/support.cpp:540`
- `src/frontend/parser/impl/support.cpp:547`
- `src/frontend/parser/impl/support.cpp:620`
- `tests/frontend/frontend_parser_tests.cpp:6717`

`unique_structured_record_candidate_for_text_id` accepts a `TypeSpec` with `tag_text_id`, `namespace_context_id == 0`, no global qualifier, and no qualifier segments, then scans `struct_tag_def_map` for one completed record with the same `TextId`. If exactly one candidate exists, it returns that parser record even when the candidate has a different namespace context.

The new test `test_parser_record_layout_const_eval_accepts_unique_structured_record_match` codifies that behavior by resolving a context-defaulted carrier in context `0` to a record with `namespace_context_id = 3` because it is the only same-`TextId` candidate in the parser map.

That is not rendered-string authority under a renamed helper, but it is still not the source idea's Sema-owned record-domain identity. The source idea explicitly says `TextId` is spelling identity only and must not become record identity. Uniqueness in the current parser compatibility map is not the same as a Sema record-domain key, and it can make the selected record depend on what else has been parsed into `struct_tag_def_map`.

This looks semantically general as an ambiguity guard, but the accepted success condition is still a parser-map/TextId decision. It should either be narrowed to full structured context matches, routed through a Sema/HIR record key, or kept as an explicitly temporary compatibility hole in `todo.md` with a follow-up packet that deletes it.

### Medium: full structured metadata is not consistently checked when namespace context is present

Evidence:
- `src/frontend/parser/impl/support.cpp:515`
- `src/frontend/parser/impl/support.cpp:522`
- `tests/frontend/frontend_parser_tests.cpp:6670`

`type_record_context_matches_candidate` compares `tag_text_id` and, when `ts.namespace_context_id >= 0`, returns based only on `candidate->namespace_context_id == ts.namespace_context_id`. It skips `is_global_qualified` and qualifier `TextId` comparison in that branch.

The Sema contract added in this diff describes record identity as namespace context plus global qualification, qualifier `TextId` sequence, and base `TextId`. The parser helper is therefore looser than the documented contract for structured carriers. That may be acceptable if namespace context is proven to canonicalize the qualifier path completely, but the new unit test constructs mismatched global/qualifier metadata and still expects success, so the current proof does not establish the stricter contract.

### Low: `record_definition_in_context_by_text_id` Step 4 change is directionally aligned

Evidence:
- `src/frontend/parser/impl/types/types_helpers.hpp:290`
- `tests/frontend/frontend_parser_tests.cpp:6761`

The helper now deduplicates duplicate rendered keys for the same record node and rejects same-context/same-`TextId` different-record ambiguity instead of returning whichever `unordered_map` entry happens to appear first. This is a real improvement over rendered map iteration authority. The remaining concern is that it still scans `struct_tag_def_map`; as a bounded parser support helper, that is consistent with Step 4 only if the next packets keep moving callers toward structured/Sema-owned record identity.

## Alignment Judgments

Idea-alignment judgment: `drifting from source idea`

Runbook-transcription judgment: `plan matches idea`

Route-alignment judgment: `drifting`

Technical-debt judgment: `action needed`

Validation sufficiency: `needs broader proof`

Reviewer recommendation: `rewrite plan/todo before more execution`

## Notes

No testcase expectation downgrades or unsupported reclassifications were found. The new tests are focused on parser record lookup behavior, but one test enshrines the TextId-unique compatibility path that appears to violate the durable idea contract. The next route should not continue widening Step 4 until the plan/todo state names whether that path is a temporary compatibility escape hatch to remove or replaces it with a full structured/Sema-owned lookup.
