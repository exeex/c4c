Status: Active
Source Idea Path: ideas/open/184_direct_call_signature_metadata_structured_boundary.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Add focused direct-call tests

# Current Packet

## Just Finished

Step 3 coverage completed: reviewed the Step 2 frontend/backend direct-call
tests and added the missing backend semantic coverage for aggregate-sensitive
generated direct calls.

Backend notes coverage now proves:

- structured direct calls still ignore stale rendered suffixes;
- metadata-rich direct calls reject missing structured callee signatures;
- structured direct byval aggregate calls materialize BIR byval ABI facts from
  `arg_type_refs` instead of treating the argument as a plain pointer;
- mismatched structured aggregate metadata on that direct byval path fails
  closed in the direct-call semantic family;
- the existing raw/no-metadata byval fixture without structured mirrors still
  lowers through the explicit compatibility path.

The new aggregate-sensitive test exposed a small Step 2 direct-call lowering
bug: generated-style `ptr` signatures with `ptr byval(...)` arguments were
taking the plain-pointer path before consulting structured aggregate mirrors.
The direct-call branch now handles explicit byval arguments before the pointer
fast path.

## Suggested Next

Execute Step 4 from `plan.md`: validate and prepare the idea 184 handoff,
including any supervisor-selected broader proof or lifecycle-close review.

## Watchouts

- Direct calls with `direct_callee_link_name_id` and no `callee_signature` now
  fail closed; raw/no-metadata compatibility should keep `direct_callee_link_name_id`
  invalid if it intentionally relies on rendered call text.
- The direct-call byval fix was intentionally placed before the plain pointer
  branch only on the direct-call lowering path covered by idea 184.
- `lower_extern_decl` still prefers `return_type_str` before `return_type`; this
  is adjacent compatibility behavior, not necessarily the direct-call Step 2
  repair point.
- Backend freeze closure remains owned by idea 188.

## Proof

Supervisor-selected proof passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|backend_)' > test_after.log 2>&1`

Proof log: `test_after.log`
