Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory parser alias/template identity surfaces

# Current Packet
## Just Finished
Activated the runbook for idea 86 and set the active step pointer to Step 1. Completed the Step 1 inventory pass over parser alias/template identity surfaces:
- `template_state_.alias_template_info` is string-keyed alias-template metadata storage and is the semantic registry to classify first.
- `defined_struct_tags`, `struct_tag_def_map`, `template_struct_defs`, `template_struct_instantiations`, `instantiated_template_struct_keys`, and `template_instantiation_tokens` are template-owned string-keyed tables that currently preserve rendered-name identity rather than canonical typed identity.
- `last_using_alias_name`, `last_resolved_typedef`, and `current_struct_tag` each keep duplicate active-context identity in both `std::string` and `TextId` form, so the packet should treat them as mirrored context fields rather than isolated scalar state.
- `parser_declarations.cpp` still registers alias templates through `set_last_using_alias_name(...)` and stores alias metadata under string names.
- `parser_types_declarator.cpp` still probes `alias_template_info` through rendered names.
- `parser_types_base.cpp` still falls back to rendered-name alias-template lookup.

## Suggested Next
Start the implementation packet that replaces rendered-name alias-template reads with canonical identity plumbing, beginning at the declaration path and then following the template lookup fallback sites.

## Watchouts
- Keep the work limited to the active parser/frontend scope.
- Do not promote bridge-only spelling recovery into the semantic storage path.

## Proof
Inventory-only packet. No build or tests were run, and `test_after.log` was intentionally left unchanged.
