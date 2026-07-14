# Current Packet

Status: Active
Source Idea Path: ideas/open/769_lir_global_initializer_label_address_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Diagnose the expanded full-suite regression gate
你該做test baseline review了

## Just Finished

- Step 2 completed: `LirGlobal::initializer_elements` now publishes each
  static label-address initializer as `{LinkNameId enclosing_function,
  LirBlockId target}` from the HIR constant/global-lowering walk. The LIR
  verifier requires exactly one matching function owner and exactly one block
  in that function. Direct frontend-LIR coverage proves structured positive
  publication and malformed owner/target rejection.

- The Step 2 focused `frontend_lir` proof passed 5/5 in commit `56d86556a`,
  but it is not final acceptance: the post-commit full-suite candidate
  `test_baseline.new.log` regressed from the green `test_baseline.log` baseline
  (3034/0 to 3035) with four new failures: `llvm gcc torture 20040302_1`,
  `20041214_1`, `920501_4`, and `920501_5`.

## Suggested Next

- Reproduce the four expanded-suite failures exactly and establish whether
  `56d86556a`'s structured global-initializer authority caused them. Investigate
  the production authority path generically; no further commit or Raw-BIR/
  importer handoff decision is permitted until the expansion is eliminated or
  an evidence-backed, separately scoped downstream importer blocker is created
  and activated atomically.

## Watchouts

- `init_text` remains display/compatibility spelling and is not used as
  label-target authority. Raw-BIR/importer global lowering still has no field
  for the structured element, so any integration consumption remains a
  separately scoped downstream blocker if Step 3 proves it is the cause; do
  not assume that boundary explains the new failures.

- The hook rejected `test_baseline.new.log` against `test_baseline.log`; the
  four-failure expansion is a live acceptance blocker, not a baseline roll
  forward candidate.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_' > test_after.log` passed (5/5,
  including `frontend_lir_global_label_address_initializer`); log:
  `test_after.log`.

- Reproduction and prevention proof required before another commit: run the
  exact expanded-suite comparison that produced `test_baseline.new.log`, show
  none of the four named cases are newly failing versus `test_baseline.log`,
  and retain direct focused positive/malformed frontend-LIR proof. If a
  Raw-BIR/importer boundary is proven, instead preserve the reproducer and
  causality evidence in the separately scoped blocker before any handoff.
