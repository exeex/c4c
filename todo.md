Status: Active
Source Idea Path: ideas/open/309_aarch64_indirect_call_argument_preservation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Callee and Argument Preservation

# Current Packet

## Just Finished

Idea 310 closed after proving the prerequisite producer fact for indirect-call
string arguments. The focused `00189.c` prepared dump now publishes
`address_materialization block=entry inst_index=4 kind=string_constant
result=@.str1 text=.str1 ...`, so idea 309 can resume without asking AArch64
lowering to guess string identity from assembly, source shape, or argument
position.

## Suggested Next

Resume `plan.md` Step 2. Re-check the focused `00189.c` prepared facts and
current AArch64 assembly, then inspect the remaining `fprintfptr`
callee/register preservation boundary before choosing the smallest semantic
call-lowering repair.

## Watchouts

- Treat the `.str1` prepared fact as repaired; do not reopen the producer route
  unless new evidence shows prepared output lost that fact again.
- The known remaining hazard is the `fprintfptr` callee register placement
  mismatch: prepared storage previously named `%t0` as `reg=x21`, while
  ordinary non-GOT `load_global` lowering loaded `fprintfptr` as `x20`.
- Also verify whether the nested result or outer-call argument homes overlap
  saved-register storage or stale prepared homes before finalizing the repair.
- Do not broaden into direct multi-argument shuffle, direct vararg aliasing,
  address-of-local direct-call preparation, expectations, allowlists,
  unsupported classifications, runner policy, CTest registration, timeout
  policy, proof logs, or test contracts.

## Proof

Close-time lifecycle proof for idea 310:

- `test_after.log` contains the focused prepared-BIR check showing `.str1`
  publication for `00189.c`.
- Supervisor-reported broader backend validation:
  `ctest --test-dir build -j --output-on-failure -R '^backend_'` passed
  139/139.
- Plan-owner close gate ran
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
  and passed with 1/1 before and 1/1 after. The semantic close signal is the
  post-CTest prepared-BIR materialization check moving from absent to present.
