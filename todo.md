# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 3.3
Current Step Title: Narrow Prepared Debug And Admission Surfaces To Observational Adapters
Plan Review Counter: 5 / 6
# Current Packet

## Just Finished

Completed step 3.3 by moving the prepared call-ABI selector/result declaration
surface out of `x86_codegen.hpp` into a new
`lowering/call_abi_queries.hpp` seam, then rewiring the transitional shared
header to consume that lower-level declaration owner instead of housing those
prepared-only compatibility holdouts directly.

## Suggested Next

Continue with the next bounded 3.3 packet by identifying any remaining
prepared-only declaration clusters still owned by `x86_codegen.hpp` that are
only consumed through dedicated `lowering/` or `prepared/` seams, then move
one coherent cluster behind the owning header without disturbing the inline
bounded multi-defined helpers.

## Watchouts

- `x86_codegen.hpp` still needs the stack/local-slot operand declarations and
  the prepared call-ABI query include because its inline bounded multi-defined
  helpers call those surfaces directly; do not strip either until those inline
  holdouts move.
- Keep `lowering/call_abi_queries.hpp` limited to the prepared call-bundle
  query/result declaration surface; do not let broader call-lowering
  implementation ownership drift into that header.

## Proof

Step 3.3 prepared call-ABI declaration seam packet on 2026-04-23:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
