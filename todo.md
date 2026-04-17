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

## Suggested Next

- Start from the current `realize_template_struct(...)` flow and extract the
  smallest reusable helper that exposes selected pattern, resolved bindings,
  and concrete instance tag.
- Integrate that helper into
  `StmtEmitter::resolve_member_field_access(...)` before direct
  `struct_defs[tag]` failure.
- Keep a focused repro around `eastl::pair<int, int>` member access as the
  acceptance surface before deciding whether to re-enable the external utility
  case in the same slice.

## Watchouts

- Do not solve this with `pair`-specific aliases or emitted-name string
  rewrites.
- Do not silently expand the patch into a repo-wide ABI-mangling redesign.
- Keep the active EASTL bootstrap idea open; this switch is a route split, not
  a closure.

## Proof

- `cmake -S . -B build`
- `cmake --build build --target c4cll -j8`
- `build/c4cll -I ref/EASTL/include -I ref/EABase/include/Common /tmp/eastl_pair_member.cpp -o /tmp/eastl_pair_member.ll`
