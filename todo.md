# Current Packet

Status: Active
Source Idea Path: ideas/open/148_same_block_load_local_stored_value_owner.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Move the API into the addressing owner

## Just Finished

Step 2 moved the same-block load-local stored-value API declarations into the
addressing owner without changing lookup behavior.

- `PreparedSameBlockLoadLocalStoredValueSource` now lives in
  `src/backend/prealloc/addressing.hpp` next to the same-block global load
  access facts and prepared memory-access declarations.
- `find_prepared_same_block_load_local_stored_value_source` is now declared
  from `src/backend/prealloc/addressing.hpp` with the existing signature.
- `src/backend/prealloc/prepared_lookups.hpp` no longer declares the struct or
  finder directly.
- `src/backend/prealloc/prepared_lookups.cpp` was left behavior-identical; the
  existing definition still reuses `find_prepared_same_block_scalar_producer`,
  `find_prepared_memory_access`, and the prepared stack-layout/range helpers.
- AST checks confirmed the moved signature is emitted from `addressing.hpp`,
  absent as a direct declaration from `prepared_lookups.hpp`, and still defined
  in `prepared_lookups.cpp`.

## Suggested Next

Execute Step 3 by rewiring direct consumers to include the narrow addressing
owner where they need this moved API. Keep any consumer that still requires
broader prepared lookup declarations on its existing include.

## Watchouts

- The only known direct consumer from Step 1 is
  `src/backend/mir/aarch64/codegen/calls.cpp`; check whether it still needs
  unrelated `prepared_lookups.hpp` declarations before changing its include.
- `prepared_lookups.hpp` still includes `addressing.hpp`, so stale transitive
  access can mask missing narrow consumer includes until Step 3.
- Do not change AArch64 emission/register/scratch/extension handling while
  rewiring includes.

## Proof

Ran `c4c-clang-tool function-signatures` on
`src/backend/prealloc/addressing.hpp` and
`src/backend/prealloc/prepared_lookups.hpp`, plus
`c4c-clang-tool-ccdb find-definition` on
`src/backend/prealloc/prepared_lookups.cpp` for
`find_prepared_same_block_load_local_stored_value_source`.

Ran delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
Result: passed. Regression guard passed with 179/179 backend tests before and
after, no new failures.
Proof log: accepted after-proof was rolled forward to `test_before.log`;
there is no current root `test_after.log`.
