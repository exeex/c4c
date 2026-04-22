# Execution State

Status: Active
Source Idea Path: ideas/open/60_scalar_expression_and_terminator_selection_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Prove Family Shrinkage And Record Rehoming
Plan Review Counter: 0 / 4
# Current Packet

## Just Finished

Step 2.1 repaired the direct-immediate single-block return seam for named
non-parameter `i32` values that already carry authoritative prepared homes and
move-bundle facts. The dispatcher now falls through to the existing generic
single-block renderer after the older fast paths, the shared i32 binary helper
preserves the authoritative lhs carrier when the rhs is currently in `eax`,
and same-module scalar load/store selection now accepts authoritative in-bounds
`i32` accesses over zero-initialized aggregate storage instead of requiring a
scalar-typed global declaration. That moves both
`c_testsuite_x86_backend_src_00023_c` and `c_testsuite_x86_backend_src_00024_c`
past the old direct-immediate rejection without adding a testcase-shaped
global-load lane.

## Suggested Next

Step 2.2 should prove that the family really shrank beyond the two repaired
cases. Keep `backend_x86_handoff_boundary` plus `00023.c` and `00024.c` as the
protective route checks, then add the nearest remaining same-family direct
return cases such as `00025.c` or `00026.c` to confirm whether they now pass
or expose the next still-generic scalar seam. Record any cases that graduate
out of idea 60 into a later leaf instead of silently keeping them in this
family.

## Watchouts

- Do not special-case `load_global` or one named testcase. The prepared dump
  for `00023.c` already shows the generic facts the dispatcher should consume:
  named value-home ownership plus one authoritative `before_return` move
  bundle into the return ABI register.
- `backend_x86_route_debug` can still report a matched exploratory lane even
  when `--codegen asm` ends in the direct-immediate rejection. Acceptance for
  the next packet still needs real asm/codegen proof, not only trace output.
- `00035.c` and `00041.c` currently fail on the authoritative prepared
  guard-chain handoff and should not be used as idea-60 proof targets.
- `00019.c` has graduated out of the scalar-emitter family into a runtime bug;
  do not use it as the primary idea-60 proof case.
- The accepted fix depends on authoritative prepared addressing plus the
  existing single-block renderer. Do not regress back to a global-name or
  aggregate-layout-specific fast path when extending the remaining direct
  return family.

## Proof

Accepted packet proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00023_c|c_testsuite_x86_backend_src_00024_c)$'`.
Supporting inspection used
`./build/c4cll --dump-prepared-bir --target x86_64-unknown-linux-gnu tests/c/external/c-testsuite/src/00024.c`,
`./build/c4cll --trace-mir --target x86_64-unknown-linux-gnu tests/c/external/c-testsuite/src/00024.c`,
and direct asm probes for `00024.c`. Result: `backend_x86_handoff_boundary`,
`c_testsuite_x86_backend_src_00023_c`, and
`c_testsuite_x86_backend_src_00024_c` now pass; `test_after.log` holds the
acceptance-ready narrow proof command.
