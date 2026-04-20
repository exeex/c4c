# Execution State

Status: Active
Source Idea Path: ideas/open/58_semantic_lir_to_bir_gap_closure_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The First Semantic/BIR Seam
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Step 2 packet repaired the still-owned scalar-cast seam behind
`c_testsuite_x86_backend_src_00037_c` by teaching semantic `lir_to_bir`
lowering to preserve local pointer provenance through `ptrtoint`/`inttoptr`
and to fold pointer-difference `sub i64` when both operands came from the same
tracked local base. The slice stays upstream and general rather than
`00037`-shaped: local-array and local-slot pointer addresses can now survive
through integer casts long enough for pointer-difference math to fold in the
semantic layer. Under the delegated proof, `00037` no longer fails in the
idea-58 `scalar-cast semantic family` and instead graduates into the
authoritative prepared guard-chain handoff family.

## Suggested Next

Treat `00037` as graduated out of idea 58 and route its new
`authoritative prepared guard-chain handoff` failure to the downstream
prepared-module ownership, likely idea 59 rather than this upstream semantic
bucket. For idea 58 itself, the next packet should stay on a still-owned seam
such as `c_testsuite_x86_backend_src_00050_c` and its `load local-memory`
family instead of reopening guard-chain work here.

## Watchouts

- Keep this runbook on upstream semantic/BIR gaps; route stack/addressing
  ownership to idea 62 when that is the more precise failure family.
- Do not add x86 emitter special cases for cases that still fail before
  prepared emission exists.
- Preserve downstream routing when a case graduates into ideas 59, 60, or 61
  instead of silently keeping it under idea 58.
- The new cast handling is still bounded to tracked local/global pointer
  provenance; do not widen it into untracked integer-address arithmetic claims
  without a real semantic owner for those routes.
- The current blocker for `00037` is outside this runbook’s semantic bucket:
  it now reaches prepared x86 routing and stops in the authoritative
  guard-chain handoff family, so do not spend more idea-58 budget inside
  prepared-x86 consumption code from this packet.

## Proof

Ran the delegated proof command and preserved `/workspaces/c4c/test_after.log`:
`cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_.*|c_testsuite_x86_backend_src_00037_c)$' | tee test_after.log`.
Current result: all `74` `backend_*` tests pass, and
`c_testsuite_x86_backend_src_00037_c` no longer fails with the old idea-58
`scalar-cast semantic family` diagnostic. The proof still ends nonzero overall
because `00037` now graduates into a downstream `[FRONTEND_FAIL]`:
`error: x86 backend emitter requires the authoritative prepared guard-chain
handoff through the canonical prepared-module handoff`. For idea-58
acceptance, that route change is valid progress because the testcase has left
the owned semantic-lowering bucket. This slice is commit-ready for idea-58
runbook progress even though the delegated proof remains red on the downstream
prepared-route failure.
