Status: Active
Source Idea Path: ideas/open/573_rv64_select_phi_select_lowering.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify The Lowering Boundary

# Current Packet

## Just Finished

- Step 2 of `plan.md` classified the refreshed first lowering boundary for
  `tests/c/external/gcc_torture/src/20030408-1.c`.
- The diagnostic values are `%t80 -> %t84`, reported by prepared value ids
  `%t18 -> %t19`. Refreshed prepared BIR has `%t80 = bir.ne i32 %t79, 67`;
  it is a plain compare result, not itself a select or phi-select carrier.
  `%t84` is the selected/joined carrier value:
  `%t84.phi.sel0 = bir.select ne i32 %t70, 0, i32 1, %t80`,
  `%t84.phi.sel1 = bir.select ne i32 %t70, 0, i32 1, %t80`, and
  `%t84 = bir.select ne i32 %t54, 65, i32 %t84.phi.sel0, %t84.phi.sel1`.
- Control-flow evidence keeps this inside the active 573 select-publication
  route: `join_transfer logic.end.75 result=%t84 kind=phi_edge
  carrier=select_materialization` records incoming
  `logic.rhs.end.74 -> %t80` and edge transfer
  `logic.rhs.end.74 -> logic.end.75 incoming=%t80 destination=%t84`.
  The matching `parallel_copy` is `%t80 -> %t84` with
  `carrier=select_materialization`, and homes/storage put both value ids in
  GPR `t0`.
- The first object-route failure is therefore not the old direct owner
  `SelectInst owner=i32 %t126.phi.sel0`, and it is not operand selection,
  compare materialization, or nested select sequencing. The responsible
  boundary is result carrier publication on an out-of-SSA predecessor
  register-to-register move bundle for a select-materialized phi edge.
- This move is first after the idea 578 Step 4 repair because that repair
  handled the earlier RV64 select compare/carrier publication route and
  allowed traversal to advance to the next unresolved prepared publication
  shape. The refreshed failure now exposes generic RV64 materialization for
  a `pre_terminator_copies` register-to-register phi join copy whose carrier
  is select materialization.

## Suggested Next

- Delegate Step 3 coverage for predecessor select/phi publication: a generic
  select-materialized phi edge where the predecessor branch publishes a compare
  result into the selected join carrier via a pre-terminator out-of-SSA
  register-to-register copy. Based on current evidence, supervisor does not
  need a plan-owner split before implementation; the blocker is part of active
  573 result-carrier publication, not a separate non-select predecessor-copy
  initiative.

## Watchouts

- Do not treat `src/20030408-1.c`, `test1`, `logic.end.117`, or
  `%t126.phi.sel0` as part of the repair contract.
- Keep the repair generic over select-materialized predecessor publication.
  A patch that recognizes `logic.rhs.end.74`, `%t80`, `%t84`, `%t126`, or this
  representative file would be testcase overfit.
- Keep select-publication lowering separate from same-module call, inline asm
  carrier, floating-point binary, pointer arithmetic, branch/CFG
  reconstruction, and runtime comparison work unless new evidence proves the
  first owner moved.
- Expectation rewrites, unsupported-marker edits, allowlist changes, and
  diagnostic-only renames are not capability progress.
- The refreshed representative still contains the later nested `%t126.phi.sel*`
  publication chain, but current object-route traversal stops earlier at
  `%t80 -> %t84`. Treating Step 3 as direct scalar select materialization or
  the old `%t126.phi.sel0` owner would use stale evidence.

## Proof

- Classification proof log: `test_after.log`.
- Refreshed prepared-BIR command exited 0 and wrote:
  `build/agent_state/573_rv64_select_phi_select_lowering/step2/src_20030408-1.c/dump-prepared-bir.txt`.
  Command metadata:
  `build/agent_state/573_rv64_select_phi_select_lowering/step2/src_20030408-1.c/dump-prepared-bir.command.log`.
- `test_after.log` records the exact dump command plus excerpts for the
  `logic.rhs.end.74` / `logic.end.75` BIR, join transfer, parallel copy,
  value homes, select-chain rows, carrier-alias authority rows, and the Step 1
  object-route failure. No implementation or broad validation was run for this
  classification-only packet.
