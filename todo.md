Status: Active
Source Idea Path: ideas/open/272_prealloc_schema_header_decomposition.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Tighten includes and prove behavior preservation

# Current Packet

## Just Finished

Step 5 finished by verifying the decomposed prealloc headers do not depend on
`prealloc.hpp` as an include-order crutch and by tightening only obvious nearby
prealloc include sites.

No focused header under `src/backend/prealloc/*.hpp` includes the compatibility
umbrella. `prepared_printer.hpp` now includes `module.hpp`,
`target_register_profile.hpp` now includes the owning `regalloc.hpp`
dependency, and `stack_layout/stack_layout.hpp` now includes only BIR for its
inline type helpers. Stack-layout implementation files now include
`../module.hpp` directly where they need prepared schema types.

## Suggested Next

Ask the plan owner to decide whether the active runbook is complete, should be
closed, or needs a final lifecycle review step.

## Watchouts

- Keep this plan behavior-preserving.
- Do not change prepared output, public type names, or test expectations.
- Keep `prealloc.hpp` as the compatibility umbrella.
- Do not split implementation bodies as part of the schema header step.
- Include tightening was intentionally limited to local prealloc headers and
  stack-layout implementation include sites. Broader backend consumer migration
  remains out of scope.
- A standalone header probe compiled each decomposed prealloc header, the
  compatibility umbrella, `prepared_printer.hpp`,
  `target_register_profile.hpp`, and `stack_layout/stack_layout.hpp` as the
  first include in a temporary translation unit.

## Proof

Ran delegated proof:

`bash -o pipefail -c 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '\''^backend_'\''' > test_after.log 2>&1`

Result: passed. Build completed and 139/139 backend tests passed.
Proof log: `test_after.log`.

Additional local compile-safety probe:

`/usr/bin/c++ -std=gnu++17 -I/workspaces/c4c/src -x c++ -c`

Result: passed for each focused prealloc header, `prealloc.hpp`,
`prepared_printer.hpp`, `target_register_profile.hpp`, and
`stack_layout/stack_layout.hpp` when included first in a temporary translation
unit.
