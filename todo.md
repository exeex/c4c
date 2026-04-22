# Execution State

Status: Active
Source Idea Path: ideas/open/60_scalar_expression_and_terminator_selection_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Prove Family Shrinkage And Record Rehoming
Plan Review Counter: 0 / 4
# Current Packet

## Just Finished

Step 2.2 repaired the `backend_prepare_liveness` regression from the prepared
pointer-home contract without reopening the retired compare-driven seam. The
regalloc/value-location contract now records helper-built aggregate byval and
aggregate `va_arg` destinations as stack-backed ABI consumption again, while
preserving the extra helper register binding needed for the prepared pointer
route. The focused `00204.c` dump/trace pair still lands `function match` on
`local-slot-guard-chain`, and the x86 handoff boundary keeps the recovered
downstream short-circuit rejection instead of slipping back to the older
authoritative-handoff failure.

## Suggested Next

Take the next packet on the remaining whole-module `00204.c` blockers outside
this repaired liveness/value-location contract. The focused route is still
stable on `local-slot-guard-chain`; the remaining downstream work is the other
unsupported helper families that still keep `c_testsuite_x86_backend_src_00204_c`
from assembling end to end.

## Watchouts

- Keep helper-built aggregate byval and aggregate `va_arg` destinations modeled
  as stack-consumed ABI traffic even when x86 still needs an auxiliary pointer
  register to materialize the copy. The stack destination is the semantic
  contract; the helper register is a separate binding.
- Do not broaden the short-circuit continuation check into a blanket ban on
  `i8` select materialization. `match` still depends on an authoritative
  `select_materialization` join with `i8` source compares, but it stays valid
  because it routes through the compare-join fallback instead of the dedicated
  rhs-continuation carrier path.
- Remaining `00204.c` blockers are downstream of this packet. Keep them routed
  as separate helper-family gaps rather than folding them back into the retired
  `compare-driven-entry` gate.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_x86_route_debug|backend_x86_handoff_boundary|backend_cli_dump_mir_00204_match_rejection|backend_cli_trace_mir_00204_match_rejection)$'`.
Result: all five delegated checks passed. `backend_prepare_liveness` again
publishes stack-backed ABI consumption for the helper-built aggregate byval and
aggregate `va_arg` cases, while `backend_x86_handoff_boundary` and the focused
`00204.c` dump/trace pair still preserve the accepted `local-slot-guard-chain`
route and recovered downstream short-circuit boundary. Preserved proof log:
`test_after.log`.
