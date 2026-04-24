Status: Active
Source Idea Path: ideas/open/prealloc-agent-index-header-hierarchy-and-rust-reference-removal.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Decide Exported Top-Level Prealloc Surfaces

# Current Packet

## Just Finished

Lifecycle activation created the active runbook and execution scratchpad for
`ideas/open/prealloc-agent-index-header-hierarchy-and-rust-reference-removal.md`.

## Suggested Next

Execute Step 1: Decide Exported Top-Level Prealloc Surfaces.

Classify `prealloc.hpp`, `prepared_printer.hpp`, and
`target_register_profile.hpp` by inspecting their include users before moving
or merging any headers.

## Watchouts

- Keep this first packet inspection-oriented unless the supervisor delegates
  implementation edits.
- Do not delete `ref/claudes-c-compiler/**`.
- Do not change backend semantics or testcase expectations.
- Do not introduce one header per `.cpp`.

## Proof

Activation-only lifecycle change; no build proof required.
