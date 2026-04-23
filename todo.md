Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Advanced `plan.md` step 2 by retargeting the statement-side `if` condition
declaration probe and the shared value-like template disambiguation helper to
keep the current token `TextId` on the first typedef/type-head lookup instead
of dropping straight to the string-only wrapper path. Added focused parser
coverage that proves a scope-local typedef alias now parses as an `if`
condition declaration through the lexical scope facade.

## Suggested Next
Continue `plan.md` step 2 with one narrow packet that audits the remaining
parser helper sites where an identifier token already carries `TextId` but the
first type-head or typedef probe still routes through a string-only wrapper or
recomputed spelling path.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. Do not reopen the
qualified-owner lookup slice completed under idea 84, and do not treat
structured-qualified tables or post-parse enum/name fallbacks as candidates for
flat `TextId` replacement.

## Proof
Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' | tee test_after.log`.
Proof log: `test_after.log`.
