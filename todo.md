# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Migrate Canonical Call And Return Families
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Started step 2.2 by materializing the reviewed lowering file boundaries for the
canonical call and return families. Added `lowering/call_lowering.*` and
`lowering/return_lowering.*`, moved the existing call/return implementation
methods behind those seams, wired the new lowering translation units into the
backend build, and left `calls.cpp` / `returns.cpp` as explicit compatibility
placeholders so the ownership move is visible instead of silently mixed.

This packet also fixed the transitional x86 shared surface so
`c4c::backend::PhysReg` is concretely defined in `x86_codegen.hpp`; the new
compiled lowering seams need that type definition rather than only the old
forward declaration.

## Suggested Next

Keep step 2.2 focused on canonical ownership, not behavior rewrites: move one
coherent family next, likely call setup/argument placement first or return
publication/epilogue second, and keep prepared/module callers as thin
consumers of the new lowering seam.

## Watchouts

- Keep `calls.cpp` and `returns.cpp` non-owning; future packets should migrate
  callers into the new lowering seams rather than repopulating those top-level
  files.
- Avoid widening this seam-establishment work into ABI policy changes,
  prepared-route admission logic, or module dispatch rewiring.
- The `PhysReg` definition is now shared in `x86_codegen.hpp`; if later packets
  start using more of the old top-level codegen TUs, make sure they converge on
  that single concrete type instead of reintroducing shadow declarations.
- Reject testcase-shaped matcher growth while migrating call/return helpers:
  step 2.2 owns seam control and canonical lowering, not named-case shortcuts.

## Proof

Step 2.2 call/return seam materialization on 2026-04-22:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
