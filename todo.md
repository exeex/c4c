Status: Active
Source Idea Path: ideas/open/126_aarch64_memory_va_list_field_local_owner.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Characterize the Current Route

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from Step 1 of `plan.md`.

## Suggested Next

Execute Step 1: characterize the current `va_list` field memory route in
`src/backend/mir/aarch64/codegen/memory.cpp`, identify focused proof coverage,
and record the exact proof command before implementation changes.

## Watchouts

- Keep this as an AArch64-local ownership cleanup.
- Do not move or recompute shared prepared variadic facts.
- Do not weaken tests or rewrite expectations to claim progress.

## Proof

Lifecycle-only activation; no build or code validation run.
