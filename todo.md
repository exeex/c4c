Status: Active
Source Idea Path: ideas/open/08_calls_argument_sources_retirement.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Argument-Source Helper Ownership

# Current Packet

## Just Finished

Lifecycle activation created the runbook for Step 1. No executor packet has
completed yet.

## Suggested Next

Start Step 1 by auditing `calls_argument_sources.cpp`, `calls.hpp`, direct
callers, and matching prepared call-plan source-selection APIs.

## Watchouts

- Keep semantic source-choice authority in prepared call facts, not AArch64
  helper reshuffles.
- Do not edit implementation files, `ideas/closed/`, `review/`, or test logs
  as part of lifecycle activation.
- Treat expectation weakening, duplicate prepared/AArch64 source selection, and
  helper-only moves as route failures.

## Proof

Lifecycle-only activation. No build proof required.
