Status: Active
Source Idea Path: ideas/open/400_rv64_object_route_local_memory_addressing_edges.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Classify Local-Memory Addressing Forms

# Current Packet

## Just Finished

Lifecycle transition only. Idea 401 closed after scalar compare/trunc and
floating-cast diagnostics were satisfied or split; idea 400 is now active for
the separate local-memory/addressing blocker.

## Suggested Next

Classify Step 1 local-memory representatives, including the inherited
`src/20020225-2.c` failure that now reports `unsupported_local_memory_access`.

## Watchouts

- Do not reopen idea 401's scalar compare/trunc or floating-cast work here.
- Treat `src/20020225-2.c` as local-memory/addressing evidence only after the
  floating cast producer has already lowered.
- Route wide rematerialized-immediate producer admission to
  `ideas/open/404_prepared_wide_rematerialized_immediate_admission.md`.
- Route scalar `ashr`/bitfield-global lowering to the existing instruction
  fragment idea, `ideas/open/395_rv64_object_route_instruction_fragment_lowering.md`.

## Proof

Close gate for idea 401:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R backend > test_after.log && python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log
```

Result: build was up to date; regression guard passed with `test_before.log`
326/326 and regenerated `test_after.log` 327/327, with no new failing tests.
