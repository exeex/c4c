Status: Active
Source Idea Path: ideas/open/330_native_object_model_and_emission_api.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Current Object And Backend Surfaces

# Current Packet

## Just Finished

Lifecycle switch completed: the umbrella runbook was deactivated as the active
plan, `ideas/open/329_native_object_emission_umbrella.md` was left open with a
durable note that execution moved to child 330, and `plan.md` now points to
`ideas/open/330_native_object_model_and_emission_api.md`.

## Suggested Next

Execute Step 1 from `plan.md`: inspect current object and backend surfaces,
choose the shared API location for the native object model, and record the
focused first implementation proof command.

## Watchouts

- Do not implement RV64 or AArch64 object emission in child 330.
- Do not add `--codegen obj` CLI behavior in child 330.
- Do not make the compiler object path depend on printed `.s` parsing.
- Keep target-specific relocation names and fixup semantics out of shared
  object model records.
- Preserve existing asm-route coverage.

## Proof

Lifecycle-only switch; no build or test validation required.
