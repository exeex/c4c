# Step 4 HIR Late Substitution Review

Active source idea: `ideas/open/150_nttp_type_binding_domain_key_contract.md`

Chosen base commit: `5f7203b73` (`[plan] Activate NTTP type binding domain key contract plan`)

Why this base: it is the lifecycle activation commit for the current active source idea and created the current `plan.md`/`todo.md` from that idea. The prior commit `5d8c68fce` edited the source idea, but `5f7203b73` is the active runbook checkpoint requested by the review base rule.

Commits since base: 11

## Findings

1. High: Step 4 still makes HIR NTTP late substitution authoritative on `TextId` alone.

   `TemplateArgRef` only carries `nttp_text_id` for forwarded NTTP value refs, with no owner template key, parameter index, or parameter kind metadata in the value carrier (`src/frontend/parser/ast.hpp:184`). The new Step 4 lookup in `HirTemplateArgMaterializer::find_bound_nttp_for_text_id` matches that `TextId` against the current primary template's parameter table, then indexes the legacy `nttp_bindings` string map by the matched parameter name (`src/frontend/hir/impl/templates/materialization.cpp:240`). `resolve_explicit_typed_arg` treats this as the structured path whenever `ref.nttp_text_id != kInvalidText` (`src/frontend/hir/impl/templates/materialization.cpp:748`).

   That is not the source idea contract. The idea says `TextId` is spelling identity only and semantic identity must include owner template key/context, parameter index, parameter kind, and structured binding payload. This is not just a renamed debug-text lookup, but it is still spelling-only authority for NTTP late substitution. It can bind the wrong value when equal NTTP spellings exist across template owners or nested instantiation contexts.

2. Medium: Type substitution has a similar TextId-only acceptance path when owner metadata is absent.

   `type_param_name_for_ref` accepts a missing `template_param_owner_text_id` as matching the current primary owner (`src/frontend/hir/impl/templates/materialization.cpp:269`), then maps `template_param_text_id`/`tag_text_id` to a current primary parameter name and looks up `tpl_bindings` by string (`src/frontend/hir/impl/templates/materialization.cpp:315`). This is better than direct `debug_text`, and typed carriers with owner metadata are guarded, but missing-owner carriers still become semantic matches by spelling. That keeps a residual form of TextId-alone authority in the HIR materializer.

3. Medium: The repaired `Args1#0` path is generic enough not to be named-fixture overfit, but it remains a compatibility bridge and should not be treated as final authority.

   The `cb7c54533` repair is not hard-coded to the fixture name; it parses any `Base#N` pack binding with `parse_pack_binding_name_hir` (`src/frontend/hir/impl/templates/materialization.cpp:404`). It also avoids direct `tpl_bindings.find("Args1")` for structured carriers. That makes the path acceptable as a bounded compatibility repair for existing explicit pack-series bindings.

   However, the fallback for structured-but-owner-mismatched pack refs still gets the base name from `debug_name`/legacy tag candidates (`src/frontend/hir/impl/templates/materialization.cpp:331`, `src/frontend/hir/impl/templates/materialization.cpp:404`). This should stay in `todo.md` as compatibility with a removal condition, not be counted as satisfying the source idea's final HIR domain-key contract.

4. Medium: There is still an older HIR string fallback in the reviewed target that Step 4 did not retire.

   `find_template_typedef_binding` still converts `template_param_text_id`/`tag_text_id` through the text table and probes `type_bindings.find(key)` before rejecting structured carriers (`src/frontend/hir/impl/templates/type_resolution.cpp:106`). Blame shows this predates Step 4, but Step 4's goal is specifically to migrate HIR late substitution away from rendered string keys. This should be explicitly inventoried as remaining compatibility or removed in the next packet.

5. Low: No expectation downgrade was found in the reviewed diff.

   The test changes are primarily API migration to `ParserTemplateBindingSet`, plus compatibility coverage for legacy string-pair overloads and a structured NTTP metadata test. I did not find supported-path tests converted to unsupported or weakened diagnostic contracts in `5f7203b73..HEAD`.

## Judgments

Idea alignment: drifting from source idea

Runbook transcription: plan matches idea

Route alignment: drifting

Technical debt: action needed

Validation sufficiency: needs broader proof after authority repair

Reviewer recommendation: rewrite plan/todo before more execution

## Notes

The Step 4 changes did meaningfully gate several plain `debug_text` fallbacks. The route is not merely a wholesale rename of string/debug-text authority. The blocking problem is narrower but central: HIR NTTP late substitution still lacks owner/index/kind metadata in its carrier, so the new structured path cannot meet the source idea's domain-key contract.

Recommended next packet: update the Step 4/Step 5 todo to require a real NTTP domain carrier through `TemplateArgRef` or an adjacent HIR binding carrier before further cleanup, then add a focused proof with same-spelled NTTP parameters in different owners/contexts. Keep the `Args1#0` pack bridge as explicitly temporary compatibility until the pack binding map has a structured representation.
