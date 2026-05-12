Status: Active
Source Idea Path: ideas/open/202_hir_generated_member_payload_structured_miss.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Enforce Structured-Miss Fail-Closed Behavior

# Current Packet

## Just Finished

Step 2 - Enforce Structured-Miss Fail-Closed Behavior completed. In
`src/frontend/hir/impl/expr/scalar_control.cpp`, the static-member decl and
const-value lambdas now track whether a candidate produced a structured
`HirStructMemberLookupKey`; once it did, a miss no longer falls through to the
rendered `find_struct_static_member_decl(lookup_owner, candidate.name)` or
`find_struct_static_member_const_value(lookup_owner, candidate.name)` overloads.
Rendered fallback remains fenced to candidates that did not produce structured
member metadata.

`tests/frontend/frontend_hir_lookup_tests.cpp` now covers metadata-rich
generated static-member misses for both rendered const-value fallback and
rendered decl/init fallback. In both cases, complete structured owner/member
metadata misses fall through as ordinary references instead of recovering stale
rendered static-member authority.

## Suggested Next

Supervisor should review and commit this Step 2 slice, then choose the next
packet from the active runbook.

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

Result: passed. Focused proof log preserved at `test_after.log`.
