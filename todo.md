Status: Active
Source Idea Path: ideas/open/347_aarch64_local_conversion_store_load_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize the Residual First Bad Fact

# Current Packet

## Just Finished

Activation created the active runbook for Step 1 from
`ideas/open/347_aarch64_local_conversion_store_load_publication.md`.

## Suggested Next

Execute Step 1: localize the first residual local conversion store/load bad
fact in `c_testsuite_aarch64_backend_src_00175_c` after the direct-call series.

## Watchouts

- Do not treat the residual as direct-call argument/formal publication without
  fresh callsite ABI or callee formal evidence.
- Do not special-case `00175`, named functions, specific registers, stack
  offsets, or one instruction sequence.
- Do not edit expectations, unsupported classifications, allowlists, CTest
  registration, timeout policy, proof-log policy, runner behavior, or root
  logs.

## Proof

Lifecycle-only activation. No build or test proof required for this packet.
