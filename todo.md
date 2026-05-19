# LIR To BIR Local Memory Admission Todo

Status: Active
Source Idea Path: ideas/open/297_lir_to_bir_local_memory_admission.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Check Store/Load Boundaries

# Current Packet

## Just Finished

Step 3 implementation packet completed the direct local-memory scalar subobject
load admission repair selected for `00046`. Scalar loads through local
aggregate byte-array subobjects now mirror the accepted store path: when the
pointer is a tracked `i8` local pointer-array view and the scalar access fits
within the byte slots, lowering emits an addressed local load instead of
failing on the exact local-slot type mismatch.

Proof moved `00046` past the old `load local-memory semantic family` blocker
and it now passes in the focused AArch64 c-testsuite subset. `00140` remains at
the AArch64 call-boundary move printer failure and `00216` remains at the GEP
local-memory residual in `foo`. `00218` also moved past its load local-memory
residual and now reaches a later AArch64 scalar bitwise-immediate printer
failure.

## Suggested Next

Lifecycle decision: keep idea 297 active on Step 3. Do not close yet and do not
advance to Step 4 proof until `00216` is repaired or classified outside the
local-memory admission owner. The direct local aggregate/union byte-subobject
load/store lane represented by `00046` is now repaired and passing, and `00140`
plus `00218` are classified as later AArch64 printer residuals outside
local-memory admission.

Next bounded packet: diagnose and repair or reclassify the remaining in-owner
Step 3 residual, `00216`, which still fails in `foo` at the GEP local-memory
semantic family. Keep the packet focused on semantic local-memory admission;
split only if fresh diagnosis proves `00216` belongs to a distinct owner rather
than the active local-memory store/load boundary work.

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
  blocker; after this packet it reaches a later GEP-family local-memory
  blocker in `foo`.
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
call-boundary move printer, `00216` remains GEP local-memory, and `00218` moved
to an AArch64 scalar bitwise-immediate printer failure.
