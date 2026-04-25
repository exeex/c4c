Status: Active
Source Idea Path: ideas/open/95_parser_dual_lookup_structured_identity_cleanup.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Prefer ID-First Parser Helpers

# Current Packet

## Just Finished

Step 6 - Prefer ID-First Parser Helpers continued by migrating a narrow
known-function registration family. `register_known_fn_name_in_context` now
registers from declaration context plus `TextId`/fallback spelling, and the
existing string/`QualifiedNameKey` overloads remain compatibility bridges.

Using-import known-function aliases and simple unqualified parsed function
declarations now use the context-plus-`TextId` helper where those carriers are
already available. Qualified operator/constructor names and qualified
declarator bridge names intentionally stay on rendered-string registration.

Focused parser coverage now proves known-function ID-first registration prefers
the `TextId` carrier over a mismatched fallback spelling while preserving string
lookup for the rendered namespace-qualified name.

## Suggested Next

Next coherent packet: continue Step 6 by migrating another parser-owned helper
family where call sites already carry a `QualifiedNameKey` or context plus
`TextId`, without changing rendered-only bridge behavior.

## Watchouts

- Qualified or bridge-rendered variable registrations were intentionally left
  on string compatibility overloads unless the declarator spelling exactly
  matched the registered name.
- Qualified operator/constructor known-function registrations and qualified
  declarator bridge names were intentionally left on the string overload.
- The string and `QualifiedNameKey` known-function overloads remain available;
  this packet did not delete or demote any bridge API.

## Proof

Passed: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' > test_after.log 2>&1`

Proof log: `test_after.log`
