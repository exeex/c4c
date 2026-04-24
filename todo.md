Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Retarget template-struct specialization lookup to the same structured key path

# Current Packet
## Just Finished
Step 3 retargeted `find_template_struct_specializations(const Node*)` and the shared specialization helper to prefer the primary template node's declaration identity. Specialization lookup now probes `primary_tpl->namespace_context_id` plus `unqualified_text_id`/`unqualified_name` before falling back to current-context or rendered-name compatibility paths.

## Suggested Next
Continue Step 3 by inventorying direct callers of `find_template_struct_specializations(...)` and migrating any remaining caller-provided rendered template names to structured `QualifiedNameRef` or primary-node identity where available.

## Watchouts
- Keep the scope inside parser alias/template identity.
- Treat string-keyed template-struct maps as compatibility bridges unless inventory proves a use is still semantic.
- Do not widen into shared qualified-name infrastructure, backend, HIR, or repo-wide identity migration.
- Preserve existing parser behavior while changing the lookup substrate.
- The `struct T<...>` fall-through path now uses structured primary lookup explicitly; do not reintroduce direct `find_template_struct_primary(ts.tag)` calls there.
- The instantiated-base path now has no direct `find_template_struct_primary(origin)` calls; keep qualified origin handling on `QualifiedNameRef` rather than rendered-name shortcuts.
- The template-specialization parse probe now preserves the parsed tag `QualifiedNameRef`; keep qualified/global specialization lookup on that structured name instead of the rendered tag spelling.
- `find_template_struct_specializations(primary_tpl)` now uses declaration identity when `apply_decl_namespace` populated the primary node; keep legacy rendered-name fallback for older injected or compatibility nodes.
- `instantiated_template_struct_keys` is string-shaped by design today; Step 4 should decide whether it remains an emitted-artifact guard or needs a structured primary component.

## Proof
Ran `bash -lc 'set -o pipefail; cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '\''^frontend_parser_tests$'\'' | tee test_after.log'`.

Result: passed. Proof log: `test_after.log`.
