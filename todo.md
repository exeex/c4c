# Execution State

Status: Active
Source Idea Path: ideas/open/62_stack_addressing_and_dynamic_local_access_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Repair The Selected Stack/Addressing Seam
Plan Review Counter: 2 / 10
# Current Packet

## Just Finished

Step 2.2 repaired the mixed aggregate-parameter stack/member seam by
normalizing `ptr byval(...)` aggregate-param metadata and by materializing
direct aggregate loads from globals into local aggregate slots. Reduced
semantic-BIR repros for both `fa3`-style and `fa4`-style mixed signatures now
lower past the old idea-62 `store/load local-memory` failures, and
`c_testsuite_x86_backend_src_00204_c` no longer stops in `fa3`. The case now
fails later in `arg` under the downstream `direct-call semantic family`, so it
has graduated out of idea 62's stack/addressing ownership.

## Suggested Next

Treat `c_testsuite_x86_backend_src_00204_c` as rehomed out of idea 62 and
route the next packet to the downstream semantic-call owner for the new
`arg`/`direct-call semantic family` failure. Keep idea 62 focused on any
remaining stack/addressing cases that still stop before semantic-call
lowering.

## Watchouts

- Do not keep debugging `00204.c` under idea 62 now that it fails in the
  downstream `direct-call semantic family`; that would blur stack/addressing
  progress with semantic-call ownership.
- The repaired seam covered both mixed aggregate-param metadata collection and
  global aggregate-to-local materialization. Regressions here are most likely
  to show up as `store/load local-memory semantic family` failures on direct
  aggregate field access or direct aggregate call preparation.
- The existing route sentinels still pass, so avoid weakening them or
  backsliding into testcase-shaped call/emitter shortcuts.

## Proof

Current family subset:
`ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_x86_64_local_direct_dynamic_member_array_(store|load)_observe_semantic_bir|c_testsuite_x86_backend_src_00204_c)$'`

Proof run:
- `cmake --build --preset default`
- `ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_x86_64_local_direct_dynamic_member_array_(store|load)_observe_semantic_bir|c_testsuite_x86_backend_src_00204_c)$'`
- `test_after.log`

Reduced-route audit:
- `./build/c4cll --backend-bir-stage semantic --codegen asm --target x86_64-unknown-linux-gnu /tmp/repro_fa3_a.c -o /tmp/repro_fa3_a.out` now lowers `fa3`-style direct-field access with extra aggregate params through semantic BIR instead of failing in store local-memory lowering
- `./build/c4cll --backend-bir-stage semantic --codegen asm --target x86_64-unknown-linux-gnu /tmp/repro_arg_fa3.c -o /tmp/repro_arg_fa3.out` now moves past local-memory lowering and fails later in the downstream `direct-call semantic family`
