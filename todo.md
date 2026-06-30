Status: Active
Source Idea Path: ideas/open/428_rv64_call_adjacent_scalar_inline_asm_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Call-Adjacent Evidence And Choose First Packet

# Current Packet

## Just Finished

Activation created `plan.md` from the source idea and initialized this
executor-compatible packet skeleton for Step 1.

## Suggested Next

Execute Step 1: inspect the call-adjacent evidence rows, classify the first
coherent scalar call/inline-asm implementation packet, and record the selected
target plus proof command here before code changes.

## Watchouts

- Do not implement code during lifecycle activation.
- Keep aggregate `sret`/`byval`, F128 helper-call behavior, missing prepared
  metadata reconstruction, and expectation/accounting changes out of this
  plan.
- Do not key the route to representative filenames such as `src/pr38533.c`.

## Proof

Lifecycle-only activation. Run `git diff --check` before supervisor commit.
