# Execution State

Status: Active
Source Idea Path: ideas/open/60_scalar_expression_and_terminator_selection_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Idea-60 Ownership And Confirm The Next Scalar Seam
Plan Review Counter: 1 / 4
# Current Packet

## Just Finished

Step 1 refreshed current idea-60 ownership with fresh narrow x86 probes and
replaced stale sample cases. `c_testsuite_x86_backend_src_00007_c`,
`00008.c`, `00034.c`, `00036.c`, `00066.c`, `00101.c`, and `00102.c` now
pass. `c_testsuite_x86_backend_src_00019_c` no longer stops on the scalar
emitter restriction and has moved to a runtime segfault, so it is not the next
idea-60 seam. The current direct-immediate return family still holds on
`00023.c`, `00024.c`, `00025.c`, `00026.c`, `00056.c`, `00062.c`,
`00067.c`, `00068.c`, `00069.c`, `00070.c`, and `00107.c`; the current
minimal single-block terminator family still holds on `00045.c`, `00058.c`,
`00081.c`, and `00082.c`. `00023.c` is the cleanest next packet: prepared MIR
already publishes a named `%t0` global-load value home plus a single
`before_return` move bundle to `rax`, but x86 still rejects it under the
direct-immediate-only return dispatcher.

## Suggested Next

Step 2.1 should repair one generic single-block scalar return seam for named
non-parameter `i32` values that already have authoritative prepared value-home
and single `before_return` move-bundle data. Start with
`c_testsuite_x86_backend_src_00023_c` as the primary proof case and keep the
repair at the return-selection layer rather than adding a global-load-only
lane. Use the nearest backend coverage around
`render_prepared_single_block_return_dispatch_if_supported` and keep `00024.c`
or another nearby direct-immediate family case as the same-family follow-on
check.

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

## Proof

Ran ownership-refresh probes:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_route_debug|backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00007_c|c_testsuite_x86_backend_src_00019_c)$'`
and
`ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_x86_backend_src_(00008|00034|00035|00036|00041|00045|00058|00066|00081|00082|00101|00102|00023|00024|00025|00026|00056|00062|00067|00068|00069|00070|00107)_c)$'`.
Follow-up inspection used
`./build/c4cll --dump-prepared-bir --target x86_64-unknown-linux-gnu tests/c/external/c-testsuite/src/00023.c`,
`./build/c4cll --trace-mir --target x86_64-unknown-linux-gnu tests/c/external/c-testsuite/src/00023.c`,
and direct asm probes for `00023.c` and `00045.c`. Result: the direct-immediate
family still fails in asm/codegen on the named non-parameter return path, the
minimal single-block terminator family still fails on separate cases, several
older sample cases now pass, and `00019.c` has moved to a runtime failure.
Preserved proof log: `test_after.log`.
