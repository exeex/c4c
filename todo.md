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
- Fixed the method-scope injected-class-name hole so instantiated struct
  methods now resolve bare owner-family spellings like `pair` back to the
  current instantiated struct during signature lowering.
- Proved `pair::swap(pair& p)` now lowers in HIR with a concrete parameter type
  and owner-tagged `&p.first` / `&p.second` member accesses.
- Removed the codegen-side `resolve_template_struct_concrete_tag(...)`
  fallback after confirming the focused pair repros and reduced EASTL utility
  smoke still compile through simple LIR `struct_defs[tag]` lookup.

## Suggested Next

- Add a targeted HIR/LIR regression test that asserts the `GetPair`-style
  member path and `pair::swap(pair& p)` both keep their HIR-bound owner layout
  instead of regressing back to placeholder/injected-family lookup.
- Decide whether any remaining template-family recovery helpers in HIR still
  cover distinct pre-materialization cases now that the LIR-side fallback is
  gone.
- Check whether this slice is broad enough to checkpoint now that the utility
  proofs still pass without codegen-side template recovery.

## Watchouts

- Do not solve this with `pair`-specific aliases or emitted-name string
  rewrites.
- Do not silently expand the patch into a repo-wide ABI-mangling redesign.
- Keep the active EASTL bootstrap idea open; this switch is a route split, not
  a closure.

## Proof

- `cmake -S . -B build`
- `cmake --build build --target c4cll -j8`
- `build/c4cll -I ref/EASTL/include -I ref/EABase/include/Common /tmp/eastl_pair_swap.cpp -o /tmp/eastl_pair_swap.ll`
- `build/c4cll -I ref/EASTL/include -I ref/EABase/include/Common /tmp/eastl_pair_member.cpp -o /tmp/eastl_pair_member.ll`
- `build/c4cll -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/external/eastl/utility/frontend_basic.cpp -o /tmp/eastl_utility_frontend_basic.ll`
- `build/c4cll --dump-hir -I ref/EASTL/include -I ref/EABase/include/Common /tmp/eastl_pair_swap.cpp`
- `build/c4cll --dump-hir -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/external/eastl/utility/frontend_basic.cpp`
