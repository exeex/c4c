# LIR To BIR Local Memory Admission Todo

Status: Active
Source Idea Path: ideas/open/297_lir_to_bir_local_memory_admission.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Check Store/Load Boundaries

# Current Packet

## Just Finished

Step 3 implementation packet completed a bounded local aggregate store admission
repair for by-value aggregate parameter copies in variadic functions.
`collect_aggregate_params()` now keeps fixed aggregate parameters collected
before the variadic sentinel instead of discarding the whole parameter map when
it sees `...`. This admits `%p.f` as an aggregate source for the local copy
`store %struct.foo %p.f, ptr %lv.param.f` in `00140`.

Proof moved `00140` past the old `store local-memory semantic family` blocker;
it now reaches a later AArch64 call-boundary move printer failure. `00046` still
fails in `store local-memory semantic family`, `00216` still fails in
`gep local-memory semantic family`, and `00218` still fails in
`load local-memory semantic family`.

## Suggested Next

Plan-owner route decision: keep idea 297 active on Step 3. Do not close the
idea yet, because `00046` remains a direct local-memory store subobject
admission residual inside the source idea's store/load boundary criteria. Do
not switch or split for `00216`, `00218`, or the reclassified global/pointer
projection GEP residuals in this lifecycle pass; record them as later
owner-candidate evidence unless the supervisor explicitly selects that route.

Keep the next packet focused on `00046` if the supervisor wants to continue
Step 3 store/load boundaries: scalar stores through local aggregate/union byte
subobjects still need to route through addressable local subobject provenance
instead of falling into the exact local-slot type mismatch. Treat `00140`'s new
call-boundary move printer failure as a later non-local-memory backend packet.

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
- `00204` is a separate bootstrap global aggregate/array semantics gate; do
  not fold it into this owner without evidence.
- Do not change expectations, allowlists, unsupported classifications, CTest
  registration, timeout policy, runner behavior, `test_after.log`, or
  `test_before.log`.
- Do not claim progress through filename matching or named-case shortcuts.
- Step 2 should not special-case `00143`; the rejected form is semantic:
  dynamic GEP into scalar local arrays whose elements are represented as local
  slots.
- `00046` may need a separate follow-up after aggregate copy: scalar stores
  through local aggregate/union byte subobjects must route through
  `local_slot_pointer_values_`/`try_lower_pointer_provenance_store` as
  addressable local subobjects instead of falling into exact local-slot type
  mismatch.
- Keep `00218` out of the first implementation packet unless the supervisor
  confirms pointer-parameter aggregate projection is in scope for idea 297.
  It is not a direct local alloca boundary inside the failing function.

## Proof

Lifecycle decision proof note: close was not attempted, so no close guard was
run. The active focused boundary baseline available in this worktree is
`test_before.log`, where `backend_lir_to_bir_notes` passes and residuals are
`00046` store local-memory, `00140` AArch64 call-boundary move printer, `00216`
GEP local-memory, and `00218` load local-memory. `test_after.log` is not
present in the current worktree, and this lifecycle pass did not recreate or
modify proof logs.

Ran exactly:

`cmake --build build --target c4cll backend_lir_to_bir_notes_test && ctest --test-dir build -R '^(backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_(00046|00140|00216|00218)_c)$' --output-on-failure | tee test_after.log`

Prior executor-recorded result: build passed and `backend_lir_to_bir_notes`
passed. The focused c-testsuite subset still failed overall. Residuals:
`00046` store local-memory, `00140` now AArch64 call-boundary move printer
failure, `00216` GEP local-memory, `00218` load local-memory.
