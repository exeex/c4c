Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Advanced `plan.md` step 2 by retargeting expression-side type/value
disambiguation in `parser_expressions.cpp`. The operator-function reference
probe now carries the first qualifier token `TextId` into
`visible_type_head_name(...)`, and the qualified-type-owner probe now uses the
probe's own qualifier `TextId` instead of the outer spelling-derived name.

## Suggested Next
Continue `plan.md` step 2 with one narrow packet that audits the remaining
parser expression helpers for any other spelling-first visible-type lookup
paths where the current token or probe already carries a valid `TextId`.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. Do not reopen the
qualified-owner lookup slice completed under idea 84, and do not treat
structured-qualified tables or post-parse enum/name fallbacks as candidates for
flat `TextId` replacement.

## Proof
Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' | tee test_after.log`.
Result: passed.
Proof log: `test_after.log`.
