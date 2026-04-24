Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Narrow instantiation and mangled-name bridge behavior around structured template identity

# Current Packet
## Just Finished
Step 4 tightened the remaining direct `instantiated_template_struct_keys` guard in `parser_types_base.cpp`.

The guard now uses an `emitted_instance_key` local and documents that structured primary/specialization selection has already resolved `tpl_def`; the string set is only a duplicate guard for emitting the concrete struct definition. Mangled-name construction, emitted struct-tag spelling, primary lookup, and specialization selection are unchanged.

## Suggested Next
Supervisor should run the lifecycle decision for this active runbook: Step 4's remaining emitted-artifact guard clarification is complete.

## Watchouts
- Keep the scope inside parser alias/template identity.
- Treat string-keyed template-struct maps as compatibility bridges unless inventory proves a use is still semantic.
- Do not widen into shared qualified-name infrastructure, backend, HIR, or repo-wide identity migration.
- Preserve existing parser behavior while changing the lookup substrate.
- `ensure_template_struct_instantiated_from_args(...)` now uses primary-node identity for specialization selection; keep `template_name` for emitted instantiation spelling and mangled-name construction.
- `find_template_struct_specializations(primary_tpl)` still falls through the shared context/`TextId`/fallback helper, so this cleanup should not remove compatibility behavior.
- Do not migrate the `types_helpers.hpp` qualified-type probe away from `QualifiedNameRef`; it has no primary node and is already the structured source available at that point.
- `instantiated_template_struct_keys` is string-shaped by design today; Step 4 should decide whether it remains an emitted-artifact guard or needs a structured primary component.
- Step 3 specialization selection call sites now visibly use primary-node lookup when `primary_tpl` is available in `parser_types_base.cpp` and `parser_types_template.cpp`; keep any future `QualifiedNameRef` specialization callers only where no primary node exists.
- Do not change mangled-name spelling in Step 4 unless a concrete semantic bug requires it; those names are emitted artifacts and struct-tag keys.
- `instantiated_template_struct_keys` remains string-shaped by design, but its remaining direct guard is now explicitly scoped as post-selection duplicate emission protection.
- This packet intentionally did not change `build_template_struct_mangled_name(...)`, `template_name` injected-parse spelling, or concrete struct emission behavior.

## Proof
Ran delegated proof:

`bash -lc 'set -o pipefail; cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '\''^frontend_parser_tests$'\'' | tee test_after.log'`

Result: passed. `test_after.log` updated.

Also ran `git diff --check`; result: passed.
