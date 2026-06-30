Status: Active
Source Idea Path: ideas/open/437_rv64_runtime_external_abort_exit_support.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Classify Runtime External Policy And First Packet

# Current Packet

## Just Finished

Activated `ideas/open/437_rv64_runtime_external_abort_exit_support.md` as the
next active plan. Selection rationale: it is the concrete residual split from
the just-closed aggregate `sret` plan, while the runtime-mismatch triage idea
excludes unsupported compile/codegen failures.

## Suggested Next

Execute Step 1: classify the RV64 policy for runtime external `abort`/`exit`
calls and choose the first bounded implementation or fail-closed contract
packet.

## Watchouts

- Do not claim aggregate `sret` progress through runtime external support.
- Do not special-case `20000917-1`, `20020206-1`, `main`, `baz`, or one dump
  shape.
- Keep F128, softfloat, inline asm, select, local-publication, pointer, and
  aggregate ABI work out of this plan.
- Preserve fail-closed diagnostics for unsupported runtime externals outside
  the accepted policy.

## Proof

Activation-only lifecycle change. Required local check:

```sh
git diff --check
```
