Status: Active
Source Idea Path: ideas/open/85_parser_ctor_init_visible_head_probe_split.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add focused regressions for each seam

# Current Packet

## Just Finished
Split the visible-head decision into an explicit ctor-init handoff inside
`probe_ctor_vs_function_decl()`. The parser now names the value-head vs
type-head outcome before it falls through to the unresolved grouped
pointer/reference checks and the tentative parameter-list parse, while keeping
the current `source(other)` behavior unchanged.

## Suggested Next
Add focused parser regressions that pin the three mapped seams separately:
parenthesized visible-value direct-init, grouped unresolved
pointer/reference starters, and the visible-head handoff boundary itself.

## Watchouts
Do not let this route drift back into the broader lexical-scope lookup plan.
The explicit handoff is only a boundary refactor. Do not use it as cover for a
classifier-first reorder that flips `Box value(source(other));` back onto the
function-declaration side, and keep the grouped `identifier &/* identifier`
fast-reject independent from the visible-value path.

## Proof
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' | tee test_after.log`
passed after the visible-head handoff extraction. Proof log: `test_after.log`.
