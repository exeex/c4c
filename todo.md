Status: Active
Source Idea: ideas/open/16_template_struct_instance_resolution_for_symbol_lookup.md
Source Plan: plan.md

# Current Packet

Extract a shared template-struct instance resolver from the current
partial-specialization path, then wire late member lookup into it for the
exposed `eastl::pair<int, int>` failure route.

## Just Finished

- Switched the active lifecycle state away from the EASTL bootstrap idea after
  confirming the new work is a separate compiler initiative, not routine suite
  expansion.
- Recorded a new open idea for shared template-instance resolution in symbol
  and member lookup.
- Rewrote `plan.md` around the shared-helper route instead of continuing the
  old external-suite expansion runbook.
- Added a shared late-lookup fallback surface in HIR/codegen so member access
  can reuse template-instance aliases and family-level layout lookup instead of
  only raw `struct_defs[tag]`.
- Proved the focused `/tmp/eastl_pair_member.cpp` repro now compiles through
  the repaired member path.
- Bound resolved member-owner layout tags into HIR `MemberExpr` nodes so the
  `GetPair`/pair helper path now tells LIR which realized struct layout to use
  instead of asking LIR to infer it from placeholder template tags.
- Re-ran the reduced EASTL utility smoke successfully after the HIR-side owner
  binding change.
- Extended the HIR-side owner binding to more construction paths, including
  defaulted member copies and initializer-list helper field stores, and added
  AST-type fallbacks so templated assignment-style member accesses keep their
  owner layout in dumps.

## Suggested Next

- Chase the remaining `pair::swap`-style `&p.first` / `&p.second` member path
  that still prints without `resolved_owner_tag`; until that route is fixed,
  the codegen-side `resolve_template_struct_concrete_tag(...)` fallback still
  has real coverage value.
- After that last HIR gap is closed, tighten the contract by reducing or
  removing the codegen-side `resolve_template_struct_concrete_tag(...)`
  fallback.
- Add a targeted HIR/LIR regression test that asserts the `GetPair`-style
  member path keeps its HIR-bound owner layout instead of regressing back to
  placeholder tag lookup.
- Decide whether this slice is broad enough to checkpoint now or whether the
  next packet should first finish removing the remaining codegen-side template
  recovery path.

## Watchouts

- Do not solve this with `pair`-specific aliases or emitted-name string
  rewrites.
- Do not “fix” the remaining `pair::swap` route by teaching LIR more template
  inference; keep that investigation on the HIR side.
- Do not silently expand the patch into a repo-wide ABI-mangling redesign.
- Keep the active EASTL bootstrap idea open; this switch is a route split, not
  a closure.

## Proof

- `cmake -S . -B build`
- `cmake --build build --target c4cll -j8`
- `build/c4cll -I ref/EASTL/include -I ref/EABase/include/Common /tmp/eastl_pair_member.cpp -o /tmp/eastl_pair_member.ll`
- `build/c4cll -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/external/eastl/utility/frontend_basic.cpp -o /tmp/eastl_utility_frontend_basic.ll`
- `build/c4cll --dump-hir -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/external/eastl/utility/frontend_basic.cpp`
