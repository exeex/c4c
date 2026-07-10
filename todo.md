Status: Active
Source Idea Path: ideas/open/668_llvm_torture_20040709_research.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Build Research Index And Validate Scope

# Current Packet

## Just Finished

Plan-owner close review for Step 3 rejected lifecycle close only because the
required close-time regression guard is not currently satisfiable from the
canonical logs under the delegated constraints. The research package itself is
complete: `docs/llvm_torture_20040709_owner_discovery/index.md` links both
numbered answer files, summarizes the result without replacing either answer,
and the Step 3 proof showed the output directory contains exactly the required
three documentation files.

## Suggested Next

Ask the supervisor to provide or authorize matching canonical regression logs
for the close gate, then re-run the plan-owner close decision. The existing
`test_before.log` is a backend CTest run while `test_after.log` is the Step 3
documentation proof, so
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`
exits `2` with `ERROR: Could not find summary line in test_after.log`.

The source idea appears complete under its research acceptance criteria. Any
later implementation work should be a separate direct implementation idea.

## Watchouts

- This is research/documentation work only.
- Do not edit implementation files, tests, expectations, unsupported markers,
  allowlists, runtime behavior, baseline acceptance, or lifecycle history.
- The package concludes that neither LLVM torture row has a proven existing
  generated implementation owner.
- Do not create an implementation follow-up inside this research lifecycle.
- If follow-up is accepted later, start with row-specific generated IR,
  prepared/BIR, target/object, and runtime-abort evidence before naming a first
  owner.

## Proof

Step 3 previously ran the supervisor-selected proof command:

```sh
(find docs/llvm_torture_20040709_owner_discovery -maxdepth 1 -type f -printf '%f\n' | sort; git diff --name-only -- todo.md docs/llvm_torture_20040709_owner_discovery; test "$(find docs/llvm_torture_20040709_owner_discovery -maxdepth 1 -type f | wc -l)" -eq 3; test -f docs/llvm_torture_20040709_owner_discovery/index.md; test -f docs/llvm_torture_20040709_owner_discovery/01_current_failure_boundary.md; test -f docs/llvm_torture_20040709_owner_discovery/02_owner_mapping.md) > test_after.log 2>&1
```

Result: exit `0`. `test_after.log` lists exactly the three documentation files
in the output directory, reports the scoped tracked diff for `todo.md`, and
confirms all three required files exist. The new untracked `index.md` is proven
by the directory listing and `test -f` checks rather than by `git diff`.
Proof log: `test_after.log`.

Close gate attempted:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log
```

Result: exit `2`. Existing canonical logs do not form a matching CTest
before/after pair for closure, and this delegated packet explicitly forbids
touching `test_before.log` or `test_after.log`.
