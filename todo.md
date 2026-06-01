Status: Active
Source Idea Path: ideas/open/78_aarch64_edge_copy_typed_stack_source_prepared_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Consume Prepared Typed Stack-Source Authority

# Current Packet

## Just Finished

Step 2: consumed prepared same-width I32 typed stack-source publication
authority in
`src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`.

`lower_predecessor_join_source_publication` now computes
`prepare::prepare_same_width_i32_stack_source_publication(&publication)` after
destination-register parsing and scratch selection. When that fact is
`Available`, the AArch64 edge-copy path emits a target-local `ldr` from the
prepared stack offset into the prepared destination GPR placement and records
that register in scalar state. Unsupported typed-stack-source statuses continue
through the existing recursive publication fallback.

No ownership expansion into `dispatch_publication.cpp` or `src/backend/prealloc`
was needed.

## Suggested Next

Run supervisor review/acceptance for Step 2 or delegate the next plan step if
the current slice is accepted.

## Watchouts

- Do not thread `PreparedEdgePublication` into `dispatch_publication.cpp` just
  to satisfy this route.
- Do not add named testcase shortcuts or weaken expected supported behavior.
- Keep AArch64 load/copy/register emission target-local.
- The new fast path intentionally accepts only `Available` same-width I32
  typed stack-source facts; every other status falls back to the existing
  recursive publication route.

## Proof

Passed:

`cmake --build --preset default > test_after.log && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log`

`test_after.log` contains the delegated proof. Backend subset result:
169/169 tests passed.
