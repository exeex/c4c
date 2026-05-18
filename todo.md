# AArch64 Scalar Expression Control-Value Authority Todo

Status: Active
Source Idea Path: ideas/open/292_aarch64_scalar_expression_control_value_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Locate Scalar Authority Breaks

# Current Packet

## Just Finished

Lifecycle switch complete. The AArch64 c-testsuite inventory umbrella selected
this focused scalar expression/control-value authority idea, and active
`plan.md` now points here.

## Suggested Next

Begin Step 1 by reproducing the starter representative subset with an explicit
timeout, then inspect generated AArch64 assembly to locate the first semantic
scalar authority repair target.

## Watchouts

- Do not implement from the old umbrella inventory idea.
- Keep pointer/aggregate address failures, timeout/hang cases, and
  compile-stage printer gaps out of the first scalar owner unless fresh proof
  shows the same scalar authority primitive owns them.
- Reject filename-shaped repairs. The route must repair scalar value authority
  across expression results, branch/control values, return values, and scalar
  call arguments.

## Proof

Lifecycle-only switch. No implementation validation was run.
