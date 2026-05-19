# LIR To BIR Local Memory Admission Todo

Status: Active
Source Idea Path: ideas/open/297_lir_to_bir_local_memory_admission.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Check Store/Load Boundaries

# Current Packet

## Just Finished

Step 3 implementation packet completed a bounded local aggregate load/copy
admission repair. `record_loaded_local_pointer_slot_state` now publishes a
`LocalAggregateSlots` view when a loaded local pointer is known to point at a
local aggregate slot or aggregate subobject. This admits the direct local
representative in `00216`, where `%t166 = load ptr, ptr %lv.pls` points back to
`%lv.ls` and `%t167 = load %struct.S, ptr %t166` can be treated as an aggregate
value alias over the same leaf slots.

Proof moved `00216` past the old `load local-memory semantic family` blocker;
it now reaches a later `gep local-memory semantic family` failure in `foo`.
`00046` and `00140` still fail in `store local-memory semantic family`, and
`00218` still fails in `load local-memory semantic family`.

## Suggested Next

Keep the next packet focused on the remaining direct local store owner:
`00140` still rejects the by-value aggregate parameter copy into
`%lv.param.f`, while `00046` remains the scalar subobject store follow-up.
Alternatively, split `00216`'s new later GEP failure only if the supervisor
wants to continue that case after the aggregate load boundary is unblocked.

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

Ran exactly:

`cmake --build build --target c4cll backend_lir_to_bir_notes_test && ctest --test-dir build -R '^(backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_(00046|00140|00216|00218)_c)$' --output-on-failure | tee test_after.log`

Result: build passed and `backend_lir_to_bir_notes` passed. The focused
c-testsuite subset still fails overall, with `test_after.log` preserving the
proof. Residuals: `00046` store local-memory, `00140` store local-memory,
`00216` now GEP local-memory, `00218` load local-memory.
