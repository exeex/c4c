Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Retarget template-struct specialization lookup to the same structured key path

# Current Packet
## Just Finished
Step 3 inventoried all direct `find_template_struct_specializations(...)` callers after the primary-node retargeting:
- `types_helpers.hpp` qualified-type probing uses the `QualifiedNameRef` overload with no primary node available; no caller-side migration needed.
- `parser_types_template.cpp` overloads are the shared structured/compatibility bridge: `QualifiedNameRef`, primary-node identity, and context/`TextId`/fallback all route through the same helper, with string maps remaining fallback-only.
- `parser_types_template.cpp::ensure_template_struct_instantiated_from_args(...)` still passes caller-provided `template_name` through the context/`TextId`/fallback overload even though `primary_tpl` is available; this is a safe remaining Step 3 migration to primary-node identity.
- `parser_types_base.cpp` specialization-selection paths at the template-owner/member-follow-through sites already use primary-node identity where direct, and use `QualifiedNameRef` when the owner was parsed as qualified.
- The remaining `parser_types_base.cpp` unqualified/resolved owner fallback probes still pass rendered owner names through the context/`TextId`/fallback overload with `primary_tpl`; these are safe to collapse to primary-node identity once behavior is preserved.

## Suggested Next
Complete Step 3 with a narrow code packet that replaces the safe caller-provided rendered-name specialization probes in `ensure_template_struct_instantiated_from_args(...)` and the transformed-owner fallback in `parser_types_base.cpp` with `find_template_struct_specializations(primary_tpl)` or equivalent primary-node identity lookup, leaving `QualifiedNameRef` callers intact.

## Watchouts
- Keep the scope inside parser alias/template identity.
- Treat string-keyed template-struct maps as compatibility bridges unless inventory proves a use is still semantic.
- Do not widen into shared qualified-name infrastructure, backend, HIR, or repo-wide identity migration.
- Preserve existing parser behavior while changing the lookup substrate.
- The `struct T<...>` fall-through path now uses structured primary lookup explicitly; do not reintroduce direct `find_template_struct_primary(ts.tag)` calls there.
- The instantiated-base path now has no direct `find_template_struct_primary(origin)` calls; keep qualified origin handling on `QualifiedNameRef` rather than rendered-name shortcuts.
- The template-specialization parse probe now preserves the parsed tag `QualifiedNameRef`; keep qualified/global specialization lookup on that structured name instead of the rendered tag spelling.
- `find_template_struct_specializations(primary_tpl)` now uses declaration identity when `apply_decl_namespace` populated the primary node; keep legacy rendered-name fallback for older injected or compatibility nodes.
- Do not migrate the `types_helpers.hpp` qualified-type probe away from `QualifiedNameRef`; it has no primary node and is already the structured source available at that point.
- The context/`TextId`/fallback overload remains needed as a shared compatibility bridge and for callers that genuinely lack a primary node.
- The safe Step 3 code migration is caller cleanup, not helper deletion: keep string-map fallback behavior inside the shared helper.
- `instantiated_template_struct_keys` is string-shaped by design today; Step 4 should decide whether it remains an emitted-artifact guard or needs a structured primary component.

## Proof
Inventory-only packet; no build required and `test_after.log` was not touched.

Ran `git diff --check`.

Result: passed.
