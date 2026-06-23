Status: Active
Source Idea Path: ideas/open/325_variadic_target_ir_abi_boundary_triage.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Prepare Minimal Evidence Fixtures

# Current Packet

## Just Finished

Lifecycle activation initialized `plan.md` from
`ideas/open/325_variadic_target_ir_abi_boundary_triage.md` and created this
executor-compatible scratchpad. No implementation or evidence collection has
started.

## Suggested Next

Begin Step 1 by creating the tiny triage fixtures under
`build/variadic_abi_triage/` and recording the fixture list plus target triples.

## Watchouts

There is no standalone `--dump-lir` in the current `c4cll` CLI. Use
`--codegen llvm` for LLVM-route evidence, then `--dump-bir` and
`--dump-prepared-bir` for backend evidence. Keep RV64 variadic diagnostics
fail-closed during this triage.

## Proof

Lifecycle-only activation. No build or test proof was required.
