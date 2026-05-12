Status: Active
Source Idea Path: ideas/open/202_hir_generated_member_payload_structured_miss.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Add Focused Coverage

# Current Packet

## Just Finished

Step 3 - Add Focused Coverage completed by recording the focused coverage
status for the generated static-member structured-miss behavior. The preceding
Step 2 slice already added the needed focused tests for rendered const-value
fallback and rendered decl/init fallback on metadata-rich generated static
members.

Those tests prove that stale rendered owner/member spelling cannot repair a
structured generated-member miss: once complete owner/member metadata produces
a structured lookup key, a structured miss falls through as an ordinary
reference instead of recovering authority through rendered fallback spelling.
No duplicate Step 3 tests are needed from this packet.

## Suggested Next

Supervisor should treat Step 3 as complete, review the combined Step 2/Step 3
slice, and choose whether to commit or route lifecycle closure/review.

## Watchouts

- Idea 195 remains open but inactive/blocked until ideas 201 and 202 are
  resolved or explicitly fenced.
- Do not close idea 195, run backend restart work, or advance milestone
  validation from this blocker runbook.
- Treat a rendered owner/member fallback on metadata-rich generated-member
  paths as a blocker unless it is fenced as explicit no-metadata compatibility.
- Do not claim progress through expectation changes, unsupported markings, or
  helper renames that preserve rendered owner/member authority.
- The rendered overloads in `hir_types.cpp` are documented as no-owner
  compatibility mirrors; the issue traced here is that `scalar_control.cpp`
  calls those rendered overloads after constructing complete structured keys.
- This slice intentionally does not downgrade expectations or add
  testcase-shaped matching; the fence is keyed to structured lookup-key
  availability for each candidate.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_lookup_tests$' > test_after.log 2>&1`

Result: already passed in the preceding Step 2 slice and preserved at
`test_after.log`. Supervisor regression guard passed against `test_before.log`
with `--allow-non-decreasing-passed`; the accepted hook full-suite baseline
candidate at commit `4bbc48abc` passed 3137/3137. This proof is sufficient for
Step 3 focused coverage recording; no blocker or missing proof remains.
