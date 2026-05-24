Status: Active
Source Idea Path: ideas/open/prealloc-publication-accessor-contracts.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Tighten Local Includes And Ownership Comments

# Current Packet

## Just Finished

Completed Step 2 from `plan.md`: tightened the proven local
publication/accessor include and ownership-comment issues without changing
behavior.

Changed files:

- `src/backend/prealloc/prepared_lookups.hpp`: removed the unused `<optional>`
  include after confirming the header does not directly use `std::optional`.
- `src/backend/prealloc/value_locations.hpp`: replaced stale `Step 5 fence`
  wording with durable prepared publication/accessor ownership comments for
  value homes, move bundles, parallel-copy step lookup, and unique move-bundle
  lookup.
- `src/backend/prealloc/value_locations.hpp`: replaced the live string-name
  accessor comment's stale `spelling bridge` / `legacy callers` wording with a
  durable text-name accessor boundary note.

Deferred candidates:

- `storage_plans.hpp` still includes `module.hpp` for
  `populate_storage_plans(PreparedBirModule&)`; leave any forward-declaration
  contraction for a separately delegated slice because `module.hpp` is an
  aggregate prealloc contract.
- `prepared_printer/private.hpp` still includes `../module.hpp`; leave printer
  private aggregate contraction deferred because it is shared by non-target
  printer declarations.

## Suggested Next

Execute Step 3 from `plan.md`: perform a narrow contract naming and helper
grouping pass only where local call sites prove a publication/accessor name or
grouping is stale. Start with the target package headers and avoid public
renames that would require broad caller churn.

## Watchouts

- Keep the route behavior-preserving; no publication, storage, decoded-home,
  value-location, or prepared dump semantics changed in Step 2.
- Do not split `module.hpp`, `regalloc.hpp`, or broad aggregate prealloc
  contracts as part of naming cleanup.
- Keep compatibility comments or adapters when call sites still depend on the
  old aggregate prepared-module view.
- `value_locations.hpp` still requires `<optional>` for value-home fields,
  block-entry publication data, and inline optional-return helpers.

## Proof

Ran delegated proof:

`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`

Result: passed; build completed and 162/162 backend tests passed.

Ran `git diff --check`: passed.

Proof log: `test_after.log`.
