# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Migrate Canonical Call And Return Families
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Completed step 2.2’s call-lowering seam repair packet by restoring the moved
canonical call owner links inside `lowering/call_lowering.cpp`. The compiled
call-lowering TU now carries the shared-support and x86 utility definitions it
actually consumes after the seam move, so the backend target links again
without widening ownership into `calls.cpp`, `returns.cpp`, or return-lowering.

## Suggested Next

Keep step 2.2 on canonical ownership cleanup: decide whether the temporary
support bridge in `lowering/call_lowering.cpp` should stay local for the rest
of the rebuild or whether a later packet should re-home those shared helpers by
bringing the intended support TU into the compiled backend target.

## Watchouts

- `lowering/call_lowering.cpp` now contains a temporary bridge for shared call
  support and x86 utility owners because the active backend target still does
  not compile `shared_call_support.cpp` or `mod.cpp`.
- Keep `calls.cpp` and `returns.cpp` non-owning; future packets should keep
  migrating behavior toward the lowered seams instead of repopulating the old
  top-level files.
- Do not treat this link repair as permission to widen into ABI policy changes,
  prepared-route admission logic, or return-lowering work.

## Proof

Step 2.2 call-lowering seam repair on 2026-04-22:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
