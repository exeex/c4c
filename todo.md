# Execution State

Status: Active
Source Idea Path: ideas/open/68_prepared_local_slot_handoff_consumption_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Idea-68 Ownership And Confirm The Next Local-Slot Seam
Plan Review Counter: 1 / 4
# Current Packet

## Just Finished

Step `1` confirmed that `c_testsuite_x86_backend_src_00204_c` still belongs to
idea 68 and narrows the next owned seam to `myprintf`'s first authoritative
multi-block `vaarg` join/local-slot handoff. Fresh `--trace-mir`,
`--dump-prepared-bir`, and backend asm repros show the route still keeps the
focused `backend_cli_trace_mir_00204_myprintf_rejection` guardrail green while
the full case now dies later on the local-slot `instruction` handoff after
`myprintf` publishes authoritative prepared control-flow blocks and branches
into the first `vaarg.amd64.reg.16|stack.17 -> vaarg.amd64.join.18` join path.

## Suggested Next

Run Step `2.2` against one seam only: make
`src/backend/mir/x86/codegen/prepared_local_slot_render.cpp` consume the first
authoritative `vaarg` join-transfer/local-slot copy path in `myprintf`
generically, starting with the `vaarg.amd64.join.18` family and only extending
shared prepared control-flow/local-slot contract surface if the current
metadata is not expressive enough for normalized join-copy rendering.

## Watchouts

- Treat the focused `myprintf` rejection as a guardrail; do not accept a
  local-slot repair that breaks that route or regresses it back into idea-61
  module-shape ownership.
- Reject helper-topology or testcase-shaped x86 growth that only admits one
  bounded continuation/helper lane without consuming the prepared contract
  generally.
- The current failure surface is `instruction`, not `branch` or `return`; keep
  the next packet focused on unsupported authoritative join-copy/local-slot
  instruction consumption before touching later aggregate-print blocks.
- The prepared dump for `myprintf` already carries authoritative
  `branch_conditions` through the `vaarg` ladder, so re-deriving control-flow
  from raw CFG shape would be route drift.

## Proof

Latest delegated proof run:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_cli_trace_mir_00204_myprintf_rejection|c_testsuite_x86_backend_src_00204_c)$' | tee test_after.log`

Observed state for lifecycle routing:
`backend_cli_trace_mir_00204_myprintf_rejection` passed, and
`c_testsuite_x86_backend_src_00204_c` failed only with
`x86 backend emitter requires the authoritative prepared local-slot
instruction handoff through the canonical prepared-module handoff`. Supporting
inspection with `build/c4cll --dump-prepared-bir --target x86_64-unknown-linux-gnu --mir-focus-function myprintf tests/c/external/c-testsuite/src/00204.c`
showed the first authoritative multi-block local-slot join at
`vaarg.amd64.join.18`, which is the current Step `1` handoff target.
Proof log path: `test_after.log`.
