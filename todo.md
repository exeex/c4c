# Execution State

Status: Active
Source Idea Path: ideas/open/58_semantic_lir_to_bir_gap_closure_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Refresh Remaining Idea-58 Ownership And Pick The Next Seam
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Step 2.1 re-audited `00204` after the accepted long-double aggregate-global
repair and confirmed the case has durably graduated out of idea 58's
bootstrap-global lane. The frontend LLVM route for `fa3` now lowers `%p.b` and
`%p.c` as byval aggregate pointers, takes member GEPs from `%p.c`, loads two
`x86_fp80` fields, and feeds them into the variadic `printf` call. That keeps
the current blocker in byval stack/member-addressing semantics around local
aggregate access rather than in idea 58's earlier semantic-global ownership.

## Suggested Next

Route `00204` to idea 62 for the next implementation packet. The next coherent
work item should shrink the byval aggregate/member-addressing seam exposed by
`fa3` and protect it with the nearest backend coverage, while idea 58 stays on
remaining cases that still truly fail in the broad semantic-lowering bucket
before stack/addressing-specific ownership is established.

## Watchouts

- The aggregate-global lane now covers zero-sized members, string-backed
  integer-array fields, and `x86_fp80` fields lowered as honest 16-byte storage
  with little-endian payload bytes plus tail padding.
- `00204` is no longer idea-58 bootstrap-global residue. Its durable blocker is
  function `fa3`'s byval local/member-addressing path for `%p.c`'s
  `x86_fp80` fields on the way into variadic `printf`.
- Do not "fix" the long-double lane by inventing direct scalar `F128` globals;
  the shared aggregate path should keep long-double fields byte-addressable
  until a broader direct-scalar contract exists.
- Keep this runbook on upstream semantic/BIR gaps; do not reopen `00204` under
  idea 58 unless it re-enters the bootstrap-global diagnostic family.
- Do not treat the next packet as generic variadic cleanup unless the stack and
  member-addressing proof shows the `x86_fp80` byval access path is already
  healthy.

## Proof

Canonical proof remains the accepted packet artifact in
`/workspaces/c4c/test_after.log` from:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|c_testsuite_x86_backend_src_00204_c)$' | tee test_after.log`.
Follow-up audit evidence for the routing decision:
`./build/c4cll --codegen llvm --target x86_64-unknown-linux-gnu tests/c/external/c-testsuite/src/00204.c -o /tmp/00204.ll`
shows `fa3` as
`define void @fa3(%struct.hfa14 %p.a, ptr byval(%struct.hfa23) align 8 %p.b, ptr byval(%struct.hfa32) align 16 %p.c)`
with member GEP/load on `%p.c` before the variadic `printf` call. That makes
the surviving `store local-memory semantic family` failure a stack/member-
addressing route fact, not a reopened idea-58 aggregate-global gap.
