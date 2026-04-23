Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Advanced `plan.md` step 2 by retargeting the template-argument identifier-token
helper in `parser_types_declarator.cpp` so it checks the local visible typedef
scope by `TextId` before the broader spelling-based fallback. Added focused
parser coverage for the local-visible-typedef-first template-argument path.

## Suggested Next
Continue `plan.md` step 2 with one narrow packet that audits the remaining
identifier-token type-head helpers in `parser_types_declarator.cpp` for the
same local-visible-typedef-first ordering before any broader visible-facade
fallbacks.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. Do not reopen the
qualified-owner lookup slice completed under idea 84, and do not treat
structured-qualified tables or post-parse enum/name fallbacks as candidates for
flat `TextId` replacement.

## Proof
Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' | tee test_after.log`.
Proof log: `test_after.log`.
