Status: Active
Source Idea Path: ideas/open/12_dispatch_value_materialization_authority.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Audit Materialization Helpers and Clients

# Current Packet

## Just Finished

Lifecycle activation created this execution scratchpad for Step 1. No
implementation work has started.

## Suggested Next

Start Step 1 by auditing `dispatch_value_materialization.hpp`,
`dispatch_value_materialization.cpp`, `dispatch.cpp`, and direct clients of the
materialization header. Record helper ownership classifications, proposed first
move packet, and focused proof candidates here before editing code.

## Watchouts

- Keep this route focused on value materialization authority.
- Do not fold in edge-copy cleanup, publication cleanup, calls cleanup, or
  broad AArch64 pipeline redesign.
- Treat expectation weakening, unsupported downgrades, text-emission contracts,
  and named-test shortcuts as route failures.
- Leave unrelated transient `review/` artifacts untouched.

## Proof

Not run; lifecycle activation only.
