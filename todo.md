Status: Active
Source Idea Path: ideas/open/158_sema_validate_string_authority_audit.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Global, Function, Enum, and Consteval Lookup Precedence

# Current Packet

## Just Finished

Step 2: Global, Function, Enum, and Consteval Lookup Precedence advanced the
ordinary value lookup path for complete unqualified global/function/enum
metadata. `lookup_symbol` now treats a valid structured value key or alias
target key as authoritative for the covered path, so a structured miss does not
recover through stale rendered global/enum bridge lookup, and function lookup
does not fall back by rendered name for the same complete unqualified case.
A focused frontend test now covers stale rendered global and enum names while
preserving no-metadata rendered compatibility.

## Suggested Next

Continue Step 2 with a separately bounded packet for qualified value/function
lookup, keeping transitional anonymous-namespace and using-declaration cases in
view before removing their rendered-name compatibility.

## Watchouts

- This slice intentionally limits fail-closed behavior to complete unqualified
  value metadata; qualified rendered names such as anonymous-namespace globals
  and namespace using-declaration imports still need their own structured route
  before their compatibility bridge can be removed.
- Do not weaken tests or mark supported paths unsupported.
- The new test is `cpp_positive_sema_lookup_value_structured_metadata`.

## Proof

Passed: `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(positive_sema_|cpp_positive_sema_)') > test_after.log 2>&1`

Proof log: `test_after.log` reports 919/919 matching positive sema tests
passed.
