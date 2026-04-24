# Advanced Prepared Call Authority And Grouped-Width Allocation

Status: Open
Created: 2026-04-24
Last-Updated: 2026-04-24
Parent Idea: [88_prepared_frame_stack_call_authority_completion_for_target_backends.md](/workspaces/c4c/ideas/closed/88_prepared_frame_stack_call_authority_completion_for_target_backends.md)

## Intent

Finish the next layer of target-independent prepared authority by covering the
remaining advanced call/frame families and making grouped register allocation
real for width-greater-than-one values instead of stopping at bank/span
substrate scaffolding.

## Owned Failure Family

This idea owns prealloc gaps where:

- scalar frame/stack/call authority already exists for the covered baseline
  cases
- the remaining missing authority is in advanced call families such as
  variadic, byval, indirect result, memory return, or richer before/after-call
  move publication
- grouped register bank/span/storage contracts exist, but width `> 1`
  allocation, spill, reload, or call-boundary behavior is not yet exercised as
  truthful allocator behavior
- the first bad fact is still in prepared authority or regalloc publication,
  not in x86 codegen spelling

## Scope Notes

Expected repair themes include:

- stronger prepared call authority for variadic, byval, indirect-result, and
  memory-return families
- clearer before-call and after-call move publication for advanced call cases
- width-aware grouped allocation using published bank/span legality
- grouped spill/reload and call-boundary behavior for width `> 1`
- tests proving that advanced call publication and grouped allocation are both
  consumed without backend-local policy reconstruction

## Boundaries

This idea does not own:

- x86-side behavior recovery once truthful prepared authority is already
  published
- frontend type lowering for any one vector ISA
- reworking liveness into physical-resource modeling; liveness stays a
  lifetime-only phase
- out-of-SSA critical-edge and parallel-copy deepening, which belongs to a
  separate follow-on idea

## Completion Signal

This idea is complete when advanced call/frame families publish truthful
prepared authority, width-aware grouped allocation works as real allocator
behavior, and target backends can consume both without reopening ABI or
physical-resource policy locally.
