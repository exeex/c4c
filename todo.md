Status: Active
Source Idea Path: ideas/open/95_parser_dual_lookup_structured_identity_cleanup.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Demote Parser-Owned Legacy String Paths

# Current Packet

## Just Finished

Step 8 - Demote Parser-Owned Legacy String Paths continued by auditing
known-function string lookup bridges in
`src/frontend/parser/impl/core.cpp`.

`register_known_fn_name_in_context` now prefers the namespace context plus
valid base `TextId` when that structured identity is available, including the
previously legacy-promoting qualified fallback branch. Rendered fallback is
preserved only for TextId-less registration and the public string-facing
`register_known_fn_name`/`has_known_fn_name` compatibility bridge.

Focused parser coverage now proves that a qualified rendered fallback with a
valid base TextId registers `idFirstNs::idFirstQualifiedFn` instead of the
stale rendered fallback, while public string-facing lookup and TextId-less
known-function registration remain compatible.

## Suggested Next

Completion handoff for this Step 8 packet: no parser-owned known-function
string lookup bridge remains in `src/frontend/parser/impl/core.cpp` beyond the
explicit public string-facing overloads and TextId-less rendered fallback
compatibility. Supervisor should decide whether Step 8 has more parser-owned
legacy string families to packetize or should move to lifecycle review.

## Watchouts

- Template primary/specialization rendered maps are still populated as mirrors
  during registration; this packet demoted read fallback, not mirror writes.
- NTTP default rendered cache mirrors are still populated and mismatch-checked;
  only valid-TextId legacy-only fallback was demoted.
- Template instantiation de-dup rendered mirrors are still populated by mark
  paths and healed from structured-present state; legacy-only rendered state is
  now mismatch-observed but not promoted when a structured
  `TemplateInstantiationKey` exists.
- `lookup_value_in_context`, `lookup_using_value_alias`,
  `register_var_type_binding`, string-facing `find_var_type`, and
  `find_visible_var_type(kInvalidText, name)` remain compatibility bridges and
  should not be demoted without focused public-string and visible-lookup proof.
- Known-function public string-facing overloads remain bridge-required by
  design; this packet only demoted parser-owned context registration where
  TextId/QualifiedNameKey identity is available.

## Proof

Passed: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' > test_after.log 2>&1`

Proof log: `test_after.log`
