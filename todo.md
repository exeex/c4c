Status: Active
Source Idea Path: ideas/open/95_parser_dual_lookup_structured_identity_cleanup.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Prefer ID-First Parser Helpers

# Current Packet

## Just Finished

Step 6 - Prefer ID-First Parser Helpers continued with a narrow synthesized
typedef cleanup. `register_synthesized_typedef_binding(TextId, spelling)` now
resolves the semantic spelling from the `TextId`, stores that spelling in the
synthesized `TypeSpec`, and registers through the ID-first
`register_typedef_binding(TextId, spelling, ...)` path directly instead of
reconstructing a string and calling the compatibility overload.

Focused parser coverage now verifies synthesized typedef registration with a
mismatched fallback stores `SynthParam`, does not register `corrupted`, and
still unregisters by the semantic `TextId`.

## Suggested Next

Next coherent packet: continue Step 6 by auditing remaining parser-owned
string fallback probes that already have a `TextId` or `QualifiedNameKey`
carrier and can be routed through an ID-first helper without changing public
compatibility overloads.

## Watchouts

- `find_visible_typedef_type(TextId, fallback)` does not currently perform a
  general global TextId lookup before the string fallback path; the strengthened
  synthesized typedef test inspects the registered `SynthParam` type directly.
- The string overload for synthesized typedef registration remains a
  compatibility bridge into the ID-first overload.

## Proof

Passed: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' > test_after.log 2>&1`

Proof log: `test_after.log`
