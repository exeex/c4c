Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Completed `plan.md` step 1 by reconciling the active source-idea inventory with
the current parser state: `enum_consts`, `const_int_bindings`, and template
scope params are already `TextId`-aware; scope-local typedef/value lookup now
has dedicated `ParserLexicalScopeState` storage; qualified holdouts such as
`known_fn_names` and `struct_typedefs` remain on their structured path.

## Suggested Next
Execute `plan.md` step 2 with one narrow packet that expands the
scope-local typedef/value facade to any remaining unqualified parser-visible
lookup probes that still bypass `find_visible_typedef_type` or
`find_visible_var_type`, then add focused parser tests for that route.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. Do not reopen the
qualified-owner lookup slice completed under idea 84, and do not treat
structured-qualified tables as candidates for flat `TextId` replacement.

## Proof
Not run. Execution-state update only; no code or lifecycle structure changed.
