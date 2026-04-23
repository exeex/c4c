Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Move unqualified visible lookup onto the new scope-local path

# Current Packet

## Just Finished
Completed another Step 4 parser packet by routing the remaining unqualified
template/struct type-head helper probes through a shared local-first visible
type helper. `is_type_start`, value-template argument probing, and expression
owner/type-head checks now consult the scoped visible typedef facade before
falling back to legacy visible-name resolution.

## Suggested Next
Continue Step 4 by auditing the remaining direct-resolution paths that still
assume local typedefs are mirrored into legacy flat tables, especially the
template member lookup fallback in `parser_types_template.cpp` and the
unqualified typedef resolution paths in `parse_base_type()` and dependent
typename handling.

## Watchouts
Do not collapse namespace traversal into lexical lookup. Step 3 only adds the
local-first facade; local bindings are still mirrored into the legacy flat
tables in some paths, so remaining Step 4 cleanup should keep qualified lookup
and namespace-owned resolution on their separate route. The new helper is only
for type-head probes; it should not replace places that still need canonical
typedef names for alias-template or compatibility bookkeeping.

## Proof
Proof command: `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^frontend_parser_tests$'`
Result: passed; proof log at `test_after.log`.
