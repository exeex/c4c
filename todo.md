Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Replace remaining suitable single-name string tables

# Current Packet

## Just Finished
Advanced Step 5 by making template-scope parameter tracking carry `TextId` alongside the compatibility spelling bridge, and updated active type-parameter probes to use token `text_id` when available.

## Suggested Next
Continue Step 5 with another narrow single-name table that still stores semantic identity as `std::string`, preferably one that does not also carry qualified-name semantics.

## Watchouts
Keep namespace traversal separate from lexical scope lookup. Do not treat qualified or owner-scoped names as plain `TextId` migration candidates.

## Proof
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^frontend_cxx_'`
