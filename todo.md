Status: Active
Source Idea Path: ideas/open/302_aarch64_scalar_machine_node_operand_forms.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reactivation Review And Focused Evidence Check

# Current Packet

## Just Finished

Activated idea 302 after ideas 303, 304, and 305 were closed. No
implementation files, source ideas, closed ideas, test logs, or commit history
were changed.

## Suggested Next

Run Step 1 reactivation review. Decide whether the scalar machine-node
operand-form owner is now a valid close candidate by checking committed proof
for `00064`, `00139`, and `00205`, confirming the old scalar `div`, scalar
`mul`, and scalar `logical_shift_right` unsigned-reduction diagnostics are
absent, and verifying that any later residuals belong outside idea 302.

## Watchouts

- Do not claim idea 302 completion from the closed split owners alone; verify
  the original scalar operand-form acceptance criteria.
- Closed owners 303, 304, and 305 should stay closed unless generated-code
  evidence contradicts their closure boundaries.
- Keep idea 295 as the umbrella inventory for later backend-regex
  classification; implementation work here must remain scoped to idea 302.
- Do not edit implementation files, test logs, expectations, allowlists,
  unsupported classifications, timeout policy, runner behavior, proof-log
  policy, or CTest registration during the reactivation review.

## Proof

Activation only; no build, CTest, or regression guard was run in this packet.
