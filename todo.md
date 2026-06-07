Status: Active
Source Idea Path: ideas/open/118_aarch64_calls_deferred_cluster_post_contract_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Collect Closed-Route Evidence

# Current Packet

## Just Finished

Step 1 of `plan.md` collected closed-route evidence for idea 118.

Evidence inventory:

- Idea 92 deferred these calls-side clusters for later shared-authority
  evidence: before-call move bundle lowering; after-call, return, value, and
  preservation lowering; scalar producer dispatch bridge; result recording and
  late publication. It also kept ordinary call-boundary move and ABI-binding
  record construction local unless directly coupled to one of those deferred
  clusters.
- Idea 93 closed the stack/frame-slot operand owner. Duplicate-work guardrail:
  do not reopen stack call argument destinations, selected frame-slot sources,
  sret/local-frame address sources, aggregate stack sources, endpoint stack
  storage, prior stack-preserved sources, or memory-return frame-slot storage;
  future calls work must consume that local owner instead of reselecting homes
  or rebuilding prepared call, move-bundle, result, preservation, or aggregate
  transport facts under a new name.
- Idea 94 closed the f128 carrier operand owner. Duplicate-work guardrail: do
  not reopen prepared f128 carrier completion, module fallback lookup, or
  q-register operand rendering; future calls work must not invent new shared
  f128 transport authority or absorb ordinary call-boundary record ownership
  through an f128 helper.
- Idea 95 closed the immediate scalar argument publication owner.
  Duplicate-work guardrail: do not reopen same-block immediate cast lookup,
  immediate bit extraction, GP/FP destination views, FP immediate scratch
  materialization, or inline-asm instruction construction; keep
  `lower_before_call_immediate_binding` as the prepared immediate consumer.
- Idea 114 adds a prepared outgoing stack argument area fact owned by shared
  prealloc/call planning. Calls-side follow-ups may consume the call-level area
  as reservation/address authority, while AArch64 still owns `x16`, stack
  adjustment/restoration, store ordering, and target ABI placement. Guardrail:
  do not duplicate outgoing stack area work or rederive the total area from
  per-argument destinations in a new calls owner.
- Idea 116 adds shared prepared dispatch facts for edge-copy materializable
  producer classification, edge-publication source consistency, current-block
  join instruction routing, and direct-global select-chain dependency lookup.
  Guardrail: do not reopen dispatch prepared-producer, current-block
  publication, join-routing, or select-chain contract work; calls-side scalar
  producer bridge analysis should look for consumers of those shared facts, not
  new AArch64 producer discovery.
- Idea 117 adds shared prepared comparison facts for fused-compare operand
  producers, materialized compare join/publication facts, and current-block
  publication registers. Guardrail: do not reopen comparison publication or
  fused-compare routing; calls-side result/publication analysis may consume the
  current-block publication contract but must keep AArch64 compare/branch
  spelling out of shared code.

## Suggested Next

Execute Step 2 by mapping the current deferred calls clusters to
function-level ownership in `src/backend/mir/aarch64/codegen/calls.cpp`
without editing code.

## Watchouts

- This route is analysis-only; do not edit implementation, test, or build
  metadata files.
- Do not directly edit `src/backend/mir/aarch64/codegen/calls.cpp`.
- Do not reopen completed work from ideas 93, 94, 95, 114, 116, or 117 without
  new evidence.
- Reject monolithic `calls.cpp` shrink proposals or line-count-only progress.

## Proof

Docs/analysis-only packet. No build or test proof was required because only
`todo.md` changed; no `test_after.log` was generated for this packet.
