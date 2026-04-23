# Prepared Frame-Stack-Call Authority Completion For Target Backends

Status: Open
Created: 2026-04-23
Last-Updated: 2026-04-23
Parent Idea: [86_full_x86_backend_contract_first_replan.md](/workspaces/c4c/ideas/open/86_full_x86_backend_contract_first_replan.md)

## Intent

Complete the target-independent prepared authority for frame, stack, and call
behavior so backends consume final plans instead of rebuilding stack/frame/call
policy locally.

## Owned Failure Family

This idea owns prealloc contract gaps where:

- BIR and prealloc already know enough to decide frame, stack, or call policy
- target backends still need to reconstruct part of that policy because the
  published prepared facts are too fragmented or underspecified
- dynamic stack regions, fixed-frame anchoring, call arg/result placement, or
  call-clobber/save ownership are not yet published as executable authority

## Scope Notes

Expected repair themes include:

- strengthening `frame_plan`, `dynamic_stack_plan`, `call_plans`, and
  `storage_plans`
- publishing clearer before-call and after-call move obligations
- making fixed-frame vs dynamic-stack coexistence explicit
- clarifying authoritative callee-save/caller-save usage and call clobber
  publication
- covering variadic, byval, indirect call, memory-return, and nested dynamic
  stack cases with BIR/prepared tests and dumps

## Boundaries

This idea does not own:

- x86 instruction spelling or prologue/epilogue text emission once truthful
  plans are published
- register-group alias/span modeling beyond what frame/stack/call plans need
  to reference
- frontend lowering of new source-language constructs unless the first bad fact
  is still upstream of BIR

## Completion Signal

This idea is complete when frame, stack, and call decisions are published as
durable prepared authority that target backends can trust directly, including
dynamic stack and call-boundary behavior, without reopening ABI or frame-policy
reasoning locally.
