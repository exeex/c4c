Status: Active
Source Idea Path: ideas/open/555_rv64_prepared_local_memory_addressing_src_960209_1.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement The Owned Addressing Shape

# Current Packet

## Just Finished

Step 2 - Implement The Owned Addressing Shape completed the RV64 object-route
consumer slice for prepared pointer-value local memory facts.

Implementation details:

- `object_emission.cpp` now keeps the strict block/instruction prepared memory
  access lookup first, then falls back only to a unique prepared access in the
  same block whose authoritative result/stored value name matches the BIR
  local load/store.
- The local load/store consumer and local-memory diagnostic paths use the same
  prepared-fact lookup, so a prepared access with shifted carrier index is not
  treated as missing while raw/testcase shape remains ignored.
- Focused RV64 object coverage now proves a named `i8` store through a
  prepared pointer-value base in `s1`, stored byte in `t0`, offset `0`,
  `size=1 align=1`, emitting `mv t1, t0; sb t1, 0(s1); ret`.
- Focused fail-closed coverage preserves rejection when the pointer-value fact,
  pointer register home, or base-plus-offset authority is removed.

## Suggested Next

Execute Step 3 by running the one-row `src/960209-1.c` RV64 gcc torture
backend scan, confirming that `unsupported_local_memory_access` is fixed for
the repaired operation, and recording the newly exposed first blocker without
silently expanding this local-memory plan into a move-bundle route.

## Watchouts

- The delegated backend proof passed. A manual one-row
  `src/960209-1.c` CMake harness probe, run outside the delegated proof log,
  progressed past the local-memory diagnostic and then exposed
  `prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`.
  That is a separate move-bundle classifier issue, not the owned pointer-value
  byte-store addressing shape.
- The new fallback intentionally requires a unique prepared access with the
  same prepared block label and matching result/stored value name; it does not
  infer an address from raw local slot spelling, testcase name, source block, or
  BIR address carrier shape.
- The original strict block/instruction lookup still wins when present.

## Proof

Proof log: `test_after.log`.

Commands run:

```sh
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: passed. `ctest` reported `100% tests passed, 0 tests failed out of
345`; `backend_riscv_object_emission` passed with the new focused RV64 object
byte-store coverage.
