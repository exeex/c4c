Status: Active
Source Idea Path: ideas/open/358_rv64_object_route_abi_width_edges.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Local Memory Width and Home Edges

# Current Packet

## Just Finished

Completed Plan Step 3's representative validation for `src/20001203-1.c` after
the local-memory width slice. The temporary allowlist harness now passes this
case, so the previous `unsupported_local_memory_access` diagnostic is gone and
no later structured boundary appeared for this representative.

## Suggested Next

Use the remaining Step 1 audit failure, `src/20030125-1.c`, to choose the next
ABI-width edge packet. Start by classifying its current generic
`unsupported_instruction_fragment` into a precise backend boundary before
changing lowering behavior.

Suggested proof shape for the next classification packet: rerun only
`src/20030125-1.c` through the same temporary allowlist harness and preserve
the resulting `test_after.log` plus case log.

## Watchouts

- The local-memory address predicate remains intentionally narrow: default
  address space, non-volatile, frame-slot base, published frame-slot id,
  base-plus-offset eligibility, matching access size, compatible alignment,
  non-negative offset, and signed 12-bit immediate reach.
- Narrow local loads use the same stack-load helper and therefore encode signed
  `lb`/`lh`; add an explicit load fixture before changing signedness behavior.
- `src/20001203-1.c` is now green in the representative harness; do not keep
  expanding this packet around that testcase.
- `src/20030125-1.c` remains out of scope for this packet and should still be
  classified separately before any FPR or floating-operation implementation.

## Proof

Delegated proof:

```sh
tmp=$(mktemp); printf 'src/20001203-1.c\n' > "$tmp"; ALLOWLIST="$tmp" CASE_TIMEOUT_SEC=20 STOP_ON_FAILURE=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log; rc=$?; rm -f "$tmp"; exit $rc
```

Result: passed, 1/1 representative case. Proof log: `test_after.log`.
Case log: `build/rv64_gcc_c_torture_backend/src_20001203-1.c/case.log`.

Additional local check:

```sh
git diff --check -- todo.md
```

Result: passed.
