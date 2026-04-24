Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Retarget template-struct specialization lookup to the same structured key path

# Current Packet
## Just Finished
Step 3 cleaned up `parser_types_template.cpp::ensure_template_struct_instantiated_from_args(...)` so specialization pattern selection now calls `find_template_struct_specializations(primary_tpl)` when `primary_tpl` is already available. This moves that caller onto the primary-node identity path while preserving the existing `template_name` spelling for mangling and injected-parse instantiation.

## Suggested Next
Complete the remaining Step 3 caller cleanup in `parser_types_base.cpp` by replacing the transformed-owner fallback specialization probes that still have a `primary_tpl` with `find_template_struct_specializations(primary_tpl)` or equivalent primary-node identity lookup, leaving `QualifiedNameRef` callers intact.

## Watchouts
- Keep the scope inside parser alias/template identity.
- Treat string-keyed template-struct maps as compatibility bridges unless inventory proves a use is still semantic.
- Do not widen into shared qualified-name infrastructure, backend, HIR, or repo-wide identity migration.
- Preserve existing parser behavior while changing the lookup substrate.
- `ensure_template_struct_instantiated_from_args(...)` now uses primary-node identity for specialization selection; keep `template_name` for emitted instantiation spelling and mangled-name construction.
- `find_template_struct_specializations(primary_tpl)` still falls through the shared context/`TextId`/fallback helper, so this cleanup should not remove compatibility behavior.
- Do not migrate the `types_helpers.hpp` qualified-type probe away from `QualifiedNameRef`; it has no primary node and is already the structured source available at that point.
- `instantiated_template_struct_keys` is string-shaped by design today; Step 4 should decide whether it remains an emitted-artifact guard or needs a structured primary component.

## Proof
Ran delegated proof:

`bash -lc 'set -o pipefail; cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '\''^frontend_parser_tests$'\'' | tee test_after.log'`

Result: passed. Proof log: `test_after.log`.
