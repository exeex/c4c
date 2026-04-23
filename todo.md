Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Revalidated the active lexical-lookup inventory against the current parser
state. The live partition is:
`TextId`-native already (`typedef_fn_ptr_info`, `enum_consts`,
`const_int_bindings`, `non_atom_*`, `ParserTemplateScopeParam::name_text_id`);
scope-local substrate already present (`ParserLexicalScopeState` owns
`visible_typedef_types` and `visible_value_types`); structured-qualified
holdouts intentionally deferred (`known_fn_names`, `struct_typedefs`,
`concept_qualified_names`); and the main single-name cleanup still pending is
the legacy string-backed `concept_names` shadow path.

## Suggested Next
Execute step 2 with the narrowest live packet: wire the simplest local
typedef/value registration and lookup sites through `ParserLexicalScopeState`
before touching concepts or any structured-qualified table.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. Do not reopen the
qualified-owner lookup slice completed under idea 84. The first migration
packet should stay on unqualified local typedef/value visibility and treat the
string-backed `concept_names` cleanup as a follow-on once the scope-local
typedef/value route is proven.

## Proof
Not run. Inventory refresh only.
