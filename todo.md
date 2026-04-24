Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Narrow instantiation and mangled-name bridge behavior around structured template identity

# Current Packet
## Just Finished
Step 4 simplified the redundant duplicate-instantiation guard in `ensure_template_struct_instantiated_from_args(...)`.

The function now relies on `struct_tag_def_map` to decide whether injected parsing is needed and no longer builds or checks `instantiated_template_struct_keys` in that bridge. Mangled-name construction and emitted struct-tag spelling are unchanged.

## Suggested Next
Supervisor should decide whether Step 4 has enough proof to close the active runbook or whether the remaining direct emitted-artifact guard in `parser_types_base.cpp` needs a separate packet.

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
- `instantiated_template_struct_keys` should not become a parser identity authority. If kept, document or tighten it as a duplicate-emission guard that is reached only after primary-node/specialization selection.
- This packet intentionally did not change `build_template_struct_mangled_name(...)`, `template_name` injected-parse spelling, or the direct concrete-struct emission path in `parser_types_base.cpp`.

## Proof
Ran delegated proof:

`bash -lc 'set -o pipefail; cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '\''^frontend_parser_tests$'\'' | tee test_after.log'`

Result: passed. `test_after.log` updated.

Also ran `git diff --check`; result: passed.
