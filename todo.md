Status: Active
Source Idea Path: ideas/open/240_aarch64_inline_asm_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Remaining-Scope Validation And Lifecycle Decision

# Current Packet

## Just Finished

Plan Step 6 - Remaining-Scope Validation And Lifecycle Decision: ran matching
backend-scope validation for the remaining-scope runbook after named operand
substitution and tied-home validation.

## Suggested Next

Ask the plan owner to decide lifecycle state. The current backend-local
remaining-scope runbook has completed named operand substitution and tied-home
validation, but clobber ingress and memory/address constraints still appear to
need source/LIR, prepared-home, or allocator ownership before true backend
support can continue. Do not close the source idea unless those leftovers are
split into durable follow-up ownership or judged outside the source idea.

## Watchouts

- Do not touch `plan.md` or `ideas/open/240_aarch64_inline_asm_machine_nodes.md`
  for routine packet progress.
- Completed backend-local support now includes structured positional operands,
  named operand substitution, one register output, numeric tie facts with
  concrete prepared-home agreement, integer immediates, `%wN`/`%xN` modifiers,
  side-effect marking, and explicit diagnostics for unsupported forms.
- Clobbers remain diagnostic-only because inline-asm source/LIR/BIR still lacks
  structured clobber authority.
- Memory/address constraints remain diagnostic-only until prepared homes expose
  target-valid memory/address authority for inline-asm operands.
- Alias-aware tied-home coallocation, scratch allocation, and spill policy
  remain allocator/prepared-home policy, not printer behavior.

## Proof

Ran matching backend validation twice for regression comparison:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_before.log 2>&1`

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed; both logs contain 139/139 backend tests passing.

Regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: passed with no new failures and no new slow-test entries.
