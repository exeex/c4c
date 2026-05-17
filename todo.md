Status: Active
Source Idea Path: ideas/open/272_prealloc_schema_header_decomposition.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract names and base declarations

# Current Packet

## Just Finished

Step 2 extracted the low-level prepared names/base declaration block into
`src/backend/prealloc/names.hpp`.

`names.hpp` now owns the prepared ID aliases, `PrepareOptions`, `PrepareNote`,
`PreparedNameTables`, the low-level `PreparedBirModule` forward declaration,
and the prepared name spelling/intern helper functions.

`src/backend/prealloc/prealloc.hpp` now includes `names.hpp` as the
compatibility umbrella and still declares `infer_call_arg_abi`, `BirPreAlloc`,
and the prepare entry APIs.

## Suggested Next

Execute Step 3 from `plan.md`: extract the frame, addressing, liveness,
regalloc, and value-location schema families into focused headers while keeping
`prealloc.hpp` as the public compatibility umbrella.

## Watchouts

- Keep this plan behavior-preserving.
- Do not change prepared output, public type names, or test expectations.
- Keep `prealloc.hpp` as the compatibility umbrella.
- Do not split implementation bodies as part of the schema header step.
- `names.hpp` intentionally forward-declares `PreparedBirModule`; the concrete
  aggregate should move later with `module.hpp` after member-family headers
  exist.
- Ambiguous boundary: `PreparedStorageEncodingKind` is introduced just before
  call plans but is reused by storage plans and indirect callee plans; extract
  with `calls.hpp` only if later includes stay acyclic, otherwise use the
  lowest shared focused header that avoids recreating a helper monolith.
- Ambiguous boundary: register bank/placement types currently live with frame
  and saved-register schema but are reused broadly by regalloc, value homes,
  calls, carriers, and runtime helpers; move them before dependents or isolate
  as part of the low-level frame/register-placement family.
- Ambiguous boundary: control-flow helpers include substantial inline analysis
  over BIR and prepared name tables; keep them together initially rather than
  distributing tiny helpers across unrelated family headers.

## Proof

Ran delegated proof:

`bash -o pipefail -c 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '\''^backend_'\''' > test_after.log 2>&1`

Result: passed. Build completed and 139/139 backend tests passed.
Proof log: `test_after.log`.
