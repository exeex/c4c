Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Retarget template-struct primary registration and lookup to structured keys

# Current Packet
## Just Finished
Step 2 migrated the instantiated-base `parser_types_base.cpp` `origin` caller family away from direct `find_template_struct_primary(origin)` string overload probes. The block now resolves qualified origins through `QualifiedNameRef` and unqualified origins through `current_namespace_context_id()`, `find_parser_text_id(origin)`, and the origin spelling, leaving string-map probing only behind the structured helper fallback path.

## Suggested Next
Continue Step 2 with a supervisor-selected primary registration or lookup packet outside this instantiated-base family, or move to Step 3 if the remaining primary lookups are accepted as already structured or compatibility-only.

## Watchouts
- Keep the scope inside parser alias/template identity.
- Treat string-keyed template-struct maps as compatibility bridges unless inventory proves a use is still semantic.
- Do not widen into shared qualified-name infrastructure, backend, HIR, or repo-wide identity migration.
- Preserve existing parser behavior while changing the lookup substrate.
- The `struct T<...>` fall-through path now uses structured primary lookup explicitly; do not reintroduce direct `find_template_struct_primary(ts.tag)` calls there.
- The instantiated-base path now has no direct `find_template_struct_primary(origin)` calls; keep qualified origin handling on `QualifiedNameRef` rather than rendered-name shortcuts.
- `find_template_struct_specializations(primary_tpl)` is structured-first inside the helper but currently derives identity from `primary_tpl->name`; leave that for Step 3 unless Step 2 proves a primary-node key is already stored on the node.
- `instantiated_template_struct_keys` is string-shaped by design today; Step 4 should decide whether it remains an emitted-artifact guard or needs a structured primary component.

## Proof
Ran `bash -lc 'set -o pipefail; cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '\''^frontend_parser_tests$'\'' | tee test_after.log'`.

Result: passed. Proof log: `test_after.log`.
