# Scalar Expression And Terminator Selection For X86 Backend

Status: Open
Created: 2026-04-20
Last-Updated: 2026-04-20
Parent Idea: [57_x86_backend_c_testsuite_capability_families.md](/workspaces/c4c/ideas/open/57_x86_backend_c_testsuite_capability_families.md)

## Intent

Replace the current minimal matcher-shaped x86 scalar emitter with generic
selection over prepared contracts for ordinary scalar expressions and
terminators.

This idea exists because a large share of current backend failures is not
"missing handoff field" but "x86 emitter still only supports a tiny special
case".

## Owned Failure Families

This idea owns these current diagnostic families:

- `error: x86 backend emitter only supports a minimal single-block i32 return ...`
- `error: x86 backend emitter only supports direct immediate i32 returns, ...`

Current sizes from the 2026-04-20 backend run:

- minimal single-block i32 return restriction: 34 failures
- direct-immediate-only scalar return restriction: 30 failures

## Current Known Failed Cases It Owns

Minimal single-block terminator / branch restriction examples:

- `c_testsuite_x86_backend_src_00007_c`
- `c_testsuite_x86_backend_src_00008_c`
- `c_testsuite_x86_backend_src_00034_c`
- `c_testsuite_x86_backend_src_00035_c`
- `c_testsuite_x86_backend_src_00036_c`
- `c_testsuite_x86_backend_src_00041_c`
- `c_testsuite_x86_backend_src_00058_c`
- `c_testsuite_x86_backend_src_00066_c`
- `c_testsuite_x86_backend_src_00081_c`
- `c_testsuite_x86_backend_src_00082_c`
- `c_testsuite_x86_backend_src_00101_c`
- `c_testsuite_x86_backend_src_00102_c`

Direct-immediate-only scalar return restriction examples:

- `c_testsuite_x86_backend_src_00019_c`
- `c_testsuite_x86_backend_src_00023_c`
- `c_testsuite_x86_backend_src_00024_c`
- `c_testsuite_x86_backend_src_00025_c`
- `c_testsuite_x86_backend_src_00026_c`
- `c_testsuite_x86_backend_src_00056_c`
- `c_testsuite_x86_backend_src_00062_c`
- `c_testsuite_x86_backend_src_00067_c`
- `c_testsuite_x86_backend_src_00068_c`
- `c_testsuite_x86_backend_src_00069_c`
- `c_testsuite_x86_backend_src_00070_c`
- `c_testsuite_x86_backend_src_00107_c`

This idea also owns any other current backend failures with those same
diagnostic families.

## Scope Notes

Expected repair themes include:

- generic scalar value selection instead of direct-immediate-only returns
- ordinary compare / branch / return terminator emission over prepared facts
- removing single-block assumptions where prepared CFG/value data is already
  authoritative

## Recommended Repair Route

The intended fix direction for this idea is to make x86 scalar emission consume
prepared value-home, move-bundle, and CFG facts as a generic lowering surface
instead of growing one more bounded return/branch lane per testcase family.

The route should be:

1. identify the prepared value and control-flow facts already available for the
   owned scalar cases
2. build normalized scalar selection plans from those prepared facts
3. render asm from those normalized plans
4. remove or bypass emitter logic that only recognizes one narrow expression or
   one narrow terminator topology

The default assumption for this idea should be:

- if x86 can only emit a scalar case by recognizing one expression spelling or
  one exact block shape, the prepared contract is not being consumed generally
- if generic scalar selection cannot be written cleanly from the current
  prepared value/CFG surface, fix the shared prepared-contract layer first
  instead of adding one more emitter-local scalar lane

## Current Contract Surface To Consume

This idea should primarily consume the existing prepared surfaces for:

- `PreparedValueLocations`
- `PreparedMoveBundle`
- `PreparedControlFlowFunction.blocks`
- `PreparedControlFlowFunction.branch_conditions`
- `PreparedJoinTransfer` and related join helpers when scalar returns cross a
  compare-join or materialized join

The most relevant current prepared facts are:

- authoritative value homes for params, temporaries, and stack-backed values
- authoritative move bundles for instruction-local and return-ABI moves
- authoritative branch-condition and target-label ownership for scalar branch
  routes
- authoritative return-bundle and compare-join ownership for scalar leaf returns

Packets under this idea should begin by asking whether the needed scalar route
can be expressed as a normalized prepared value/terminator plan from these
surfaces. Only if the answer is no should the contract grow.

## Concrete Implementation Direction

The safest route is to factor scalar emission around prepared-plan builders
instead of enumerating expression-shaped emit cases.

Preferred structure:

- a scalar value-selection helper that lowers prepared value homes plus simple
  prepared move obligations into a normalized source/destination plan
- a scalar return-selection helper that consumes authoritative prepared return
  homes and return bundles instead of reopening local return fallback
- a scalar branch/terminator helper that consumes prepared branch metadata and
  produces a normalized terminator render plan
- thin x86 renderers that only materialize those plans into asm

The aim is not "support add-immediate, then xor-immediate, then one more
commuted case". The aim is a generic scalar lane that selects over prepared
value shape, home, and move obligations.

## Contract-First Extension Rule

When an owned scalar case shows that the current prepared value/CFG surface is
missing information, the preferred fix is to extend the shared prepared
contract and its helpers before touching x86 emitter matching logic.

The intended escalation order is:

1. reuse an existing prepared value/CFG helper
2. add a new shared helper over existing stored fields
3. add missing fields to the prepared value or CFG structs in `prealloc.hpp`
4. update the producing phase to publish those fields
5. update x86 to consume the new contract

The reverse order is out of bounds for this idea.

## Candidate Missing Interfaces And Where They Belong

If execution reveals a real contract gap, missing interfaces should be added at
the layer that owns the semantic fact.

Preferred additions:

- value-home classification or rematerialization facts:
  add them to `PreparedValueHome` / `PreparedValueLocations`
- move sequencing or ABI handoff obligations:
  add them to `PreparedMoveBundle`
- scalar branch or terminator ownership:
  add them to `PreparedBranchCondition` or `PreparedControlFlowBlock`
- compare-join or join-carried scalar return ownership:
  add them to `PreparedJoinTransfer` or related prepared join helper structs

For this idea's failure families, the most likely missing facts are:

- a normalized scalar return plan that distinguishes immediate, param-backed,
  stack-backed, and prepared-move-bundle-backed returns without x86 inferring
  from expression spelling
- a normalized scalar terminator plan that distinguishes straight return,
  prepared branch, and compare-join return routes without reopening local
  topology recovery
- explicit prepared metadata for scalar rematerialization or move-bundle use at
  return boundaries when generic emission still has to fall back to local
  pattern inspection

If any of those are missing in practice, they should be published as durable
prepared value/CFG facts instead of being inferred ad hoc inside the x86
emitter.

## Struct-Level Guidance

The following guidance is intentional:

- do not add x86-specific scalar-expression fields when the missing meaning is
  target-independent value ownership or terminator ownership
- prefer normalized semantic classifications over flags that describe one
  expression spelling
- prefer fields that explain where a value lives and how it must move, not
  fields that say which hard-coded emitter lane should run
- if a new field cannot be explained without naming a specific arithmetic or
  return testcase shape, it is probably an overfit field and should be
  redesigned

Examples of acceptable struct growth:

- a target-independent classification for return-home sourcing
- a target-independent rematerialization classification for scalar values
- a target-independent scalar terminator plan classification

Examples of suspicious struct growth:

- a flag that means "add-immediate return fast path"
- a field that only exists so x86 can accept one commuted arithmetic spelling
- a field that mirrors a raw expression tree without adding canonical ownership
  or move meaning

## Boundaries

This idea does not own:

- upstream semantic-lowering failures before x86 prepared emission
- missing short-circuit or guard-chain handoff contracts
- call-bundle support
- multi-function prepared-module support
- runtime correctness bugs after successful execution starts

## Anti-Overfit Constraints

This idea should be treated as failed if progress mainly comes from turning the
current diagnostic families into a longer list of emitter-side special cases.

The following are specifically out of bounds:

- adding one more `*_if_supported` lane for a named arithmetic shape without
  improving the generic scalar selection path
- proving one expression spelling such as add-immediate while nearby same-family
  arithmetic or stack-backed forms still fail unexamined
- reopening local return fallback or local CFG recovery when authoritative
  prepared homes, move bundles, or branch metadata are already available
- claiming completion because one new bounded return lane passes while the
  minimal single-block / direct-immediate restrictions still define the route

Acceptable progress should instead show that x86 follows prepared value homes,
move bundles, and branch/return ownership across multiple nearby scalar cases.

## Dependencies

This idea should build on upstream semantic coverage plus the relevant prepared
CFG and value-home facts.

## Proof Expectations

Proof for packets under this idea should include both:

- the directly owned failing backend c-testsuite cases being targeted
- the existing backend handoff-boundary coverage for scalar value-home,
  move-bundle, and return/branch ownership

Boundary tests alone are insufficient, but owned failed cases alone are also
insufficient if they are only passing through a newly added emitter matcher.

## Completion Signal

This idea is complete when the current scalar-expression and minimal-terminator
diagnostic families are no longer the reason those owned cases fail.
