Status: Active
Source Idea Path: ideas/open/95_parser_dual_lookup_structured_identity_cleanup.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Prefer ID-First Parser Helpers

# Current Packet

## Just Finished

Step 6 - Prefer ID-First Parser Helpers continued by migrating the `using`
import value/type lookup path to structured helpers where the target
`QualifiedNameKey` or resolved `VisibleNameResult` key is already available.
New `find_typedef_type(QualifiedNameKey, fallback)` and
`find_var_type(QualifiedNameKey, fallback)` helpers probe structured storage
first, then rendered compatibility spelling, then the explicit fallback.

`using` type imports now use the resolved type key plus visible spelling before
legacy string fallback. `using` value imports now use the directly computed
target key first, then the resolved value key for namespace/context lookups and
known-function aliases, while preserving bridge registration names and
compatibility alias spellings.

Focused parser coverage now proves `using ns::Target;` value imports and
`using ns::Alias;` type imports prefer structured target storage over corrupted
rendered fallback names.

## Suggested Next

Next coherent packet: continue Step 6 by auditing remaining parser-owned
string fallback probes that already have a `QualifiedNameKey` or
context-plus-`TextId` carrier, especially outside the `using` import block.

## Watchouts

- Bridge registrations for imported typedef/value aliases still use
  `bridge_name_in_context` so legacy string consumers remain populated.
- The new key-plus-fallback helpers intentionally preserve rendered and explicit
  string fallback lookup after structured lookup misses.
- Qualified operator/constructor known-function registrations and qualified
  declarator bridge names remain on their existing rendered-string paths.

## Proof

Passed: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' > test_after.log 2>&1`

Proof log: `test_after.log`
