# LIR To BIR Local Memory Admission Todo

Status: Active
Source Idea Path: ideas/open/297_lir_to_bir_local_memory_admission.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Check Store/Load Boundaries

# Current Packet

## Just Finished

Step 3 implementation packet completed the direct local-memory scalar subobject
store admission repair selected for `00046`. Scalar stores through local
aggregate byte-array subobjects can now lower as addressable local subobject
stores instead of falling into the exact local-slot type mismatch when the
pointer base is an `i8` local pointer-array view and the scalar access fits
within the byte slots. Provenance scalar-subobject checks also now admit
byte-array storage when the scalar access is bounded by the byte storage size.

Proof moved `00046` past the old `store local-memory semantic family` blocker;
it now reaches a later `load local-memory semantic family` residual in `main`.
`00140` remains at the AArch64 call-boundary move printer failure, `00216`
remains at the GEP local-memory residual in `foo`, and `00218` remains at its
load local-memory residual.

## Suggested Next

Keep the next packet focused on `00046` only if the supervisor wants to finish
the same source file's local-memory boundary: its next blocker is now a
load-side scalar subobject residual over the same aggregate/union byte
subobject shape. Treat `00140`'s call-boundary move printer failure and
`00216`'s GEP residual as separate later packets.

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
- `00046` no longer proves the old direct local-memory store subobject
  admission residual; after this packet it reaches a later load local-memory
  residual in `main`.
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
  admission; do not re-open the now-moved store residual unless a regression
  proves it returned.
- Keep `00218` out of the first implementation packet unless the supervisor
  confirms pointer-parameter aggregate projection is in scope for idea 297.
  It is not a direct local alloca boundary inside the failing function.

## Proof

Ran exactly:

`cmake --build build --target c4cll backend_lir_to_bir_notes_test && ctest --test-dir build -R '^(backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_(00046|00140|00216|00218)_c)$' --output-on-failure | tee test_after.log`

Result: build passed and `backend_lir_to_bir_notes` passed. The focused
c-testsuite subset still fails overall as expected for residual boundary work.
Fresh residuals in `test_after.log`: `00046` moved to load local-memory,
`00140` remains AArch64 call-boundary move printer, `00216` remains GEP
local-memory, and `00218` remains load local-memory.
