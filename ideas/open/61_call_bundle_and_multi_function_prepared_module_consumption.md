# Call-Bundle And Multi-Function Prepared-Module Consumption

Status: Open
Created: 2026-04-20
Last-Updated: 2026-04-22
Parent Idea: [57_x86_backend_c_testsuite_capability_families.md](/workspaces/c4c/ideas/open/57_x86_backend_c_testsuite_capability_families.md)

## Intent

Teach the x86 prepared emitter to consume cross-function module structure and
prepared call bundles as normal backend behavior instead of rejecting anything
beyond a single-function minimal route.

## Owned Failure Families

This idea owns these current diagnostic families:

- `error: x86 backend emitter only supports a single-function prepared module ...`
  and nearby bounded multi-function prepared-module restrictions from the same
  canonical prepared-module handoff family
- `error: x86 backend emitter requires the authoritative prepared call-bundle ...`

Current sizes from the 2026-04-20 backend run plus later same-family
re-homing from upstream leaf work:

- prepared-module restriction family: 20 failures
- prepared call-bundle requirement: 5 failures

## Current Known Failed Cases It Owns

Prepared-module restriction cases:

- `c_testsuite_x86_backend_src_00077_c`
- `c_testsuite_x86_backend_src_00078_c`
- `c_testsuite_x86_backend_src_00083_c`
- `c_testsuite_x86_backend_src_00084_c`
- `c_testsuite_x86_backend_src_00087_c`
- `c_testsuite_x86_backend_src_00100_c`
- `c_testsuite_x86_backend_src_00140_c`
- `c_testsuite_x86_backend_src_00162_c`
- `c_testsuite_x86_backend_src_00168_c`
- `c_testsuite_x86_backend_src_00170_c`
- `c_testsuite_x86_backend_src_00189_c`
- `c_testsuite_x86_backend_src_00190_c`
- `c_testsuite_x86_backend_src_00196_c`
- `c_testsuite_x86_backend_src_00197_c`
- `c_testsuite_x86_backend_src_00198_c`
- `c_testsuite_x86_backend_src_00199_c`
- `c_testsuite_x86_backend_src_00200_c`
- `c_testsuite_x86_backend_src_00204_c`
- `c_testsuite_x86_backend_src_00210_c`
- `c_testsuite_x86_backend_src_00219_c`

`c_testsuite_x86_backend_src_00040_c` joined this restriction set on
2026-04-20 after graduating out of idea-62 stack/addressing ownership, then
graduated again the same day into idea-63 runtime-correctness ownership once
prepared-module traversal and authoritative call-bundle consumption advanced it
past codegen rejection into a runtime segmentation fault.

Prepared call-bundle failures:

- `c_testsuite_x86_backend_src_00021_c`
- `c_testsuite_x86_backend_src_00116_c`
- `c_testsuite_x86_backend_src_00121_c`
- `c_testsuite_x86_backend_src_00165_c`
- `c_testsuite_x86_backend_src_00177_c`

## Latest Durable Note

As of 2026-04-22, the accepted idea-68 pointer-add/local-slot handoff repair
advanced `c_testsuite_x86_backend_src_00204_c` back out of the authoritative
prepared local-slot family and into this idea's bounded multi-function
prepared-module restriction again. Fresh route proof now shows the focused
`myprintf` lane matching `local-slot-guard-chain`, while the full top-level
case stops later in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` with the bounded
multi-function prepared-module rejection. Durable ownership therefore returns
to idea 61 until `00204.c` either clears that module/call family or graduates
into another downstream leaf.

## Scope Notes

Expected repair themes include:

- multi-function prepared-module traversal
- entry-point and non-entry-point function emission under one module contract
- call-bundle consumption for argument setup, call clobber obligations, and
  result handoff

## Recommended Repair Route

The intended fix direction for this idea is module-level and call-contract
consumption, not new x86 emitter special cases for one main-entry topology or
one call spelling.

The route should be:

1. identify the prepared module-level and call-bundle facts already published
   for the owned cases
2. build normalized module traversal and call-lane plans from those prepared
   facts
3. make x86 emit from those normalized plans
4. reject any reopening of local ABI setup/result fallback once authoritative
   prepared call-bundle ownership is present

The default assumption for this idea should be:

- if x86 needs to special-case one multi-function module shape, the module
  contract is still too weak or is not being consumed generally
- if x86 needs to re-derive call argument/result ABI setup locally when
  prepared BeforeCall/AfterCall bundles already exist, the call contract is not
  being consumed correctly
- if the current contract is not expressive enough, extend the shared prepared
  contract first instead of adding one more bounded multi-function or call lane

## Current Contract Surface To Consume

This idea should primarily consume the existing prepared surfaces for:

- `PreparedValueLocations`
- `PreparedMoveBundle` with `BeforeCall` and `AfterCall` phases
- function-level prepared names and per-function prepared value-home records
- the prepared module function inventory and authoritative function selection
  inputs already present in the prepared-module handoff

The most relevant current prepared facts are:

- authoritative argument homes for call operands
- authoritative BeforeCall move bundles for call setup
- authoritative AfterCall move bundles for call result handoff
- authoritative same-module function identity and symbol-call relationships
- authoritative per-function value-home data across the functions inside one
  prepared module

Packets under this idea should begin by asking whether the owned call or
multi-function case can be expressed as one generic prepared-module traversal
plus one generic prepared call-lane plan. Only if the answer is no should the
contract grow.

## Concrete Implementation Direction

The safest route is to factor this around shared prepared plans rather than
adding one more entry-lane or call-lane matcher.

Preferred structure:

- a prepared-module traversal helper that can enumerate and validate the
  supported same-module function set without hard-coding one `main + helper`
  topology
- a call-lane plan builder that consumes prepared argument homes plus
  `BeforeCall` / `AfterCall` move bundles and produces a normalized setup /
  clobber / result-handoff plan
- thin x86 renderers that materialize those plans into asm without reopening
  local ABI inference

The aim is not "support this one bounded multi-defined main-entry lane". The
aim is generic prepared-module and call-bundle consumption over the prepared
contract that already names where values live and how they move around a call.

## Contract-First Extension Rule

When an owned case shows that the current prepared module or call surface is
missing information, the preferred fix is to extend the shared prepared
contract and helpers before touching x86 emitter matching logic.

The intended escalation order is:

1. reuse an existing prepared module or call helper
2. add a new shared helper over existing stored fields
3. add missing fields to the prepared value/module contract in `prealloc.hpp`
4. update the producing phase to publish those fields
5. update x86 to consume the new contract

The reverse order is out of bounds for this idea.

## Candidate Missing Interfaces And Where They Belong

If execution reveals a real contract gap, missing interfaces should be added at
the layer that owns the semantic fact.

Preferred additions:

- call argument/result ownership or bundle classification:
  add them to `PreparedMoveBundle` / `PreparedValueLocations`
- same-module symbol-call or function-identity classification:
  add them to target-independent prepared module or name/relationship records,
  not to x86-only emit state
- cross-function result-home or call-result handoff meaning:
  add them to prepared value-home or call-bundle records

For this idea's failure families, the most likely missing facts are:

- a normalized call-lane classification that distinguishes direct same-module,
  extern, and other supported prepared call routes without requiring emitter
  topology matching
- a normalized module traversal classification that tells x86 which prepared
  functions participate in the supported module route and why
- explicit prepared result-handoff metadata when the existing `AfterCall`
  bundle is not enough for generic result consumption

If any of those are missing in practice, they should be published as durable
prepared contract facts instead of being inferred ad hoc inside the x86
emitter.

## Struct-Level Guidance

The following guidance is intentional:

- do not add x86-specific module-shape fields when the missing meaning is
  target-independent function relationship or call-bundle ownership
- prefer fields that describe value ownership, move obligations, and supported
  function relationships over fields that describe one special entry topology
- if a new field cannot be explained without naming one bounded multi-function
  testcase shape, it is probably an overfit field and should be redesigned

Examples of acceptable struct growth:

- a target-independent classification for supported prepared call-lane kinds
- a target-independent function-relationship record for same-module symbol-call
  consumption
- a target-independent result-handoff classification around calls

Examples of suspicious struct growth:

- a flag that means "main-with-one-helper fast path"
- a field whose only purpose is to let x86 skip prepared BeforeCall/AfterCall
  consumption
- a field that mirrors one exact call topology without adding canonical module
  or move meaning

## Boundaries

This idea does not own:

- upstream semantic-lowering failures before module emission
- CFG handoff misses for short-circuit or guard-chain families
- generic scalar expression selection unrelated to calls or cross-function
  layout
- runtime correctness bugs after the call path already executes

## Anti-Overfit Constraints

This idea should be treated as failed if progress mainly comes from turning the
owned call and multi-function cases into a longer list of bounded entry-lane or
call-lane exceptions.

The following are specifically out of bounds:

- adding one more x86-side special case for `main + helper` style modules
  without improving generic prepared-module traversal
- reopening local call-argument ABI fallback when the authoritative prepared
  `BeforeCall` bundle is missing or inconvenient
- reopening local call-result ABI fallback when the authoritative prepared
  `AfterCall` bundle is missing or inconvenient
- proving one bounded same-module symbol-call case while nearby owned
  multi-function cases remain unexplored

Acceptable progress should instead show that x86 follows authoritative
prepared call-bundle ownership and module relationships across multiple nearby
owned cases.

## Dependencies

This idea depends on enough semantic lowering and value-location/addressing
facts being present for function-level handoff to be authoritative.

## Proof Expectations

Proof for packets under this idea should include both:

- the directly owned failing backend c-testsuite cases being targeted
- the existing backend handoff-boundary coverage for prepared BeforeCall /
  AfterCall consumption and bounded multi-defined call routes

Boundary tests alone are insufficient, but owned failed cases alone are also
insufficient if they only pass through a new bounded entry-lane matcher.

## Completion Signal

This idea is complete when the backend no longer rejects the owned cases due to
single-function-only or missing prepared call-bundle restrictions.
