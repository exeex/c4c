Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Retarget template-struct primary registration and lookup to structured keys

# Current Packet
## Just Finished
Step 2 migrated the narrow `parser_types_base.cpp` `struct T<...>` fall-through caller family from `find_template_struct_primary(ts.tag)` to the structured primary lookup overload using `current_namespace_context_id()`, `find_parser_text_id(ts.tag)`, and the tag spelling. The legacy string lookup remains reachable only inside the structured helper after structured and resolved-name probes miss.

## Suggested Next
Continue Step 2 with the next narrow primary lookup migration in `parser_types_base.cpp`: evaluate the instantiated-base `find_template_struct_primary(origin)` caller family and retarget it only if the local origin spelling has an unambiguous namespace context and parser `TextId` available.

## Watchouts
- Keep the scope inside parser alias/template identity.
- Treat string-keyed template-struct maps as compatibility bridges unless inventory proves a use is still semantic.
- Do not widen into shared qualified-name infrastructure, backend, HIR, or repo-wide identity migration.
- Preserve existing parser behavior while changing the lookup substrate.
- The `struct T<...>` fall-through path now uses structured primary lookup explicitly; do not reintroduce direct `find_template_struct_primary(ts.tag)` calls there.
- `find_template_struct_specializations(primary_tpl)` is structured-first inside the helper but currently derives identity from `primary_tpl->name`; leave that for Step 3 unless Step 2 proves a primary-node key is already stored on the node.
- `instantiated_template_struct_keys` is string-shaped by design today; Step 4 should decide whether it remains an emitted-artifact guard or needs a structured primary component.

## Proof
Ran `bash -lc 'set -o pipefail; cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '\''^frontend_parser_tests$'\'' | tee test_after.log'`.

Result: passed. Proof log: `test_after.log`.
