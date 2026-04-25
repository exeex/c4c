Status: Active
Source Idea Path: ideas/open/95_parser_dual_lookup_structured_identity_cleanup.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Demote Parser-Owned Legacy String Paths

# Current Packet

## Just Finished

Step 8 - Demote Parser-Owned Legacy String Paths continued by auditing
template primary/specialization rendered-name maps and NTTP default cache
mirrors in `src/frontend/parser/impl/types/template.cpp`.

Template primary and specialization lookup now keep structured
`QualifiedNameKey`/`TextId` lookup as the semantic path when callers provide a
valid TextId. Rendered-name primary/specialization maps remain as explicit
TextId-less compatibility fallbacks only; focused parser proof rejects
valid-TextId promotion from legacy-only rendered maps while preserving
`kInvalidText` compatibility.

`eval_deferred_nttp_default` now reads the structured default-token cache first
when the template name has a TextId, still compares the rendered mirror for
mismatch counting, and no longer falls back to a legacy-only rendered mirror for
valid-TextId lookups. TextId-less NTTP default lookup still preserves rendered
cache compatibility.

## Suggested Next

Next coherent packet: continue Step 8 by auditing template instantiation
de-dup mirrors in `src/frontend/parser/impl/types/template.cpp` and
`src/frontend/parser/impl/types/base.cpp`; demote rendered de-dup fallback only
where a structured `TemplateInstantiationKey` is available and focused proof
preserves an explicit compatibility bridge for TextId-less/direct-emission
paths.

## Watchouts

- Template primary/specialization rendered maps are still populated as mirrors
  during registration; this packet demoted read fallback, not mirror writes.
- NTTP default rendered cache mirrors are still populated and mismatch-checked;
  only valid-TextId legacy-only fallback was demoted.
- `lookup_value_in_context`, `lookup_using_value_alias`,
  `register_var_type_binding`, string-facing `find_var_type`, and
  `find_visible_var_type(kInvalidText, name)` remain compatibility bridges and
  should not be demoted without focused public-string and visible-lookup proof.
- Remaining parser string bridges include template instantiation de-dup mirrors
  and known-function string lookup.

## Proof

Passed: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' > test_after.log 2>&1`

Proof log: `test_after.log`
