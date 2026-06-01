Status: Active
Source Idea Path: ideas/open/69_aarch64_call_publication_prepared_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Map Prepared Call And Publication Facts

# Current Packet

## Just Finished

Activation initialized this execution state for `plan.md` Step 1.

## Suggested Next

Map the existing prepared call and publication facts against AArch64 local
decisions in `calls.cpp` and `dispatch_publication.cpp`, then identify the
first narrow migration packet.

## Watchouts

- Do not edit source ideas for routine execution findings.
- Do not move AArch64 register conversion, call spelling, copy instruction
  selection, or machine-record construction into shared prealloc.
- Treat missing publication ordering authority as a blocker or handoff to the
  dedicated ordering probe, not as permission for a broad local workaround.
- Do not weaken tests, unsupported markers, diagnostics, or supported-path
  expectations.

## Proof

Lifecycle activation only; no build or test proof required.
