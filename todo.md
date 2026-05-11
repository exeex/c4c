Status: Active
Source Idea Path: ideas/open/168_compatibility_bridge_retirement.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Retire BIR and Backend Raw Symbol Fallbacks

# Current Packet

## Just Finished

Step 6 - Retire BIR and Backend Raw Symbol Fallbacks fenced BIR/LIR-to-BIR
pointer initializer raw symbol parsing as a retained legacy compatibility
boundary behind initializer `LinkNameId` metadata.

This packet made the raw `parse_global_address_initializer` boundary explicit:
it exists for raw/no-id initializer payload compatibility and offset parsing,
while present initializer `LinkNameId` metadata remains semantic authority and
must fail closed on misses instead of falling back to raw spelling. Existing
pointer-initializer tests continue to prove function/global `LinkNameId`
authority, raw-only compatibility when no id is present, and present-id miss
closure for scalar and aggregate pointer initializer paths.

## Suggested Next

Continue Step 6 with another narrow BIR/backend raw-symbol family where
structured id metadata already exists, such as raw function/global symbol maps
or imported BIR name tables. Preserve runtime/intrinsic invalid-id boundaries
and raw-only compatibility cases unless a present-id fallback gap is found.

## Watchouts

- This packet was behavior-preserving by design; it fenced an already-proven
  initializer path rather than changing lowering semantics.
- Raw-only pointer initializer compatibility remains valid when no initializer
  `LinkNameId` metadata is present.
- Present initializer `LinkNameId` metadata must remain authoritative and fail
  closed on miss for both scalar and aggregate pointer initializer paths.
- Keep route-local value, block, slot, prepared, and generated-name cleanup out
  of Step 6 unless a raw spelling is acting as source/link-visible bridge
  authority; those local identity families belong to idea 169.
- The pre-existing untracked `review/168_step4_hir_bridge_route_review.md`
  remains transient and was not modified.
- No current blockers.

## Proof

Step 6 focused proof:
`cmake --build build --target backend_lir_to_bir_notes_test && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_notes$'`

Result: passed in `test_after.log`.

Additional check: `git diff --check` passed.
