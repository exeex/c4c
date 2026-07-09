Status: Active
Source Idea Path: ideas/open/633_aggregate_stack_home_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Trace Stack-Home Fallthrough After Authority Gate

# Current Packet

## Just Finished

Step 6 traced the aggregate stack-home fallthrough after the Step 4 authority
gate. Evidence is recorded in
`build/agent_state/633_step6_fallthrough_trace.md`, with source extracts from
the Step 5 prepared dumps and the current RV64 local-memory consumer helpers.

Primary `src/pr38969.c` byval load:
- Function/block/instruction: `foo`, `entry`, instruction `0`.
- BIR: `%lv.param.p.x.aggregate.param.copy.0 = bir.load_local float %lv.param.p.x.0, addr %p.x`.
- Prepared access: `base=pointer_value`, `result=%lv.param.p.x.aggregate.param.copy.0`,
  `pointer=%p.x`, `offset=0`, `size=4`, `align=4`,
  `base_plus_offset=yes`, `range_verdict=proven_in_bounds`.
- Pointer value home: `%p.x` is a stack-slot home at slot `#1`, offset `8`.
- Stack object: `%p.x` has `source_kind=byval_param`, size `8`, align `4`,
  address-exposed and permanent home-slot facts.

`prepared_stack_home_local_memory_has_authority(..., ByvalParam)` would accept
that selected access with the current facts: the helper requires the pointer
value stack-slot home, byval object role, complete/proven range, result-only
access, default nonvolatile pointer-value address, and in-object byte range.
It does not require `layout_authority=scalar_layout`.

Primary `src/pr38969.c` sret store:
- Function/block/instruction: `foo`, `entry`, instruction `5`.
- BIR: `bir.store_local %lv.param.p.x.0, float foo.ret.sret.copy.0, addr %ret.sret`.
- Prepared access: `base=pointer_value`, `stored=foo.ret.sret.copy.0`,
  `pointer=%ret.sret`, `offset=0`, `size=4`, `align=4`,
  `base_plus_offset=yes`, `range_verdict=proven_in_bounds`.
- Pointer value home: `%ret.sret` is a stack-slot home at slot `#0`, offset `0`.
- Stack object: `%ret.sret` has `source_kind=sret_param`, pointer type,
  address-exposed and permanent home-slot facts.

`prepared_stack_home_local_memory_has_authority(..., SretParam)` would accept
the selected store. The diagnostic path has an sret store precheck, but the
actual floating store emission branch does not route through
`prepared_sret_stack_slot_pointer_access(...)`.

Exact failing boundary:
- `object_emission.cpp` uses a special `f64_memory` branch in
  `local_memory_diagnostic` for floating loads/stores. That branch checks only
  frame-slot, direct pointer-register, and generic pointer stack-home
  addressing. It never calls the byval/sret stack-home helpers.
- `fragment_for_prepared_load_local(...)` and
  `fragment_for_prepared_store_local(...)` take their floating branches before
  the scalar byval/sret helper routes. Those floating branches fall back to
  `materialize_prepared_pointer_value_base_offset(...)`, whose generic
  stack-home path excludes objects with `source_kind=byval_param` or
  `source_kind=sret_param`.

Cross-checks:
- `src/pr30185.c` has byval pointer-value loads from `%p.x` and `%p.y` and
  sret stores to `%ret.sret`, with `base_plus_offset=yes` and proven ranges.
  Its byte-sliced lanes use sizes such as `1` with object alignment `8`; the
  current byval/sret helper-local prechecks reject `align_bytes > size_bytes`
  before authority evaluation.
- `src/950628-1.c` is sret-only and shows the same shape for stores to
  `%ret.sret`, including size-1 lanes with alignment `2`. Size-2 lanes match
  the sret helper shape; size-1 lanes hit the same helper-local alignment gate.

Classification: RV64 consumer-routing issue. The prepared-address shape is
usable, and the authority predicate is not stale. The remaining miss is in the
RV64 consumer helpers: floating byval/sret routes bypass the authority helpers,
and integer byte-sliced lanes reject too early on stronger-than-width alignment.

## Suggested Next

Step 7 should repair only the RV64 stack-home consumer route:
- Add F32/F64 byval-load and sret-store routing in
  `fragment_for_prepared_load_local(...)`,
  `fragment_for_prepared_store_local(...)`, and the matching diagnostic
  predicates, gated by the existing
  `prepared_stack_home_local_memory_has_authority(...)` roles.
- Relax the byval/sret helper-local `align_bytes > size_bytes` prechecks only
  after authority proves the stack-home object, selected range, size, and RV64
  width are valid.
- Add focused RV64 object-emission coverage for the `pr38969`-shape F32 byval
  load and sret store plus malformed-authority negatives, with an integer
  byte-slice cross-check matching `pr30185` / `950628-1`.

## Watchouts

Idea 633 is not close-ready. The clean byval/sret rows still own a narrow RV64
consumer-routing repair.

Do not repair this by treating all pointer-value stack homes as generic pointer
homes. The existing generic helper intentionally excludes `byval_param` and
`sret_param`; Step 7 should keep using explicit stack-home authority.

Keep F128/16-byte local-memory rows, move-bundle rows, mixed local/global rows,
runtime mismatches, and unrelated aggregate copies out of the Step 7 packet.

## Proof

Trace-only packet. No build proof was required and no root-level `.log` file
was created.

Validation command:
`git diff --check`

Result: passed.
