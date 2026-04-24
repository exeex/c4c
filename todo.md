Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Narrow instantiation and mangled-name bridge behavior around structured template identity

# Current Packet
## Just Finished
Step 4 simplified the redundant duplicate-instantiation guard in `ensure_template_struct_instantiated_from_args(...)`.

The function now relies on `struct_tag_def_map` to decide whether injected parsing is needed and no longer builds or checks `instantiated_template_struct_keys` in that bridge. Mangled-name construction and emitted struct-tag spelling are unchanged.

Plan-owner lifecycle decision: keep the active runbook open for one more Step 4 packet. The remaining `instantiated_template_struct_keys` use in `parser_types_base.cpp` is reached only after `primary_tpl`, `tpl_def`, concrete args, specialization selection, and mangled-name construction are resolved, so it is not a primary lookup authority. It is still a direct string-keyed duplicate-emission guard and should be explicitly tightened or documented at the implementation site before closing the template-struct identity runbook.

## Suggested Next
Execute Step 4 packet: in `parser_types_base.cpp`, tighten or document the direct `instantiated_template_struct_keys` guard as an emitted-artifact duplicate guard only.

Packet scope:
- keep `make_template_struct_instance_key(primary_tpl, concrete_args)` and `build_template_struct_mangled_name(...)` spelling stable
- do not route primary or specialization lookup through rendered names
- do not remove compatibility string maps
- do not change `ensure_template_struct_instantiated_from_args(...)`, which already uses `struct_tag_def_map` for the injected-parse bridge
- add the smallest implementation clarification needed so the remaining string set cannot be mistaken for parser identity authority

Proof command:

`bash -lc 'set -o pipefail; cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '\''^frontend_parser_tests$'\'' | tee test_after.log'`

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
