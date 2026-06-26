Status: Active
Source Idea Path: ideas/open/391_rv64_variadic_prologue_save_area_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Focused Backend Coverage

# Current Packet

## Just Finished

Step 3: Add Focused Backend Coverage is partially complete within the current
prepared metadata surface.

Added focused RV64 object-emission coverage for the only saved-GPR publication
authority gap that can be represented today: a variadic `va_start` entry whose
prepared contract explicitly marks
`rv64.incoming_variadic_gpr_publications` as a missing required fact. The test
sets a one-named-GPR RV64 variadic entry shape and verifies that object
admission rejects the function instead of silently accepting the helper-free
overflow-area/`va_start` route without incoming post-named GPR publication
authority.

No backend implementation files were touched.

## Suggested Next

Step 4 should introduce the explicit prepared saved-GPR publication fact schema
and narrow RV64 object-emission consumer, then add accepted-route coverage plus
duplicate, mismatched, and out-of-area fact-shape coverage against that real
schema.

## Watchouts

- Accepted-route coverage is deferred because `PreparedVariadicEntryPlanFunction`
  currently has no table describing source incoming GPR, post-named argument
  position, destination save/overflow slot range, payload size, or alignment.
- Duplicate, mismatched, and out-of-area saved-GPR publication shapes are also
  deferred for the same reason; encoding them through free-form
  `missing_required_facts` strings would not test the selected route.
- The added test does not hard-code `va-arg-13.c`, `test`, `dummy`, literal
  `1234`, `a1`, or offset 72 as route predicates.

## Proof

Delegated proof command:

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
- Result: PASS, 326/326 backend tests.
- Proof log: `test_after.log`.
