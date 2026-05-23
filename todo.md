Status: Active
Source Idea Path: ideas/open/aarch64-codegen-cpp-family-consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Consolidate Calls CPP Helpers

# Current Packet

## Just Finished

Completed `plan.md` Step 3 by merging the call effects helper definitions from
`src/backend/mir/aarch64/codegen/calls_effects.cpp` into
`src/backend/mir/aarch64/codegen/calls_printing.cpp`.
Deleted the obsolete helper `.cpp`, removed its explicit
`src/backend/CMakeLists.txt` source-list entry, and trimmed
`src/backend/mir/aarch64/codegen/calls.hpp` so only
`effects_from_prepared_call_clobbers`, the call-effect API still used by other
translation units, remains public.

Post-merge scans found no remaining source-list or implementation references to
the removed `calls_effects.cpp`. A follow-up source-reference scan confirmed
`effects_from_prepared_call_preserved_values` is used only by
`calls_printing.cpp`, so it now has internal linkage there.

## Suggested Next

Execute `plan.md` Step 4: re-scan the AArch64 codegen `.cpp` layout, record
which remaining tiny call or dispatch helper files are intentionally retained
or out of scope, and run the supervisor-selected broader validation or
regression guard if required by the accumulated consolidation slices.

## Watchouts

- Keep the source idea stable unless durable intent changes.
- Do not merge large semantic lowering modules just to reduce file count.
- Do not hide target-independent logic in AArch64 during consolidation.
- Keep slices behavior-preserving and reviewable.
- `effects_from_prepared_call_clobbers` remains public because `f128.cpp` and
  `i128_ops.cpp` still consume it; `effects_from_prepared_call_preserved_values`
  and the single-record effect helpers are local to `calls_printing.cpp`.
- Remaining calls modules such as call argument sources, moves, preservation,
  and byval aggregates still look like durable semantic boundaries unless the
  Step 4 review proves otherwise.

## Proof

Proof command run and passed:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log
```

Result: build passed; `^backend_` CTest subset passed 149/149 tests.

Proof log: `test_after.log`.
