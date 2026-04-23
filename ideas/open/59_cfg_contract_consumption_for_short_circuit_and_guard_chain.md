# CFG Contract Consumption For Short-Circuit And Guard-Chain

Status: Open
Created: 2026-04-20
Last-Updated: 2026-04-20
Parent Idea: [57_x86_backend_c_testsuite_capability_families.md](/workspaces/c4c/ideas/open/57_x86_backend_c_testsuite_capability_families.md)

## Intent

Make the x86 prepared emitter consume authoritative control-flow contracts for
short-circuit and guard-chain families as real backend capability, not only as
boundary-test coverage.

The current codebase has prepared handoff skeletons, but backend c-testsuite
still shows explicit misses in these CFG families.

## Owned Failure Families

This idea owns these current diagnostic families:

- `error: x86 backend emitter requires the authoritative prepared ...`
  where the missing contract is the short-circuit handoff
- `error: x86 backend emitter requires the authoritative prepared guard-chain ...`

Current sizes from the 2026-04-20 backend run:

- short-circuit handoff: 6 failures
- guard-chain handoff: 4 failures

## Current Known Failed Cases It Owns

Short-circuit handoff failures:

- `c_testsuite_x86_backend_src_00033_c`
- `c_testsuite_x86_backend_src_00042_c`
- `c_testsuite_x86_backend_src_00046_c`
- `c_testsuite_x86_backend_src_00109_c`
- `c_testsuite_x86_backend_src_00183_c`
- `c_testsuite_x86_backend_src_00212_c`

Guard-chain handoff failures:

- `c_testsuite_x86_backend_src_00038_c`
- `c_testsuite_x86_backend_src_00057_c`
- `c_testsuite_x86_backend_src_00092_c`
- `c_testsuite_x86_backend_src_00093_c`
- `c_testsuite_x86_backend_src_00088_c`

## Scope Notes

This leaf owns the bridge from prepared CFG/control-flow facts to actual x86
emission for these families.

Expected repair themes include:

- materializing prepared short-circuit joins without falling back to local
  matcher reconstruction
- consuming prepared guard-chain structure generically
- validating nearby same-family cases so repair does not stop at the named
  tests alone

## Recommended Repair Route

The intended fix direction for this idea is upstream-contract consumption, not
new x86 emitter pattern growth.

The route should be:

1. identify the prepared control-flow facts already published for each owned
   family
2. make one shared x86-side consumer layer read those facts directly
3. have the concrete emit path render from that shared prepared plan
4. reject any reopening of raw-branch, raw-compare, or local topology recovery
   once authoritative prepared ownership is present

In practice this means:

- for guard-chain cases, consume authoritative prepared branch-condition and
  target-label ownership instead of re-deriving branch structure from the raw
  BIR compare carrier
- for short-circuit cases, consume authoritative prepared continuation labels,
  join-transfer ownership, and join-carrier meaning instead of reconstructing
  rhs passthrough or join topology from local block shape
- keep the ownership boundary explicit: prepare/legalize publishes the control-
  flow contract, x86 consumes it, and neither side should rely on the other to
  silently recreate missing meaning

The default assumption for this idea should be:

- if x86 still needs to infer CFG meaning from raw block shape, the contract is
  still incomplete
- if the contract is incomplete, fix `prepare/legalize` first instead of adding
  one more emitter-local pattern

## Current Contract Surface To Consume

The current prepared CFG surface already includes:

- `PreparedControlFlowFunction.blocks`
- `PreparedControlFlowFunction.branch_conditions`
- `PreparedControlFlowFunction.join_transfers`
- `PreparedControlFlowFunction.parallel_copy_bundles`

The most relevant existing records for this idea are:

- `PreparedBranchCondition`
  This carries the authoritative branch predicate, compare type, operands, and
  resolved true/false labels for a branch-owning block.
- `PreparedControlFlowBlock`
  This carries authoritative terminator kind and branch targets for each block.
- `PreparedJoinTransfer`
  This carries authoritative join ownership, incoming edge transfers, and the
  branch-owned mapping from a source branch to true/false incoming lanes.
- `PreparedParallelCopyBundle`
  This carries edge-specific copy obligations once join ownership is known.

Packets under this idea should start by asking whether these existing surfaces
already contain enough information to build one generic short-circuit or
guard-chain render plan. Only if the answer is no should the contract grow.

## Contract-First Extension Rule

When a missing short-circuit or guard-chain case reveals that the current CFG
contract is not expressive enough, the preferred fix is to extend
`prepare/legalize` publication and shared prepared-CFG helpers before touching
the x86 emitter.

The intended escalation order is:

1. reuse an existing prepared CFG helper
2. add a new shared prepared CFG helper over existing stored fields
3. add missing fields to the prepared CFG structs in `prealloc.hpp`
4. update `legalize.cpp` to publish those fields
5. update x86 to consume the new contract

The reverse order is out of bounds for this idea.

## Candidate Missing Interfaces And Where They Belong

If execution finds a real information gap, the missing interface should be
added at the contract layer that owns that meaning.

Preferred additions:

- branch-owned condition identity or continuation identity:
  add it to `PreparedBranchCondition`
- branch target or successor-shape facts for a block:
  add it to `PreparedControlFlowBlock`
- join ownership, incoming-lane classification, or continuation-to-join mapping:
  add it to `PreparedJoinTransfer`
- edge copy sequencing obligations that appear only after join ownership is
  known:
  add them to `PreparedParallelCopyBundle`

For this idea's failure families, the most likely missing facts are:

- an explicit classification for short-circuit entry vs rhs continuation vs
  join-owned branch condition
- an explicit mapping from a short-circuit branch condition to the continuation
  labels that x86 should follow
- an explicit classification of join incoming lanes for short-circuit truth /
  false-short-circuit ownership, when the current join-transfer representation
  is not enough for generic consumption
- an explicit guard-chain branch-plan record when a generic renderer still has
  to reopen raw compare-carrier inspection

If any of those are missing in practice, they should be published from
`prepare/legalize` as durable prepared CFG facts instead of being inferred
inside `src/backend/mir/x86/codegen/`.

## Struct-Level Guidance

The following guidance is intentional and should shape implementation:

- do not add x86-specific fields to the prepared CFG structs when the missing
  meaning is target-independent control-flow ownership
- prefer adding normalized semantic ownership fields over storing copies of raw
  BIR syntax
- prefer one field that names the canonical ownership relation over several
  booleans that only describe the current testcase shape
- if a new field cannot be explained without naming a specific testcase, it is
  probably an overfit field and should be redesigned

Examples of acceptable struct growth:

- a target-independent enum or label bundle describing continuation ownership
- a target-independent join incoming classification for short-circuit materialization
- a target-independent branch-plan classification for guard-chain style control
  flow

Examples of suspicious struct growth:

- a flag that means \"this is testcase 00033-like\"
- a field whose only purpose is to let x86 skip one local topology check
- a field that mirrors a raw BIR compare or branch shape already available
  elsewhere, without adding canonical ownership meaning

## Concrete Implementation Direction

The safest route is to factor this around prepared-plan helpers rather than
case-specific emit branches.

Preferred structure:

- a guard-chain plan builder that takes authoritative prepared branch metadata
  plus any required prepared addressing/value-home references and returns a
  normalized branch plan for emission
- a short-circuit plan builder that takes authoritative prepared continuation
  labels and join-transfer metadata and returns a normalized continuation/join
  plan for emission
- thin renderers in x86 codegen that only lower those normalized plans to asm

If those plan builders cannot be written cleanly from the current contract
surface, that is evidence to enrich `PreparedBranchCondition`,
`PreparedControlFlowBlock`, `PreparedJoinTransfer`, or
`PreparedParallelCopyBundle` first, not evidence to grow a bespoke emitter
matcher.

This idea should prefer extending shared prepared-plan helpers that multiple
same-family cases can use over adding one more emitter-local `if` chain for a
named testcase.

## Anti-Overfit Constraints

This idea should be treated as failed if progress comes mainly from making the
owned named cases pass through new x86 pattern matching while the ownership
boundary stays wrong.

The following are specifically out of bounds:

- adding a new emitter-side matcher for one short-circuit CFG shape instead of
  consuming authoritative prepared continuation metadata
- adding a new emitter-side matcher for one guard-chain compare shape instead
  of consuming authoritative prepared branch ownership
- reopening raw-branch or raw-compare fallback when authoritative prepared
  metadata is present but inconvenient
- proving only one named testcase without checking nearby cases from the same
  family

Acceptable progress should instead show that x86 follows prepared ownership
over drifted raw carriers and rejects local fallback reopening when the
authoritative contract is missing or mutated.

## Proof Expectations

Proof for packets under this idea should include both:

- the directly owned failing backend c-testsuite cases being targeted
- the existing handoff-boundary coverage that asserts x86 consumes prepared
  control-flow ownership instead of falling back to raw recovery

At minimum, execution packets should use the narrow backend subset plus the
owned failed cases they claim to advance, and should not rely on boundary tests
alone as proof of completion.

## Boundaries

This idea does not own:

- upstream semantic-lowering failures that never reach prepared x86
- call-bundle or multi-function prepared-module support
- broad scalar expression selection outside these CFG families
- runtime correctness bugs after a case already runs

## Dependencies

This idea depends on enough upstream lowering existing for the owned cases to
reach prepared emission.

## Completion Signal

This idea is complete when the owned short-circuit and guard-chain c-testsuite
families no longer fail due to missing authoritative prepared CFG handoff.

## Latest Durable Note

As of 2026-04-23, the last accepted execution packet on this idea repaired the
single-function short-circuit join-value lane for `00109.c` and `00183.c` by
separating immediate edge-carried materialization from join-defined incoming
select materialization and by publishing local `i32` loads into authoritative
register homes before cross-block use. The focused proof
`ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_x86_backend_src_00109_c|c_testsuite_x86_backend_src_00183_c)$'`
passed, and the broader `^c_testsuite_x86_backend_` rerun still showed the
next likely idea-59 packet in the wider duplicate-function / duplicate-label
family led by `00033.c`. Execution is parked here because the user explicitly
prioritized a full-`src/backend/mir/x86/` Phoenix rebuild initiative.
