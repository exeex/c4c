Status: Active
Source Idea Path: ideas/open/85_parser_ctor_init_visible_head_probe_split.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Separate the visible-head handoff from later starter checks

# Current Packet

## Just Finished
Mapped the constructor-init ambiguity into three concrete parser seams. The
current collision splits into: the parenthesized local-value direct-init path
covered by `Box value(source(other));`, the grouped unresolved
pointer/reference starter fast-reject at
`parser_declarations.cpp:855-870`, and the visible-head handoff where
`probe_ctor_vs_function_decl()` consumes
`classify_visible_value_or_type_head()` before the later call-like starter
checks in `parser_declarations.cpp:872-885`.

## Suggested Next
Make the visible-head handoff explicit inside
`probe_ctor_vs_function_decl()` so the `qualified_head_kind == 1` value path
can be reasoned about separately from the unresolved grouped
pointer/reference fallbacks and the later tentative parameter-list parse.

## Watchouts
Do not let this route drift back into the broader lexical-scope lookup plan.
The seam map to preserve is:
`Box copy(Value other);` and `Box copy(Value(other));` stay on the function
declaration side, `Box value(source(other));` stays on the direct-init
declaration side, and the grouped `identifier &/* identifier` starter check
must remain independent from any visible-head reorder.

## Proof
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' | tee test_after.log`
passed after recording the seam map against the current parser baseline. Proof
log: `test_after.log`.
