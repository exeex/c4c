# Execution State

Status: Active
Source Idea Path: ideas/open/60_scalar_expression_and_terminator_selection_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Prove Family Shrinkage And Record Rehoming
Plan Review Counter: 1 / 4
# Current Packet

## Just Finished

Step 2.2 now has fresh proof that the direct-immediate family shrank beyond
`00023.c` and `00024.c`. The straight-line constant-folded single-block return
route now consumes authoritative no-parameter `i8` string-constant loads plus
`i8 -> i32` sign extension, which moves
`c_testsuite_x86_backend_src_00026_c` past the old direct-immediate rejection
without perturbing the existing same-module global return routes. A focused
recheck of `c_testsuite_x86_backend_src_00025_c` shows that it no longer
belongs to idea 60 either: codegen now succeeds and the case fails later as a
runtime bug (`strlen("hello") - 5` returns nonzero at runtime), so it should
stay out of the scalar-emitter proof set.

## Suggested Next

Return to Step 1 and pick the next still-owned direct-immediate case after
`00026.c`, keeping `backend_x86_handoff_boundary` plus `00023.c`, `00024.c`,
and `00026.c` as the protective regression subset. Do not spend the next
packet on `00025.c`; it has already graduated downstream into runtime
debugging and should only be mentioned as a rehomed case, not treated as the
next scalar-emitter seam.

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
- `00025.c` has now joined that downstream runtime bucket. Keep it out of the
  acceptance regex even though it remains useful as rehoming evidence.
- The accepted fix depends on authoritative prepared addressing plus the
  existing single-block renderer. Do not regress back to a global-name or
  aggregate-layout-specific fast path when extending the remaining direct
  return family.

## Proof

Accepted packet proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00023_c|c_testsuite_x86_backend_src_00024_c|c_testsuite_x86_backend_src_00026_c)$'`.
Supporting inspection used
`./build/c4cll --dump-prepared-bir --target x86_64-unknown-linux-gnu tests/c/external/c-testsuite/src/00026.c`,
`./build/c4cll --trace-mir --target x86_64-unknown-linux-gnu tests/c/external/c-testsuite/src/00026.c`,
and an explicit rehoming probe with
`ctest --test-dir build -j --output-on-failure -R '^c_testsuite_x86_backend_src_00025_c$'`.
Result: `backend_x86_handoff_boundary`,
`c_testsuite_x86_backend_src_00023_c`,
`c_testsuite_x86_backend_src_00024_c`, and
`c_testsuite_x86_backend_src_00026_c` now pass; `00025.c` fails later at
runtime instead of at scalar emission; `test_after.log` holds the
acceptance-ready narrow proof command.
