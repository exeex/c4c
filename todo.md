Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Move unqualified visible lookup onto the new scope-local path

# Current Packet

## Just Finished
Completed another Step 4 parser packet by routing a few remaining unqualified
typedef probes through the parser-visible scope facade instead of direct legacy
flat typedef tables. `decode_type_ref_text(...)`, enum typedef recovery in
`parse_base_type()`, and alias-template bookkeeping after `using` alias
parsing now consult visible typedef lookup first, and a parser unit regression
now proves that type-ref decoding sees a scope-local alias.

## Suggested Next
Continue Step 4 by reviewing the remaining unqualified parser helpers that
still call `has_typedef_type(...)` or `find_typedef_type(...)` directly,
especially lower-level template/member-type reconstruction paths where
unqualified local visible typedefs may still bypass the scoped facade.

## Watchouts
Do not collapse namespace traversal into lexical lookup. This packet only
retargets obviously unqualified typedef probes to the visible-scope facade;
qualified names, owner/member traversal, and composed namespace lookup still
need to stay on their existing routes. Remaining Step 4 work should keep
separating unqualified visible lookup from namespace-side traversal.

## Proof
Proof command: `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^frontend_parser_tests$'`
Result: passed; proof log at `test_after.log`.
