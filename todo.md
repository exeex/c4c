Status: Active
Source Idea Path: ideas/open/21_x86_prepared_edge_publication_consumer.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Wire One Narrow Lowering Behavior

# Current Packet

## Just Finished

Completed Step 3 by wiring the x86 compare-join parallel-copy path through
`x86::prepared::append_edge_publication_move_instruction`, which reuses
`consume_edge_publication_move_intent` and appends target x86 move text only
from shared `edge_publications`. Missing shared lookup or missing publication
authority remains an explicit handoff error instead of falling back to local
edge-copy reconstruction. Tightened the proof with a module-level joined-branch
test that emits through `x86::api::emit_prepared_module(...)`, observes the
shared-publication-derived `mov ebx, DWORD PTR [rsp + 56]` /
`mov ebx, DWORD PTR [rsp + 64]` moves, and rejects a drifted publication
destination.

## Suggested Next

Run Step 4 validation as a supervisor packet: keep the current focused x86 and
shared lookup subset as the baseline, and add broader validation only if the
supervisor wants milestone confidence beyond the x86-owned module/prepared
diff.

## Watchouts

- The wired compare-join path emits only for the currently supported
  stack-slot source to register destination form; unsupported publication/home
  shapes keep the existing prepared-bundle validation but do not invent x86
  moves.
- The focused fixture now checks that the append step writes
  `mov ebx, DWORD PTR [rsp + 56]` from shared lookup authority and emits
  nothing when `ConsumedPlans` lacks shared lookups.
- The joined-branch handoff test now exercises the actual `module.cpp` route
  through `emit_prepared_module`, so supervisor acceptance no longer depends
  only on the direct prepared-helper fixture.
- Step 4 should treat `test_after.log` as the executor proof log and decide
  whether this x86-only slice needs broader than the delegated focused subset.

## Proof

Ran exactly:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_|backend_codegen_route_x86_64_.*|backend_prepared_lookup_helper|backend_prealloc_block_entry_publications)$' >> test_after.log 2>&1`

Result: passed, 77 focused tests. Proof log: `test_after.log`.
