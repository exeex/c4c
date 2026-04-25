Status: Active
Source Idea Path: ideas/open/95_parser_dual_lookup_structured_identity_cleanup.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Prefer ID-First Parser Helpers

# Current Packet

## Just Finished

Step 6 - Prefer ID-First Parser Helpers started with a focused parser-owned
helper migration. `register_typedef_name`, `register_typedef_binding`, and
`register_var_type_binding` now expose `TextId`/spelling overloads, while the
existing string overloads remain compatibility bridges that delegate to the
ID-first forms.

Parser declaration call sites that already receive declarator `TextId`s now use
the ID-first typedef registration path. Simple unqualified global variable
registration now uses the ID-first value registration path when the registered
name is the declarator spelling; qualified/bridged names stay on the legacy
string bridge.

Focused parser coverage now proves the ID-first typedef and value helpers prefer
the `TextId` carrier over a mismatched fallback spelling and still populate the
structured value table.

## Suggested Next

Next coherent packet: continue Step 6 by migrating another narrow group of
parser-owned helper call sites where a `QualifiedNameKey` or context plus
`TextId` is already available, especially using-import or known-function paths
that can preserve their current compatibility bridge behavior.

## Watchouts

- Qualified or bridge-rendered variable registrations were intentionally left
  on string compatibility overloads unless the declarator spelling exactly
  matched the registered name.
- The string overloads still intern/populate through the same compatibility
  tables; this packet did not delete or demote any bridge API.

## Proof

Passed: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' > test_after.log 2>&1`

Proof log: `test_after.log`
