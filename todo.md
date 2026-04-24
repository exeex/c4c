Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Narrow instantiation and mangled-name bridge behavior around structured template identity

# Current Packet
## Just Finished
Step 4 inventory classified the template-struct instantiation and mangled-name bridge uses:

- `ParserTemplateState::instantiated_template_struct_keys` is currently an emitted-artifact guard. It is string-shaped and only prevents duplicate concrete `NK_STRUCT_DEF` emission in the direct template-type parse path; it is not used to find primaries, select specializations, or resolve semantic template identity.
- `make_template_struct_instance_key(primary_tpl, args)` is an emitted-artifact guard key builder. It uses the already-resolved `primary_tpl` node plus canonical argument text, so the caller has crossed the structured identity boundary before the string key is built.
- `build_template_struct_mangled_name(template_name, primary_tpl, selected_tpl, args)` is emitted artifact construction. It returns an explicit full-specialization name when selected, otherwise builds the concrete struct tag/mangled name from the selected family spelling and primary parameter order. This output is intentionally used for `struct_tag_def_map`, `defined_struct_tags`, `TypeSpec::tag`, and injected parse spelling.
- `ensure_template_struct_instantiated_from_args(...)` is the main compatibility bridge. Its semantic selection path now starts from `primary_tpl` and `find_template_struct_specializations(primary_tpl)`, then emits or probes by mangled struct tag. The remaining `template_name` parameter is output/injected-parse spelling, not specialization lookup authority.
- The transformed-owner direct instantiation path in `parser_types_base.cpp` still has one local duplicate-emission guard that checks/inserts `instantiated_template_struct_keys` before cloning and registering a concrete struct. That use is also an emitted-artifact guard because `primary_tpl`, `tpl_def`, and concrete args are already resolved before the key is built.

## Suggested Next
First safe Step 4 code packet: simplify the redundant duplicate-instantiation guard in `ensure_template_struct_instantiated_from_args(...)`. The current branch is nested under `!struct_tag_def_map.count(*out_mangled)` and then tests `!instantiated_template_struct_keys.count(instance_key) || !struct_tag_def_map.count(*out_mangled)`, making the key check ineffective for skipping injected parse. A narrow cleanup should either remove that redundant key test there or insert/check the key consistently only as an emitted-artifact guard after successful concrete emission, while leaving `template_name` and mangled output unchanged.

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
- Step 4 may close without larger code if the supervisor decides redundant-key cleanup is not worth changing behavior; the inventory found no remaining rendered-name lookup needed for instantiation decisions once `primary_tpl` is available.

## Proof
No build required for this inventory-only packet.

Ran delegated proof:

`git diff --check`

Result: passed. No proof log updated; `test_after.log` was not touched.
