# LIR To BIR Local Memory Admission Todo

Status: Active
Source Idea Path: ideas/open/297_lir_to_bir_local_memory_admission.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Check Store/Load Boundaries

# Current Packet

## Just Finished

Step 3 diagnosis packet reclassified the remaining `00216` GEP residual outside
idea 297's local-memory admission owner. The first rejected GEP in `foo` is
`%t381 = getelementptr %struct.W, ptr %p.w, i32 0, i32 0`, which comes from the
`w->t.s` path over function parameter `%p.w`, not from a local alloca/local
slot. The LLVM route shows `%struct.W = type { %struct.V, [0 x %struct.S] }`,
so this residual is a pointer-parameter aggregate projection case involving a
flexible-array-shaped pointee layout, not a Step 3 local store/load boundary.

No semantic local-memory code change was made for `00216`; forcing one here
would drift into pointer-parameter/flexible-array aggregate GEP admission rather
than repairing the active local-memory owner.

## Suggested Next

Lifecycle decision: Step 3's focused boundary cases are now either passing or
classified outside idea 297: `00046` passes, `00140` and `00218` are later
AArch64 printer residuals, and `00216` is pointer-parameter/flexible-array
aggregate GEP admission. The next coherent packet is Step 4 focused-owner proof
or lifecycle review for closing/retiring this runbook, at supervisor discretion.

## Watchouts

- Direct dynamic scalar local-array admission is no longer the blocker for
  representatives `00143`, `00157`, and `00185`; do not conflate their new
  machine/runtime failures with the old GEP admission failure.
- Residual GEP-family cases still failing in the old diagnostic bucket:
  `00176`, `00181`, `00182`, `00195`, `00205`, `00209`; keep them out of Step 3
  unless a new source idea is activated.
- `00176`/`00181` are likely the first later split target because they share a
  plain global scalar-array dynamic index and avoid pointer-parameter
  provenance and nested aggregate projection.
- `00182` and `00209` likely belong to pointer-value GEP admission over pointer
  parameters or pointer-typed local slots; keep them separate from global array
  work.
- `00195` and `00205` likely belong to dynamic global aggregate projection,
  including scalar struct fields and nested member arrays; keep them separate
  from scalar global arrays until the simpler global scalar-array lane is
  proven.
- Store/load boundary checks: `00046`, `00140`, `00216`, `00218`.
- `00140` no longer proves the old aggregate parameter copy store boundary as
  the first blocker; after this packet it reaches a later AArch64
  call-boundary move printer failure.
- `00216` no longer proves the old aggregate load boundary as the first
  blocker; its first rejected GEP is over function parameter `%p.w` with
  pointee type `%struct.W = { %struct.V, [0 x %struct.S] }`, so treat it as a
  later pointer-parameter/flexible-array aggregate projection owner, not as
  local-memory admission under idea 297.
- `00046` no longer proves the old direct local-memory store or load subobject
  admission residual; after this packet it passes in the focused AArch64
  c-testsuite subset.
- `00204` is a separate bootstrap global aggregate/array semantics gate; do
  not fold it into this owner without evidence.
- Do not change expectations, allowlists, unsupported classifications, CTest
  registration, timeout policy, runner behavior, `test_after.log`, or
  `test_before.log`.
- Do not claim progress through filename matching or named-case shortcuts.
- Step 2 should not special-case `00143`; the rejected form is semantic:
  dynamic GEP into scalar local arrays whose elements are represented as local
  slots.
- If continuing on `00046`, keep the next packet on load-side scalar subobject
  admission only if a regression proves it returned; the focused proof now has
  `00046` passing.
- Keep `00218` out of the first implementation packet unless the supervisor
  confirms pointer-parameter aggregate projection is in scope for idea 297.
  It is not a direct local alloca boundary inside the failing function, and the
  current residual is no longer load local-memory.

## Proof

Ran exactly:

`cmake --build build --target c4cll backend_lir_to_bir_notes_test && ctest --test-dir build -R '^(backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_(00046|00140|00216|00218)_c)$' --output-on-failure | tee test_after.log`

Result: build passed and `backend_lir_to_bir_notes` passed. The focused
c-testsuite subset still fails overall as expected for residual boundary work.
Fresh residuals in `test_after.log`: `00046` passed, `00140` remains AArch64
call-boundary move printer, `00216` remains reported by the coarse GEP
local-memory diagnostic but is reclassified by first-GEP evidence as
pointer-parameter/flexible-array aggregate projection, and `00218` remains an
AArch64 scalar bitwise-immediate printer failure.
