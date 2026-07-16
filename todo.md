Status: Active
Source Idea Path: ideas/open/867_lir_memory_va_object_lifetime_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select the bounded authority seam

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from
`ideas/open/867_lir_memory_va_object_lifetime_authority.md`.

## Suggested Next

Start `plan.md` Step 1. Trace current LIR memory, VA, object, and lifetime
producer/verifier seams, exclude accepted local-object, VLA, and selected
memcpy rows, and select exactly one bounded native authority gap for this idea.

## Watchouts

Do not edit Raw-BIR receiver/importer code, reopen accepted local-object/VLA/
memcpy history, or derive authority from operand spelling, printer output,
LLVM text, rendered names, or testcase identity.

## Proof

No implementation proof yet. Lifecycle activation proof: run `git diff --check`
after creating `plan.md` and `todo.md`.
