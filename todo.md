Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Advanced `plan.md` step 2 by retargeting the shared qualified-type helper in
`types_helpers.hpp`. The unqualified visible-typedef branch in
`resolve_qualified_typedef_name(...)` now uses the existing `TextId`-aware
`visible_type_head_name(...)` path instead of returning the raw base spelling.

## Suggested Next
Continue `plan.md` step 2 by checking the next shared parser helper that still
prefers spelling-first visible-type resolution and confirm whether it already
has a usable `TextId` path.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. Do not reopen the
qualified-owner lookup slice completed under idea 84, and do not treat
structured-qualified tables or post-parse enum/name fallbacks as candidates for
flat `TextId` replacement. The unqualified helper now prefers the shared
visible-name resolver, so future step-2 packets should preserve that path
rather than reintroducing spelling-only returns.

## Proof
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' | tee test_after.log` passed. Proof log: `test_after.log`.
