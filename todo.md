# Execution State

Status: Active
Source Idea Path: ideas/open/69_long_double_aggregate_asm_emission_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Repair The Selected Long-Double Aggregate Asm-Emission Seam
Plan Review Counter: 0 / 4
# Current Packet

## Just Finished

Step `2.1` under idea 69 cleared the remaining owned long-double asm-emission
seam: `cast_ops.cpp` and the prepared `f128` local-copy/render path now also
emit Intel-syntax `fld` / `fstp`, the nearest handoff-boundary `f128` snippets
were updated to the same contract, and the generated `00204.c.s` no longer
contains `fldt` / `fstpt`.
The packet stops here because the proof now fails on the first downstream
non-seam blockers instead of on invalid long-double mnemonics.

## Suggested Next

Start a new bounded packet outside this seam for the first post-asm-emission
blocker: either the stale contract expectation in
`backend_x86_handoff_boundary_multi_defined_call_test.cpp` for the
global-function-pointer indirect variadic-runtime boundary, or the new
`00204.c` link-stage undefined-reference surface if that is the chosen
execution route.

## Watchouts

- Keep the accumulated `fld` / `fstp` swap; the long-double asm-emission seam
  is now clean across the owned and previously patched emitters.
- A direct `rg` over `src/backend/mir/x86/codegen` and the generated
  `build/c_testsuite_x86_backend/src/00204.c.s` finds no remaining
  `fldt` / `fstpt` occurrences.
- `backend_x86_handoff_boundary` now fails on a different contract message:
  `multi-defined global-function-pointer and indirect variadic-runtime boundary`
  rejects with the wrong out-of-scope contract text.
- `c_testsuite_x86_backend_src_00204_c` now gets past assembly and fails later
  at link time with undefined references such as `s1`..`s16` and
  `llvm.va_start.p0`.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$' | tee test_after.log`
ran and failed, but no longer on invalid long-double mnemonics.
`backend_x86_handoff_boundary` now fails on a stale wrong-contract expectation
for the global-function-pointer indirect variadic-runtime boundary, and
`c_testsuite_x86_backend_src_00204_c` now fails later at link time on undefined
references. Proof log path: `test_after.log`.
