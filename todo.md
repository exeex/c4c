Status: Active
Source Idea Path: ideas/open/341_aarch64_fallthrough_fixed_offset_local_load_store_emission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Fallthrough Load Store Drop Boundary

# Current Packet

## Just Finished

Lifecycle switched from idea 340 to idea 341 after runbook exhaustion. The old
scalar-cast structured-register-source diagnostic is absent, but close was not
accepted because the regression guard reported 6/7 passing both before and
after and failed its strict pass-count increase rule. The remaining `00143`
runtime residual is now owned by the active fallthrough fixed-offset local
load/store emission idea.

## Suggested Next

Execute Step 1: reproduce the `00143` runtime residual, regenerate prepared BIR
and AArch64 assembly probes, then trace one omitted fallthrough local
load/store pair from prepared BIR through AArch64 lowering, selected-node
construction, scheduling, and printer emission.

## Watchouts

- Do not reopen scalar-cast source publication unless the old structured-source
  printer diagnostic returns.
- Do not special-case `00143`, label names, block numbers, local names, source
  lines, or instruction strings.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, proof-log policy, CTest registration, plan files, or ideas
  during executor implementation.

## Proof

No implementation proof has run for idea 341 yet.
