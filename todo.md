Status: Active
Source Idea Path: ideas/open/608_prepared_global_data_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair the shared prepared global-memory fact rule

# Current Packet

## Just Finished

- Step 2 repaired prepared/prealloc global layout-authority publication for
  resolved byte-storage aggregate globals. The new rule publishes
  `layout_authority=byte_storage_aggregate` only for non-external, non-TLS,
  resolved `I8` global objects with a valid link identity, concrete size and
  alignment, complete extent, matching global-symbol provenance, and an
  in-bounds requested range.
- `src/pr36034-1.c` moved past the prior producer-side stop: prepared dumps now
  show the direct `base=global_symbol` rows for `x` and `tmp` carrying
  `layout_authority=byte_storage_aggregate` instead of
  `layout_authority=unknown`.
- `src/pr91137.c` remains consistent with the prior classification: scalar rows
  carry scalar layout and aggregate rows for `c`/`d` carry
  byte-storage aggregate layout.
- Direct RV64 object probes for both residual rows still fail with the same
  coarse message,
  `unsupported_global_data: RV64 object route requires supported prepared global
  memory facts`, after the producer fact is present. That makes the remaining
  stop downstream of the repaired prepared layout-authority publication, most
  likely in RV64 global load/store consumer admission or a missing value
  location/move fact consumed by that path.

## Suggested Next

- Step 3 packet: classify the remaining post-producer stop for
  `src/pr36034-1.c` and `src/pr91137.c` by tracing the first
  `fragment_for_prepared_load_global` or `fragment_for_prepared_store_global`
  rejection after prepared accesses have byte-storage/scalar layout authority.
  Keep the packet read-only first unless the owner is clearly still within
  prepared global-memory admission; do not edit RV64 emission without supervisor
  approval.
- Suggested proof command if Step 3 stays diagnostic-only:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

## Watchouts

- The new publication rule intentionally excludes extern globals, TLS globals,
  unresolved labels, scalar-layout globals, integer-array globals, missing
  extent, unknown or conflicting prior layout authority, out-of-bounds ranges,
  and non-global storage.
- `prepared_global_symbol_memory_has_publication_authority(...)` remains
  fail-closed for `layout_authority=unknown`; this packet changes only the
  producer that can prove byte-storage aggregate authority before that
  predicate is queried.
- The direct object probes still surface the old coarse diagnostic, so the next
  packet should not assume the remaining owner from the diagnostic string alone.

## Proof

- Focused movement probes:
  `./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu
  tests/c/external/gcc_torture/src/pr36034-1.c` wrote
  `build/agent_state/608_step2_global_memory/pr36034-1.current.prepared.txt`
  and confirmed `x`/`tmp` rows now carry
  `layout_authority=byte_storage_aggregate`.
- Direct object probes for `src/pr36034-1.c` and `src/pr91137.c` still fail at
  RV64 object compilation with
  `unsupported_global_data: RV64 object route requires supported prepared global
  memory facts`; logs are under
  `build/agent_state/608_step2_global_memory/*.current.obj.err`.
- Delegated proof run:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

- Result: passed, 346 backend tests, 0 failures. Proof log path:
  `test_after.log`.
