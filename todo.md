Status: Active
Source Idea Path: ideas/open/199_sema_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Convert Type-Binding And NTTP Bridges

# Current Packet

## Just Finished

Step 2 fenced the static-eval enum compatibility route in
`StaticEvalIntEnumLookupInput` and `static_eval_lookup_enum`.

Rendered enum lookup is now documented as legacy/no-metadata compatibility
owned by Sema static-eval enum evaluation for older HIR/static-member callers.
The comments record the limitation and removal condition: complete structured
key or TextId misses fail closed before rendered lookup, and the rendered
bridge can be removed once every static-eval enum caller passes structured enum
metadata. The focused metadata test also proves direct named opt-in through
`StaticEvalIntEnumLookupInput::with_rendered_enum_compatibility` remains
available for no-metadata callers.

Commit `515cedaed` completed the Step 2 static-eval enum compatibility fence.

## Suggested Next

Execute Step 3: convert or hard-fence Sema type-binding and NTTP compatibility
bridges so metadata-rich callers cannot recover semantic identity through
rendered-name maps after complete structured misses.

First bounded packet:
- Inspect `same_rendered_type_name_compatibility`, rendered consteval
  type-binding substitution bridges, rendered NTTP output mirrors,
  `nttp_bindings`, `type_binding_text_ids_by_name`, and
  `type_binding_keys_by_name`.
- Pick one narrow Sema-owned route where structured binding keys, domain-scoped
  `TextId` carriers, or owner-aware template metadata are already available.
- Convert the metadata-rich path to structured authority or fence retained
  rendered lookup as explicit legacy/no-metadata compatibility with owner,
  limitation, and removal condition.
- Add stale-rendered type-binding or NTTP proof for the converted/fenced route.
- Do not expand into broad HIR lowerer cleanup; record HIR-only residuals as
  follow-up scope.

## Watchouts

- The rendered enum bridge is intentionally retained because HIR/static-member
  no-metadata callers still opt in through the named compatibility API.
- Type-binding and NTTP rendered names may remain as display/source payload or
  explicit no-metadata compatibility; they must not act as semantic authority
  after a complete structured miss.
- Do not broaden Step 3 into HIR lowerer cleanup unless the supervisor assigns
  that scope; the current plan is Sema-owned compatibility retirement.

## Proof

Passed delegated proof:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^cpp_hir_sema_consteval_type_utils_structured_metadata$"' > test_after.log 2>&1`

Result: passed, 1/1 focused test green. Proof log: `test_after.log`.
