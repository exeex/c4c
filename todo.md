# LIR To BIR Local Memory Admission Todo

Status: Active
Source Idea Path: ideas/open/297_lir_to_bir_local_memory_admission.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Check Store/Load Boundaries

# Current Packet

## Just Finished

Step 3 diagnostic-only packet completed for store boundary cases `00046`,
`00140` and load boundary cases `00216`, `00218`. All four still abort before
semantic BIR handoff in the current `test_before.log`; `--dump-bir` reproduces
the same family notes and emits no BIR:

- `00046`: `main` fails in `store local-memory semantic family`. LLVM shape is
  local aggregate alloca `%lv.v` followed by nested aggregate/union GEP chains
  and scalar stores, e.g. `getelementptr %struct._anon_1, ptr %t1, i32 0, i32 0`
  then `store i32 2, ptr %t2`, plus a deeper anonymous struct/union chain to
  `store i32 3, ptr %t6`. This is still local aggregate subobject storage, but
  the representative rejected store is a scalar write through an aggregate/byte
  subobject pointer rather than a direct scalar local slot.
- `00140`: `f1` fails in `store local-memory semantic family`. LLVM shape is
  `define i32 @f1(%struct.foo %p.f, ptr %p.p, i32 %p.n, ...)` followed by
  `%lv.param.f = alloca %struct.foo` and `store %struct.foo %p.f, ptr
  %lv.param.f`. The existing aggregate store lane only copies from
  `aggregate_params_` or `aggregate_value_aliases_`; this by-value aggregate
  parameter does not appear to be admitted as a source aggregate for the local
  aggregate store.
- `00216`: `foo` fails in `load local-memory semantic family`. LLVM has several
  aggregate load/copy forms. The local-owner representative is `const struct S
  *pls = &ls; struct S ls21 = *pls;`, emitted as `store ptr %lv.ls, ptr
  %lv.pls`, `%t166 = load ptr, ptr %lv.pls`, `%t167 = load %struct.S, ptr
  %t166`, then `store %struct.S %t167, ptr %lv.ls21`. Later forms also load
  aggregate values from pointer-derived aggregate members, e.g. `%t431 = load
  %struct.T, ptr %t430`. The current aggregate load branch only aliases direct
  `load.ptr` names already present in `local_aggregate_slots_`; it does not
  copy an aggregate through `local_slot_pointer_values_`/pointer-derived local
  aggregate provenance.
- `00218`: `convert_like_real` fails in `load local-memory semantic family`.
  LLVM shape is parameter-pointer aggregate projection:
  `%t0 = getelementptr %struct.tree_node, ptr %p.convs, i32 0, i32 0`,
  `%t1 = getelementptr %struct.tree_common, ptr %t0, i32 0, i32 2`, then
  `%t2.bf.unit = load i32, ptr %t1`. The caller passes a local address, but the
  failing callee sees only a pointer parameter and aggregate field GEPs over
  that pointer. Treat this as pointer-parameter aggregate projection unless a
  lifecycle review explicitly broadens idea 297 beyond direct local-memory
  admission.

## Suggested Next

Implement one bounded local-memory aggregate boundary packet, preferably
starting with the direct owner shared by `00140` and the local representative in
`00216`: aggregate value copy between local aggregate slots and aggregate
sources tracked through by-value aggregate parameters or local pointer
provenance. Likely owner helpers are `lower_memory_store_inst` and
`lower_memory_load_inst` in `src/backend/bir/lir_to_bir/memory/local_slots.cpp`,
with copy emission through `append_local_aggregate_copy_from_slots` or a sibling
helper that can copy from a `LocalSlotAddress`/pointer-derived local aggregate.

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

Diagnostic-only Step 3 packet; per delegation, did not implement fixes, did not
rerun broad backend regex, and did not modify `test_before.log` or
`test_after.log`.

Diagnostics run:

- Read current `test_before.log` focused baseline for `00046`, `00140`,
  `00216`, and `00218`.
- Ran `./build/c4cll --codegen llvm --target aarch64-linux-gnu` to `/tmp` for
  `00046`, `00140`, `00216`, and `00218` and classified the representative
  LLVM store/load forms above.
- Ran `./build/c4cll --dump-bir --target aarch64-linux-gnu` on `00046`,
  `00140`, `00216`, and `00218`; each stopped before BIR output with the same
  family diagnostics recorded in `test_before.log`.
