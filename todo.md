# Current Packet

Status: Active
Source Idea Path: ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Map value-materialization authority gaps

## Just Finished

Activated idea 49 into `plan.md` from
`ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md`.

## Suggested Next

Start Step 1 by mapping existing prepared authority and fallback recovery in
`src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`. Record
the first fallback route to repair and the supervisor-selected proof subset
here before delegating implementation.

## Watchouts

Keep the source idea stable during routine execution. Do not combine this value
materialization repair with the open memory, ALU, calls, comparison, or other
AArch64 follow-up scopes. Reject deeper same-block producer recursion,
value-name scans, hard-coded global-name/GOT policy recovery, local-slot
spelling recovery, and expectation downgrades as progress.

## Proof

Activation only; no build proof required yet.
