Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory the remaining `template_struct_*` string bridge callers

# Current Packet
## Just Finished
Step 1 inventory completed for the remaining `template_struct_*` bridge state and callers.

Categorized bridge state:
- `template_struct_defs_by_key`: structured semantic primary-template table. Written by `register_template_struct_primary(int, TextId, ...)` and read first by `find_template_struct_primary(int, TextId, ...)`.
- `template_struct_defs`: legacy compatibility lookup bridge. Written alongside the structured table with unqualified and context-qualified spellings; read only after structured and resolved-name probes miss.
- `template_struct_specializations_by_key`: structured semantic specialization table. Written by `register_template_struct_specialization(int, TextId, ...)` and read first by `find_template_struct_specializations(int, TextId, ...)`.
- `template_struct_specializations`: legacy compatibility lookup bridge. Written alongside the structured specialization table with primary and derived qualified spellings; read after structured probes, including the `primary_tpl->name` fallback.
- `instantiated_template_struct_keys`: string-shaped instantiation guard/output key, not primary identity storage. It is probed in `ensure_template_struct_instantiated_from_args(...)` and directly inserted by the older manual instantiation path in `parser_types_base.cpp`.

Callers with structured identity already available:
- Primary/specialization registration in `parser_declarations.cpp` passes `current_namespace_context_id()` plus a parser `TextId` for `last_sd->name` or `last_sd->template_origin_name`.
- Qualified lookup paths in `parser_types_base.cpp` and `types_helpers.hpp` can pass `QualifiedNameRef`, resolving namespace context and base `TextId` before fallback spelling.
- Most primary lookup probes in `parser_types_base.cpp`, `parser_declarations.cpp`, and `parser_types_template.cpp` already pass current namespace context plus `find_parser_text_id(...)`.
- Specialization selection from a known `primary_tpl` already has the primary node, but `find_template_struct_specializations(primary_tpl)` still re-enters through `primary_tpl->name` as a bridge.

String-overload or string-key holdouts:
- `find_template_struct_primary(ts.tag)` in the `struct T<...>` fall-through checks in `parser_types_base.cpp`.
- `find_template_struct_primary(origin)` in instantiated-base handling in `parser_types_base.cpp`.
- `find_template_struct_primary(*tag)` while parsing template specializations in `parser_types_struct.cpp`.
- `find_template_struct_primary(const std::string&)`, `register_template_struct_primary(const std::string&)`, and `register_template_struct_specialization(const char*)` remain compatibility overloads.
- Direct `instantiated_template_struct_keys` use in `parser_types_base.cpp` remains an older manual instantiation guard beside the newer `ensure_template_struct_instantiated_from_args(...)` path.

## Suggested Next
Execute Step 2 by first migrating the `parser_types_base.cpp` `struct T<...>` caller family from `find_template_struct_primary(ts.tag)` to the existing `(current_namespace_context_id(), find_parser_text_id(ts.tag), ts.tag)` overload. That path already has namespace context and tag spelling in scope, should preserve behavior, and avoids broadening into specialization or instantiation logic.

## Watchouts
- Keep the scope inside parser alias/template identity.
- Treat string-keyed template-struct maps as compatibility bridges unless inventory proves a use is still semantic.
- Do not widen into shared qualified-name infrastructure, backend, HIR, or repo-wide identity migration.
- Preserve existing parser behavior while changing the lookup substrate.
- `find_template_struct_specializations(primary_tpl)` is structured-first inside the helper but currently derives identity from `primary_tpl->name`; leave that for Step 3 unless Step 2 proves a primary-node key is already stored on the node.
- `instantiated_template_struct_keys` is string-shaped by design today; Step 4 should decide whether it remains an emitted-artifact guard or needs a structured primary component.

## Proof
No build required for this inventory-only packet. Ran `git diff --check`.
