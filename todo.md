# Execution State

Status: Active
Source Idea Path: ideas/open/61_call_bundle_and_multi_function_prepared_module_consumption.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Prepared-Module And Call-Bundle Boundaries
Plan Review Counter: 2 / 10
# Current Packet

## Just Finished

Step 1 audit completed. `c_testsuite_x86_backend_src_00040_c` still fails at
the prepared-module front door before prepared call-bundle handling matters,
while `backend_x86_handoff_boundary` stays green. The first bounded repair seam
is generic multi-function prepared-module traversal in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` and
`src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`, not new x86-only
`main + helper` matching and not immediate call-bundle contract growth.

## Suggested Next

Implement the first Step 2 slice by replacing the current bounded
multi-defined-module gate with generic per-function prepared traversal over the
existing prepared module inventory, then prove it with
`backend_x86_handoff_boundary` plus `c_testsuite_x86_backend_src_00040_c`
before touching prepared call-bundle contract surfaces.

## Watchouts

- Do not reopen idea-62 stack/addressing work unless a new case reproduces a
  pre-prepared semantic lowering miss.
- Do not add another x86-only `main + helper` acceptance lane to get `00040`
  through prepared emission.
- The existing `BeforeCall` / `AfterCall` boundary coverage already shows the
  authoritative call-bundle contract is exercised separately, so the next
  packet should treat multi-function traversal as the first root cause and only
  grow shared prepared contract fields if traversal work proves the inventory
  itself is too weak.

## Proof

Selected the first idea-61 proof command and refreshed
`/workspaces/c4c/test_before.log` with it:
`cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00040_c)$'`.
Current baseline result: `backend_x86_handoff_boundary` passes and
`c_testsuite_x86_backend_src_00040_c` fails with the single-function prepared
module restriction. No new `test_after.log` was produced in this audit-only
slice.
