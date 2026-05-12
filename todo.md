Status: Active
Source Idea Path: ideas/open/184_direct_call_signature_metadata_structured_boundary.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Validate and prepare handoff

# Current Packet

## Just Finished

Step 4 validation completed for idea 184: rebuilt the default preset and reran
the supervisor-selected focused frontend/backend coverage after the direct-call
signature metadata implementation and tests.

The validation result supports handoff for lifecycle close/switch review:
metadata-rich generated direct calls now require structured callee signature
facts, aggregate-sensitive byval direct-call lowering is covered, stale or
missing structured metadata fails closed on the generated path, and the raw
no-metadata compatibility path remains explicitly covered.

## Suggested Next

Supervisor should review and, if accepted, hand idea 184 to the plan owner for
lifecycle close/switch handling. Backend freeze closure remains out of scope for
this idea and should continue through the later freeze-gate idea.

## Watchouts

- Direct calls with `direct_callee_link_name_id` and no `callee_signature` now
  fail closed; raw/no-metadata compatibility should keep `direct_callee_link_name_id`
  invalid if it intentionally relies on rendered call text.
- The direct-call byval fix was intentionally placed before the plain pointer
  branch only on the direct-call lowering path covered by idea 184.
- `lower_extern_decl` still prefers `return_type_str` before `return_type`; this
  is adjacent compatibility behavior, not necessarily the direct-call Step 2
  repair point.
- Existing full-suite candidate `test_baseline.new.log` is not a usable
  acceptance baseline for this slice because it records 9 failures out of 3137
  tests. The accepted proof for this packet is the supervisor-selected
  frontend/backend subset in `test_after.log`.
- Backend freeze closure remains owned by idea 188.

## Proof

Supervisor-selected proof passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|backend_)' > test_after.log 2>&1`

Proof log: `test_after.log`

Result: 100% tests passed, 0 failed out of 110. Disabled backend CLI trace
tests remained disabled and were not part of the selected subset.
