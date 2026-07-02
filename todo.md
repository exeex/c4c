Status: Active
Source Idea Path: ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Existing Evidence And Missing Artifacts

# Current Packet

## Just Finished

Activation created the runbook for
`ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md`.

## Suggested Next

Execute Step 1 by inventorying `docs/rv64_gcc_torture_post_contract/` against
the umbrella acceptance criteria, then record the first missing or stale
artifact packet here.

## Watchouts

- Do not implement RV64 fixes in this umbrella.
- Keep RV64 gcc_torture as external evidence, not a default CTest gate.
- Keep primary-F128 rows screened into the F128 quarantine lane unless fresh
  evidence proves broad non-F128 impact.
- Do not weaken unsupported markers, allowlists, expected output, runtime
  comparison, pass/fail accounting, or default CTest contracts.
- Keep `ideas/open/426_f128_quarantine_and_external_softfloat_policy.md` as
  the existing low-priority F128 policy lane.
- Leave `review/global_address_helper_cleanup_review.md` untouched.
- Treat `test_baseline.new.log` as a rejected full-suite candidate, not an
  accepted baseline.

## Proof

Activation-only lifecycle work. Run `git diff --check -- plan.md todo.md`
before committing.
