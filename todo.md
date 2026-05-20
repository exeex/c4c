Status: Active
Source Idea Path: ideas/open/347_aarch64_local_conversion_store_load_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair the General Local Conversion Publication Rule

# Current Packet

## Just Finished

Step 2 repaired a follow-up dispatch hazard in the local conversion result
publication owner in `src/backend/mir/aarch64/codegen/dispatch.cpp`. The first
Step 2 implementation made the new same-block scalar-cast path replace the
older GP local-store publication path whenever a cast producer existed. That
was too broad: unsupported cast forms, especially non-FP integer/pointer casts
that the new helper intentionally does not lower, could prevent the established
local-store publication rule from running.

The local store publication rule now treats the scalar/FP conversion helper as
a specialized first attempt. If it cannot emit and the selected store source is
a GPR, dispatch clears any partial lines and falls back to the existing
`emit_value_publication_to_register` path. This keeps the `00175` scalar/FP
store conversion improvement while preserving the pre-existing local-store
publication route for non-conversion cases.

## Suggested Next

Supervisor should decide whether to accept this monotonic Step 2 guard slice
or route-review the broader rejected-baseline story. The delegated proof now
matches `test_before.log`: `00005`, `00020`, `00103`, `00130`, and `00168`
still fail in the same subset, while `00175` remains passing.

## Watchouts

- The fix is intentionally in the AArch64 dispatch-side publication rule
  because the selected store already names the authoritative destination
  register; the missing piece was materializing the conversion into that
  register before the store.
- A temporary parent-commit comparison (`d22f1b86f`) showed the generated
  assembly for `00020`, `00130`, and `00168` is unchanged from before
  `04c16334c`; the assigned five-case subset is therefore not repaired to
  passing by this dispatch-only slice. `00168` still uses an uninitialized
  callee-saved value after the recursive call, which looks outside the local
  scalar/FP conversion publication path.
- Step 3 coverage should avoid pinning current scratch choices (`w13`, `s13`,
  `d16`) or stack offsets; those remain allocation details.

## Proof

Ran the delegated proof command exactly:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_(00005|00020|00103|00130|00168|00175)_c$' | tee test_after.log`.
The build recompiled `dispatch.cpp`. The subset result matches the delegated
`test_before.log` failure set: `00005`, `00020`, `00103`, `00130`, and `00168`
fail; `00175` passes. Proof log: `test_after.log`.
