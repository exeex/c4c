Status: Active
Source Idea Path: ideas/open/639_pointer_loaded_from_global_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Locate the missing authority boundary

# Current Packet

## Just Finished

Step 2 located the missing authority boundary for `src/pr46309.c` without
implementation edits. The clean row remains `%t15 = bir.load_global ptr @q`
followed by `%t16 = bir.load_local i32 %t16.addr, addr %t15`.

Boundary artifact:
`build/agent_state/639_step2_missing_authority_boundary/pr46309_boundary.md`.

Findings:

- Prepared addressing already has the selected local-memory use:
  `block_1 inst=1`, pointer `%t15`, offset `0`, width `4`, align `4`,
  default address space by absence of non-default markers.
- Prepared addressing already has the global load access:
  `block_1 inst=0`, symbol `q`, offset `0`, width `8`, align `8`, scalar
  layout, proven in bounds.
- `%t15` has only `home %t15 value_id=7 kind=register reg=s1`; it is not a
  `pointer_base_plus_offset` home and has no complete loaded-pointer source
  authority.
- Existing pointer-value memory freshness can validate a selected pointer
  memory use, but it cannot publish the pointer's global source identity,
  object extent, or pointer-value home. RV64 therefore correctly remains
  fail-closed at `unsupported_local_memory_access`.
- The first repair belongs in prepared/prealloc producer fact publication for
  pointer-loaded-from-global local-memory authority, with RV64 as a strict
  consumer of that complete fact. It should not be reclassified into direct
  global-symbol local memory, prepared global value-location, aggregate
  stack-home, or ordinary frame-slot policy.

## Suggested Next

Implement Step 3 as a narrow prepared/prealloc producer packet: publish and
print one complete `pointer_loaded_from_global` local-memory authority for the
same-block `@q -> %t15 -> load_local addr %t15` shape, then have the RV64
pointer-value base-plus-offset consumer accept only that complete authority.
The implementation proof should include one accepted `pr46309`-style shape and
one missing-authority rejection.

## Watchouts

- Keep direct `addr @symbol` local-memory rows under idea 631 and prepared
  global value-location rows under idea 621.
- Keep aggregate/byval/sret/stack-home rows under idea 633 and related ABI
  policy; `pr58984` and `pr66556` should not drive this packet.
- Missing producer, missing freshness, non-default address space, volatile
  access, incomplete global source identity, incomplete extent/width,
  ambiguous multiple producers, and stale cross-call/global publication must
  remain fail-closed.
- Do not infer loaded-pointer authority from source filenames, final symbol
  names, final assembly layout, register assignment, or BIR adjacency alone.

## Proof

No refresh proof was run for Step 2. This was a boundary-tracing packet only,
using Step 1 artifacts under
`build/agent_state/639_step1_pointer_loaded_from_global/` plus source
inspection. The canonical Step 1 proof log remains `test_after.log`; Step 2
artifact path is
`build/agent_state/639_step2_missing_authority_boundary/pr46309_boundary.md`.
