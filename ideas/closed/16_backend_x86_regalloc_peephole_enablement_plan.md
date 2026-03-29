# x86 Regalloc And Peephole Enablement Plan

Status: Complete

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

Depends on:

- `ideas/closed/15_backend_x86_port_plan.md`
- `ideas/closed/03_backend_regalloc_peephole_port_plan.md`

Should precede:

- `ideas/open/23_backend_builtin_assembler_x86_plan.md`
- `ideas/open/24_backend_builtin_linker_x86_plan.md`

## Goal

Enable the already ported shared liveness/regalloc stack and x86-local peephole pipeline for the x86 backend path, starting with bounded cases where x86 currently still behaves like a stack-heavy bring-up path or leaves obvious post-codegen cleanup on the table.

## Why This Is Separate

- the shared regalloc and cleanup layers are already ported, but x86 still needs target-local attachment and proof
- x86 prologue, caller/callee-saved policy, and text-level cleanup are distinct from the completed AArch64 integration seam
- widening x86 bring-up, x86 peephole cleanup, and x86 assembler/linker work in one plan would blur the real next blocker

## Scope

- thread shared register assignments and used-callee-saved information into x86 prologue/epilogue and emit paths
- enable the first bounded x86 peephole passes that remove clearly redundant compare/move/push/pop traffic
- keep the first slice focused on correctness-preserving cleanup that the current x86 backend-owned path already exposes

## Explicit Non-Goals

- built-in assembler work
- built-in linker work
- broad new optimization work beyond the ref backend's existing cleanup shape
- unrelated local/global address-formation work unless it is required to prove the x86 regalloc boundary

## Suggested Execution Order

1. capture one x86 backend-owned case where used callee-saved information should change the prologue/epilogue
2. wire the shared regalloc result into x86 save/restore decisions
3. enable the narrowest text-level peephole passes needed by that same slice
4. stop and split again if the work expands into broader assembler or linker behavior

## Validation

- the x86 backend path consumes the shared regalloc result instead of hardcoded save/restore behavior
- targeted backend tests can show one bounded x86 cleanup improvement without fallback
- x86-local peephole passes remain post-codegen text cleanup, not a hidden instruction-selection rewrite

## Good First Patch

Use one direct-call or compare-and-branch x86 asm slice to prove that shared used-callee-saved data and one narrow peephole cleanup pass now affect the emitted assembly.

## Completion Notes

- Completed with the direct-call proof slice in `make_typed_call_crossing_direct_call_module`, where shared regalloc now chooses the surviving callee-saved register and the x86 backend saves/restores it in the emitted prologue and epilogue.
- Completed with one bounded x86 post-codegen cleanup by removing the redundant backend-owned self-move on that same direct-call slice.
- Extended the bounded compare-and-branch scaffold through the signed predicate quartet (`slt`, `sle`, `sgt`, `sge`) without widening into generic branch lowering.

## Leftover Follow-On Work

- `eq`, `ne`, and unsigned conditional-return predicates remain out of scope for this closed slice.
- Separate inventory remains open for the struct-return function-pointer IR failure uncovered during nearby validation.
