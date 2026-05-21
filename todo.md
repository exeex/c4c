Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize the Current Call-Boundary Gap

# Current Packet

## Just Finished

Step 1 localized the current `00140` / `00204` call-boundary gap without
implementation edits.

First bad facts:

- `00140`: `main` lowers `f1(f, &f, ...)` as byval pointer `%lv.f` but the
  prepared AArch64 call plan publishes `%lv.f` as ordinary GPR register
  arguments (`x0`, `x1`, and for the variadic call also `x4`, `x5`). Generated
  `f1` then reads the byval aggregate from its incoming stack/formal frame
  (`ldr ... [sp]`, `[sp,#4]`, ...), so the caller/callee boundary disagrees on
  whether the first aggregate lives behind the byval pointer or has already
  been materialized into the callee byval slot. Owning boundary: AArch64
  prepared call-boundary aggregate/byval argument publication plus callee
  byval-entry consumption, not CTest, runner, or expectation state.
- `00204`: the current pre-runtime stop is the machine-printer diagnostic at
  `stdarg` call `block_index=0 inst_index=491`: `deferred_unsupported:
  call-boundary move node requires prepared GPR registers, scalar FPR
  registers, or structured f128 q-register authority`. The prepared call plan
  has HFA long-double/f128 lanes such as value ids `2481`-`2483` moving to
  `q0`-`q2`, but their storage/carrier placements are still `fpr:* reg=d13`,
  `d20`, `d21` while the call destination is `vreg:* reg=q*`. Later overflow
  lanes (`2504`/`2516`) are memory-backed and correctly classified as stack
  destinations, so the first unsupported node is the register call-boundary
  q-register authority mismatch, not the historical overflow cursor.

They share one repair owner at the AArch64 prepared call-boundary
move/publication boundary, but Step 2 coverage should use two focused fixtures:
one for byval/composite GPR lane publication versus callee byval entry, and
one for structured f128/HFA q-register authority. Do not split into old
idea-326 residual owners yet.

## Suggested Next

Execute Step 2 from `plan.md`: add focused backend coverage for the localized
AArch64 call-boundary owner, with separate assertions for `00140`-style byval
aggregate publication/entry consumption and `00204`-style f128 q-register
authority.

## Watchouts

Rejected adjacent historical owners with current evidence:

- Local/value-home publication: `00140` reaches generated assembly and the bad
  fact is between the prepared call plan and callee byval entry; `00204` fails
  before runtime in a call-boundary move selection diagnostic, not at ordinary
  local publication.
- Fixed pointer-formal entry publication: the old `myprintf` `%p.format`
  pointer-formal failure is not current evidence; `00204` is stopped in the
  caller-side `stdarg` before-call HFA/f128 move bundle, and `00140` is byval
  aggregate entry, not an ordinary pointer formal home.
- Byval lane, stdarg cursor/format, MOVI zero-extension, previous HFA output,
  f128 transport addressability, and large-frame/stack spelling: the current
  logs do not show those runtime mismatches. `00140` segfaults at the
  composite call boundary, and `00204` stops at selected call-boundary move
  authority before any runtime output.
- Expectation or unsupported classification work would be testcase-overfit;
  both representatives remain supported-path failures with generated backend
  evidence.

## Proof

Ran:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00140_c|c_testsuite_aarch64_backend_src_00204_c)$' > test_after.log 2>&1
```

Result: build succeeded with no work to do; focused CTest failed as expected
for the localized no-edit slice. `test_after.log` records `00140` as
`RUNTIME_NONZERO` / segmentation fault and `00204` as `FRONTEND_FAIL` at the
structured call-boundary move authority diagnostic.
