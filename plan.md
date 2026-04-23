# Parser Constructor-Init Visible-Head Probe Split

Status: Active
Source Idea: ideas/open/85_parser_ctor_init_visible_head_probe_split.md

## Purpose
Split the constructor-init visible-head ambiguity into smaller parser seams
so the `source(other)` regression family can be isolated and repaired.

## Goal
Make the constructor-init probe boundary testable in smaller pieces without
reintroducing the `Box value(source(other));` regression.

## Core Rule
Do not widen the constructor-init probe route into a broad parser cleanup or
testcase-shaped special case.

## Read First
- `ideas/open/85_parser_ctor_init_visible_head_probe_split.md`
- `src/frontend/parser/parser_declarations.cpp`
- `src/frontend/parser/parser_types_base.cpp`
- `src/frontend/parser/parser.hpp`
- `tests/frontend/frontend_parser_tests.cpp`

## Current Targets
- constructor-init visible-head classification around `probe_ctor_vs_function_decl()`
- the handoff between `classify_visible_value_or_type_head()` and later
  call-like starter checks
- grouped pointer/reference and parenthesized local-value direct-init seams
- focused parser regressions that prove each seam independently

## Non-Goals
- no lexical-scope lookup migration work from idea 83
- no namespace traversal or qualified-name refactor
- no expectation downgrades or testcase-shaped shortcuts
- no broad parser reshaping beyond the constructor-init ambiguity seams

## Working Model
- treat the current failure as a probe decomposition problem
- keep parenthesized local-value direct-init behavior separate from grouped
  pointer/reference starter handling
- use small focused regressions to prove each seam before recombining them
- preserve the passing `Box value(source(other));` path while adjusting the
  probe boundary

## Execution Rules
- preserve existing parser behavior unless a seam-specific test proves it is
  unsafe
- make the visible-head handoff explicit before retrying the earlier
  classifier ordering
- keep validation narrow and parser-focused unless the blast radius grows
- if a seam cannot be separated cleanly, record the blocker in `todo.md`

## Steps
1. Map the constructor-init ambiguity seams.
   - Record the visible-head failure family and the exact `source(other)`
     regression boundary.
   - Completion check: the grouped failure is named as distinct seams in
     `todo.md`.

2. Separate the visible-head handoff from later starter checks.
   - Refine the probe flow so the classifier boundary is explicit.
   - Preserve the stable `Box value(source(other));` path.
   - Completion check: the handoff can be reasoned about as a smaller decision
     point.

3. Add focused regressions for each seam.
   - Cover parenthesized local-value direct-init, grouped pointer/reference
     starters, and the handoff boundary.
   - Completion check: each seam has an explicit focused parser regression.

4. Re-run focused parser proof.
   - Build the frontend and run the narrow parser subset covering the seams.
   - Completion check: the updated seam split passes focused proof.
