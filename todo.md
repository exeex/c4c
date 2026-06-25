Status: Active
Source Idea Path: ideas/open/358_rv64_object_route_abi_width_edges.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Local Memory Width and Home Edges

# Current Packet

## Just Finished

Completed Plan Step 3's local-memory width slice. RV64 object emission now
accepts 1- and 2-byte scalar local memory widths for prepared frame-slot
base-plus-offset local accesses, reusing the existing semantic
`PreparedMemoryAccess` checks instead of testcase matching. The focused i16
local-store fixture now builds and verifies the emitted `sh` stack-store
sequence; existing 32- and 64-bit local memory behavior remains covered by the
same object-emission test binary.

## Suggested Next

Run the representative validation for the local-memory boundary and decide the
next Step 3 packet from the new `src/20001203-1.c` result:

```sh
tmp=$(mktemp); printf 'src/20001203-1.c\n' > "$tmp"; ALLOWLIST="$tmp" CASE_TIMEOUT_SEC=20 STOP_ON_FAILURE=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log; rc=$?; rm -f "$tmp"; exit $rc
```

## Watchouts

- The local-memory address predicate remains intentionally narrow: default
  address space, non-volatile, frame-slot base, published frame-slot id,
  base-plus-offset eligibility, matching access size, compatible alignment,
  non-negative offset, and signed 12-bit immediate reach.
- Narrow local loads use the same stack-load helper and therefore encode signed
  `lb`/`lh`; add an explicit load fixture before changing signedness behavior.
- `src/20030125-1.c` remains out of scope for this packet and should still be
  classified separately before any FPR or floating-operation implementation.

## Proof

Delegated proof:

```sh
cmake --build build --target backend_riscv_object_emission_test && ctest --test-dir build -R '^backend_riscv_object_emission$' --output-on-failure > test_after.log
```

Result: passed, 1/1 focused tests. Proof log: `test_after.log`.

Additional local check:

```sh
git diff --check
```

Result: passed.
