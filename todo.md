Status: Active
Source Idea Path: ideas/open/263_aarch64_codegen_public_compiled_module_interface.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory the current compiled-module public surface

# Current Packet

## Just Finished

Lifecycle switched from the paused 259 cast-ops redistribution idea to
`ideas/open/263_aarch64_codegen_public_compiled_module_interface.md`.

## Suggested Next

Execute Step 1 of `plan.md`: inventory the current compiled-module public
surface before editing implementation code.

Expected first packet:
- find the definitions and callers of `aarch64::module::Module` and
  `aarch64::module::BuildResult`;
- separate public handoff usage from internal implementation details that can
  remain in place;
- identify the minimal Step 2 edit set for adding `codegen.hpp` and public
  codegen-owned names;
- choose the focused build/test proof command for the first code slice.

## Watchouts

- Keep this behavior-preserving; no lowering, emitted assembly, or diagnostics
  changes.
- Do not broaden into a full namespace purge of lower-level module,
  diagnostic, or lowering internals.
- Compatibility wrappers are allowed only as staged migration support; the
  public route should advertise `aarch64::codegen` names.
- Include public asm smoke in proof once code changes begin.

## Proof

Lifecycle-only switch. No build or test proof was required for this packet.
