Status: Active
Source Idea Path: ideas/open/231_aarch64_call_frame_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Prepared Facts And Current Dispatch

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from
`ideas/open/231_aarch64_call_frame_machine_nodes.md`.

## Suggested Next

Start Step 1 by inspecting prepared call/frame facts, current AArch64 call
dispatch, selected machine records, and printer surfaces. Record the concrete
implementation targets and first proof subset in `todo.md` before code edits.

## Watchouts

- Do not reconstruct call ABI classification, frame layout, outgoing stack
  areas, callee-save slots, sret storage, or scratch policy inside AArch64
  codegen.
- Do not implement full variadic function-entry `va_list` behavior in this
  route.
- Reject named-case call/frame shortcuts, weakened expectations, or text-only
  printer patches as progress.

## Proof

Lifecycle-only activation; no build or test proof required.
