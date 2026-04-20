# Execution State

Status: Active
Source Idea Path: ideas/open/62_stack_addressing_and_dynamic_local_access_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Repair The Selected Stack/Addressing Seam
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Step 1 narrowed the first idea-62 seam to mixed aggregate-parameter
stack/member access. The semantic-BIR route still fails on
`c_testsuite_x86_backend_src_00204_c`, but reduced repros showed the break is
not generic `x86_fp80` byval handling: a standalone `struct hfa32` byval
parameter lowers, while any actual field access on a direct `struct hfa14`
parameter fails in `store local-memory semantic family` once a second
aggregate parameter is present. Empty-body variants for the same mixed
signatures lower cleanly, so the active repair target is mixed aggregate-param
member access/materialization rather than generic variadic or prepared-module
work.

## Suggested Next

Repair the mixed aggregate-parameter seam by making field access on a direct
`struct hfa14` parameter coexist with a second aggregate parameter without
falling back out of semantic BIR. Add the nearest backend sentinel for that
reduced shape, then prove it together with
`backend_codegen_route_x86_64_byval_member_array_params_observe_semantic_bir`
and `c_testsuite_x86_backend_src_00204_c`.

## Watchouts

- Do not treat this as generic `x86_fp80` or variadic cleanup. The reduced
  `struct hfa32` byval-only case already lowers to semantic BIR.
- Keep the repair on mixed aggregate-parameter member access/materialization,
  not on emitter-local call shaping.
- Do not reopen idea-58 bootstrap-global ownership for
  `c_testsuite_x86_backend_src_00204_c` unless it falls back into that
  diagnostic family.
- Rehome cases that reach prepared CFG, prepared-module, scalar-emitter, or
  runtime-correctness ownership instead of leaving them mixed into idea 62.

## Proof

Current family subset:
`ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_x86_64_local_direct_dynamic_member_array_(store|load)_observe_semantic_bir|c_testsuite_x86_backend_src_00204_c)$'`

Reduced-route audit:
- a standalone `struct hfa32` byval parameter with member loads lowers under
  `./build/c4cll --backend-bir-stage semantic --codegen asm --target x86_64-unknown-linux-gnu <tmp> -o <tmp>`
- mixed-signature reductions such as `float f(struct hfa14 a, struct hfa23 b)
  { return a.a; }` and `void f(struct hfa14 a, struct hfa32 c) { ... a.a ...
  }` fail in `store local-memory semantic family`
- empty-body variants for the same mixed signatures lower cleanly, isolating
  the seam to actual field access on the direct aggregate parameter
