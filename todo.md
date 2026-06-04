Status: Active
Source Idea Path: ideas/open/100_aarch64_00204_stdarg_hfa_runtime_repair.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Repair AArch64 Outgoing Stack Argument Lifetime

# Current Packet

## Just Finished

Plan-owner rewrite split the route after reviewer report
`review/aarch64_outgoing_stack_lifetime_route.md`. The active packet is now
`plan.md` Step 5, not the old targeted proof step.

## Suggested Next

Execute `plan.md` Step 5. Choose the outgoing stack argument lifetime invariant
before implementation, add focused lifetime contract coverage, and prove the
repair without weakening `00204.c` supported-path expectations.

## Watchouts

- Preserve the current source idea; the reviewer found the repair still aligned
  with `ideas/open/100_aarch64_00204_stdarg_hfa_runtime_repair.md`.
- Do not repair by padding HFA float lanes, changing `00204.c` expectations, or
  matching the literal testcase output shape.
- The packet must cover prepared-frame-slot sources copied into outgoing stack
  arguments, including at least one non-HFA stack argument case and the current
  HFA overflow representative.

## Proof

No code validation run for this lifecycle-only rewrite.
