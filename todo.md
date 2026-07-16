Status: Active
Source Idea Path: ideas/open/861_lir_scalar_lhs_parameter_authority_baseline_repair.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Repair scalar LHS parameter authority baseline

# Current Packet

## Just Finished

Completed Step 1 - Repair scalar LHS parameter authority baseline. Updated the
binary producer so floating scalar LHS authority is published only for selected
`fadd`, `fsub`, and `fmul` binary consumers, floating scalar RHS authority is
published only for selected `fadd` and `fmul` consumers, and duplicate selected
floating `fadd`/`fsub` LHS rows are left unclaimed after the first publication.
Adjusted verifier absence checks to require authority only for supported
selected floating rows while keeping positive fail-closed authority validation.

## Suggested Next

Supervisor should run the broader regression/full-suite guard to confirm the
remaining scalar authority baseline failures are cleared.

## Watchouts

Focused coverage now includes unsupported `fdiv` LHS/RHS suppression and a
generated duplicate `fsub` LHS fixture that publishes exactly one selected
authority row. Existing verifier mutation tests still reject explicit unsupported
authority and duplicate selected authority publication.

## Proof

Ran `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_function_signature_type_ref$'; } > test_after.log 2>&1`.
The delegated proof passed.

Supervisor reran the full-suite guard using `test_before.log` and
`test_after.log`: before `3026/3038`, after `3038/3038`, no new failures,
regression guard passed. Rolled `test_after.log` forward to `test_before.log`.
