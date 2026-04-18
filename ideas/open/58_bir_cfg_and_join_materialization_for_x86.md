# Shared CFG, Branch, Join, And Loop Materialization Before X86 Emission

Status: Open
Created: 2026-04-18
Last-Updated: 2026-04-18

## Intent

This idea defines the shared control-flow contract that must exist after
semantic BIR lowering and before target-specific scalar instruction selection.

The immediate symptom is in the prepared x86 path:
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` still reconstructs
control-flow meaning from whole-function shape. That file currently recognizes
families such as:

- compare-driven branch shapes
- guard-chain shapes
- short-circuit boolean shapes
- select-join shapes
- countdown-loop shapes
- narrow join-materialization shapes

The core problem is not that x86 lacks one more matcher. The problem is that
the shared pipeline still does not hand the target a strong enough statement of
what branch conditions mean, how former phi inputs move across edges, what a
join block is allowed to assume, and how loop-carried values survive phi
removal.

This idea is the durable source intent for fixing that seam.

It is intentionally upstream of x86 instruction selection.

It is intentionally upstream of prepared value-location consumption.

It is intentionally broader than one failing test or one backend lane.

It exists so later plans cannot quietly reintroduce x86-local CFG recovery and
call that progress.

## Executive Summary

Today the current public backend route is roughly:

1. lower LIR into semantic BIR
2. run legalization and prealloc preparation
3. hand a `PreparedBirModule` to x86 emission
4. let x86 inspect `Function.blocks` and `TerminatorKind`
5. if x86 recognizes a bounded family shape, emit asm
6. otherwise fail or fall off the supported surface

That route is too weak because the handoff between step 2 and step 3 still
forces the target to answer semantic questions that should already have been
settled by shared code.

The missing settled questions are:

- is a compare feeding a branch still an ordinary SSA-like value, or already a
  control-flow obligation
- after phi removal, where is the authoritative record of predecessor-to-join
  transfers
- what does a join block have the right to assume is available at entry
- how are loop-carried updates represented after phi elimination
- how should short-circuit boolean flow appear in prepared control flow

The reference compiler under
`ref/claudes-c-compiler/src/backend`
does not solve this by doing magical x86-only pattern matching. Its layering is
different:

- shared generation dispatches per instruction and per terminator
- control flow has explicit generation hooks such as branch, conditional
  branch, fused compare-branch, and default branch-based select
- architecture backends provide small target-specific primitives through
  `ArchCodegen`
- x86 codegen is organized by operation families, not whole-function shape

That reference design matters here not because c4c must clone it line by line,
but because it makes a cleaner ownership statement:

- shared code owns when an instruction or terminator kind is emitted
- shared code owns broad control-flow lowering patterns
- the target owns legal target instruction sequences for those already-decided
  operations

Our current prepared x86 route breaks that ownership line by asking x86 to
re-discover join and branch meaning from CFG topology. This idea exists to
restore the shared ownership line.

## Why This Idea Exists

### The Current X86 File Is Recovering Semantics

`src/backend/mir/x86/codegen/prepared_module_emit.cpp`
is not merely selecting x86 instructions for already-decided operations.

It is also deciding:

- whether a branch condition is really a compare-against-zero family
- whether two predecessors are really a materialized boolean join
- whether a chain of blocks really means short-circuit guard semantics
- whether a loop header and latch really mean a countdown update family
- whether local-slot traffic is actually standing in for former phi movement

Those are not target-local instruction questions.

Those are shared control-flow ownership questions.

When a target has to answer them again, the target naturally starts matching
whole shapes instead of lowering ordinary operations.

That is how `try_render_*` style growth happens.

### Legalization Currently Stops Too Early

`src/backend/prealloc/legalize.cpp`
currently makes some important guarantees, but the exposed guarantee line is
still shallow. The relevant visible contract today is roughly:

- `NoPhiNodes`
- `NoTargetFacingI1`

Those are useful cleanup invariants, but they are not enough.

`NoPhiNodes` only says the original phi instruction no longer appears in the
stream. It does not by itself say:

- where the join inputs are recorded
- whether incoming moves are edge-local or block-entry-local
- whether join materialization is already complete
- whether the target should consume explicit edge copies or reconstructed local
  slot flows

`NoTargetFacingI1` only says the target will not receive raw `i1` values in the
same way. It does not by itself say:

- whether compare results are still ordinary values
- whether conditional flow consumes a dedicated prepared condition object
- whether a compare feeding a branch should be fused, materialized, or delayed

This idea exists because phi removal and boolean promotion are not enough if
branch/join/loop meaning is still implicit.

### Prepared Handoff Is Not Yet Strong Enough

The public backend route in `src/backend/backend.cpp` still hands x86 a
prepared semantic BIR form. That by itself is not wrong. A prepared semantic
BIR route can work if the handoff is strict enough.

The problem is that the current handoff is not yet strict enough about control
flow.

X86 is therefore doing both:

- target emission work
- missing shared control-flow interpretation work

That mixing of responsibilities is the real defect this idea addresses.

## What This Idea Owns

This idea owns the shared contract for control-flow meaning after phi
elimination and before target-specific scalar lowering.

More specifically, this idea owns:

- the post-phi representation of predecessor-to-join transfer obligations
- the statement of what a join block may assume at block entry
- the statement of how branch conditions appear in prepared handoff
- the statement of how short-circuit boolean control flow is represented before
  x86 lowering
- the statement of how loop-carried values survive phi elimination
- the shared invariants that `PreparedBirModule` must satisfy for branches,
  joins, and loops
- the boundary between shared control-flow materialization and target-local
  instruction selection
- the elimination of target-local whole-shape CFG recovery as a required route
  for ordinary branch/join/loop support

This idea does not require one specific final data structure name, but it does
require one crisp ownership answer.

If execution later chooses between:

- explicit edge-copy records
- explicit block-entry join bindings
- prepared condition objects
- branch-ready compare records
- another equivalent shared representation

that choice must still satisfy the ownership boundaries written here.

## What This Idea Does Not Own

This idea does not own stack or location plumbing.

That belongs to
`ideas/open/60_prepared_value_location_consumption.md`.

This idea does not own scalar x86 opcode selection.

That belongs to
`ideas/open/59_generic_scalar_instruction_selection_for_x86.md`.

This idea does not own:

- exact stack-frame sizing
- exact local-slot offsets
- exact assigned registers
- exact assigned spill slots
- exact call argument move planning
- exact return copy planning
- final x86 opcode choice for arithmetic and memory instructions
- x86 peephole optimization
- backend-local tactical matcher expansion

This idea also does not own runbook packetization.

It is a durable source idea, not a step-by-step plan.

## Problem Statement In More Precise Terms

The current system contains a seam mismatch.

### Shared Side Today

Shared lowering and preparation currently know some of the following:

- original CFG
- original phi structure
- compare and select instructions
- local slots introduced during lowering or legalization
- branch terminators and condition values
- prealloc analysis results

### Target Side Today

Prepared x86 emission currently receives:

- the prepared function
- its blocks and terminators
- local slots
- some prepared analysis side data

But the target still lacks a strong enough contract for:

- which block-entry values are owed by each incoming edge
- whether a compare is semantically paired with a branch
- whether a join is semantically a boolean materialization, a select join, a
  loop-carried update, or ordinary merged dataflow
- whether a branch chain is just CFG topology or shared short-circuit control
  flow

### Consequence

Because the control-flow contract is under-specified, x86 inspects the whole
shape to infer the missing meaning.

That is why the target ends up deciding semantic questions such as:

- "this cond branch plus successor chain is probably a minimal compare branch"
- "this block triple is probably a materialized compare join"
- "this load/store/branch sequence is probably a guard chain"
- "this body/cond/backedge arrangement is probably a countdown loop"

That route is fragile even when it works.

It is fragile because semantically equivalent CFGs can differ in:

- empty block insertion
- predecessor ordering
- block naming
- direct versus branch-through entry
- whether a join uses a slot, select, or rematerialized value
- whether boolean materialization is explicit or implicit

If x86 support depends on one lucky shape, the backend is not actually
supporting the semantic feature. It is supporting one layout of that feature.

## Reference Backend Comparison

This section is intentionally more detailed because the reference backend is
not just a random inspiration source. It shows a concrete layered answer to
the same class of ownership problems.

### Shared Generation In The Reference Backend

In
`ref/claudes-c-compiler/src/backend/generation.rs`
the generation logic walks a function block by block and instruction by
instruction.

It does not ask x86 to interpret the whole function shape first.

Instead it performs shared dispatch:

- `Instruction::Load` goes to `generate_load`
- `Instruction::BinOp` goes to `emit_binop`
- `Instruction::Cmp` goes to `emit_cmp`
- `Instruction::Select` goes to `emit_select`
- `Instruction::Call` goes to `emit_call`
- terminators go through `generate_terminator`

That matters because the decision "what kind of thing is being lowered right
now" is owned by shared generation, not re-derived by x86 from block topology.

### ArchCodegen In The Reference Backend

In
`ref/claudes-c-compiler/src/backend/traits.rs`
the `ArchCodegen` trait provides:

- small target primitives
- shared default implementations
- explicit control-flow helpers
- explicit select lowering defaults
- explicit fused compare-branch hooks

Relevant examples include:

- `emit_branch_nonzero`
- `emit_branch`
- `emit_branch_to_block`
- `emit_select`
- `emit_cond_branch_blocks`
- `emit_fused_cmp_branch`

The important design lesson is not "copy these exact method names."

The important design lesson is:

- control-flow lowering entry points are explicit
- compare-to-branch fusion is a shared concept, not a whole-function matcher
- select lowering has a default shared route, with optional target-specific
  acceleration
- the target is handed a decided operation family rather than forced to infer
  it from a CFG sketch

### Reference Select Lowering

The default `emit_select` implementation in the reference backend is branch
based.

That is very important for this idea.

It means the shared framework is comfortable saying:

- a select is semantically a select
- the target may lower it with branch-based control flow if needed
- another target may override it with conditional move

In other words, the shared layer owns the semantic category.

The target only owns the legal sequence that implements that category.

Our current prepared x86 route often does the opposite:

- it sees a branch chain and tries to recover that it means select-like or
  guard-like behavior

This idea aims to move c4c closer to the reference ownership model on the
control-flow seam, even if c4c does not adopt the exact same IR or API.

### Reference Fused Compare-Branch

The reference backend has an explicit notion of fused compare-and-branch.

This is also critical.

It means there is a shared answer to the question:

- when a compare only feeds a conditional branch, should that pairing be
  exposed to the target as a direct branch opportunity

Again, x86 is not discovering "ah, this block pattern looks like compare plus
branch."

Instead shared generation knows it is looking at a compare plus branch case and
offers the target an explicit lowering hook.

This is exactly the kind of contract c4c is currently missing on the prepared
route.

### Reference Backend Uses Explicit Block And Terminator Ownership

The reference codegen path emits blocks and terminators deliberately.

It does not need an x86-only "guard chain recognizer" to know that a block
ends in conditional flow.

That is because the terminator meaning is already explicit.

C4c already has explicit `Branch` and `CondBranch` terminators in BIR, but the
problem is one level deeper:

- successor-edge obligations and join-entry meaning are still underspecified

This idea fills that deeper gap.

### What We Should Import From The Reference Design

This idea does not say c4c must port the reference backend architecture
verbatim.

It does say c4c should import these ownership principles:

- shared code decides the operation family earlier
- control flow gets explicit lowering hooks
- compare-to-branch is a known shared relationship
- select and branch are semantic categories, not x86 shape guesses
- target modules lower explicit obligations rather than reconstruct them from
  whole CFG shape

### What We Should Not Import Blindly

This idea does not require:

- converting c4c BIR into the reference IR
- copying the entire `ArchCodegen` trait
- copying the accumulator model
- copying the entire reference register cache strategy
- copying all reference backend modules

The durable lesson is ownership structure, not textual imitation.

## Current Repo Evidence For The Gap

### Evidence In `backend.cpp`

`src/backend/backend.cpp`
shows that the public backend path still routes through prepared semantic BIR.

That is not inherently a bug.

The bug is that the prepared handoff still leaves control-flow meaning
under-specified for targets.

### Evidence In `legalize.cpp`

`src/backend/prealloc/legalize.cpp`
contains machinery around phi removal and type promotion, but the durable
visible contract is still too thin.

There is no single, crisp, externally obvious statement that says:

- how predecessor-to-join transfers are represented after phi elimination
- what a join block receives at entry
- what relationship between compare and branch survives into prepared handoff
- what exact shape loop-carried transfers take after phi elimination

### Evidence In `prepared_module_emit.cpp`

`src/backend/mir/x86/codegen/prepared_module_emit.cpp`
currently includes many narrow families that prove the control-flow contract is
not settled early enough.

The file contains logic that directly inspects:

- terminator kinds
- predecessor/successor chain shape
- local-slot load/store traffic
- compare and select placement
- optional empty intermediate blocks
- backedge arrangement

That is already enough evidence that x86 is compensating for a shared
representation gap.

### Evidence In Tests

The relevant tests already hint at the seam:

- `tests/backend/backend_lir_to_bir_notes_test.cpp`
- `tests/backend/backend_x86_handoff_boundary_test.cpp`

The first is closer to shared lowering observations.

The second is closer to what the x86 handoff expects.

This idea exists so the boundary between those two can become explicit instead
of accidental.

## Terminology

The words below are used precisely in this idea.

### Control-Flow Materialization

The act of turning semantic branch/join relationships into a prepared form with
explicit obligations that a target can consume without rediscovering semantics
from incidental CFG shape.

### Join Materialization

The representation of values that conceptually meet at a join point after phi
elimination.

This idea does not assume one final mechanism yet.

It only requires that one shared mechanism be authoritative.

### Edge Obligation

A prepared statement that a predecessor edge is responsible for making some
value or state available to a successor join point.

This may later become an explicit move, slot store, condition record, or
another prepared artifact.

### Block-Entry Obligation

A prepared statement that a block begins with certain already-satisfied value
bindings or move effects, regardless of the predecessor shape used to achieve
them.

### Branch Condition Ownership

The phase that is responsible for deciding whether a compare is:

- materialized as a regular value
- consumed directly by branch lowering
- represented as a prepared condition form

### Loop-Carried Value

A value whose next iteration meaning depends on the previous iteration and thus
would historically be associated with a phi at a loop header.

After phi elimination, these values still need an explicit ownership route.

## Detailed Failure Modes

### Failure Mode A: Compare Meaning Is Recoverable But Not Represented

A branch condition currently may appear as a value that originated from:

- compare
- casted compare
- select-derived boolean
- slot-reloaded former phi
- another promoted boolean family

If the prepared handoff does not make the compare-to-branch relationship
explicit, x86 has to do some form of:

- "inspect the defining inst"
- "inspect adjacent blocks"
- "inspect whether a join stores or reloads a bool-like value"

That is exactly the wrong phase to decide it.

The target should be allowed to choose between:

- fused compare + branch
- compare materialization + branch
- branch on already-materialized condition

But the target should not be deciding from raw CFG archaeology whether a branch
was semantically compare-driven in the first place.

### Failure Mode B: Join Entry Meaning Is Split Across Multiple Mechanisms

Today a former phi may be expressed through some mixture of:

- local slot introduction
- predecessor-local stores
- join-block loads
- select materialization
- remaining value naming conventions
- test-only observations

When more than one mechanism participates without one canonical ownership rule,
the target cannot consume join semantics directly.

Instead it reconstructs one of a few familiar special shapes.

That is not acceptable as the long-term route.

### Failure Mode C: Short-Circuit And Guard Chains Depend On Incidental Block Layout

Short-circuit semantics should be ordinary control flow.

A guard chain should not need a dedicated x86 family recognizer.

If the shared handoff clearly states:

- which condition governs which successor edge
- what values are owed when a path reaches a join
- how former phi or select meaning is represented

then short-circuit chains are merely a composition of conditional branches and
join obligations.

Today they are often recovered from bounded block arrangements.

### Failure Mode D: Loop-Carried Updates Remain Shape Driven

Countdown loop recognition is the clearest embarrassing example, but it is not
the only possible one.

Any loop where a header value is updated on the latch side can fall into this
class.

Once phi is removed, the loop still needs a shared representation of:

- incoming initial value
- backedge-updated value
- header-entry binding
- exit-path meaning

If that is not explicit, the backend naturally ends up recognizing loop
families instead of lowering ordinary loop control flow.

### Failure Mode E: Backend Progress Becomes Case Collection

When control-flow ownership is unresolved, each new failure invites a tactical
patch:

- add another matcher
- widen a matcher family
- tolerate one more empty block
- tolerate one more join variant
- tolerate one more local-slot naming convention

That creates the illusion of backend growth while avoiding the real upstream
contract fix.

This idea exists to reject that route.

## Desired End State

The end state required by this idea is not "all control flow is optimal."

The required end state is:

- shared code owns the representation of branch and join semantics after phi
  elimination
- prepared handoff states those semantics explicitly enough that x86 no longer
  needs whole-shape recovery for ordinary branches, joins, or loops
- target emission consumes those prepared semantics
- semantically equivalent CFG layouts do not require separate x86 family
  matchers

In the desired end state:

- x86 may still select different instruction sequences for efficiency
- x86 may still have local fast paths
- x86 may still optimize fused compare-branch

But x86 must not be the first phase that discovers the control-flow meaning.

## Design Principles

### Principle 1: Semantic Family Must Be Decided Before Target Opcode Choice

The target should decide between legal x86 sequences for an already-known
semantic family.

The target should not decide the semantic family by reverse engineering CFG
shape.

### Principle 2: Phi Elimination Must Leave A Positive Contract, Not Just An Absence

`NoPhiNodes` is a negative statement.

This idea requires a positive statement:

- what replaced the phi contract
- where it is recorded
- who consumes it

### Principle 3: Branch Conditions Need A Shared Handoff Form

Whether the final code uses:

- `cmp` + `jcc`
- `test` + `jcc`
- materialized bool + branch
- branch-based select lowering

the target still needs a prepared branch-condition story that is already
semantically classified.

### Principle 4: Join Support Must Generalize Across Shape Variation

Any representation adopted under this idea must survive variations such as:

- empty intermediate blocks
- entry through branch-through trampolines
- swapped true/false predecessor order
- join blocks with one or more non-join instructions
- different local slot naming
- equivalent select versus branch materialization choices

If the contract only works for one exact arrangement, it is not a true shared
join contract.

### Principle 5: Loop-Carried Values Are Join Values With Backedges

Loop-carried support should not be treated as a separate magical species.

A loop header is a join with a backedge.

Any adopted representation should reflect that fact.

### Principle 6: Short-Circuit Control Flow Is Normal Control Flow

A short-circuit `&&` or `||` shape is not a backend exception.

It is a composition of conditional edges and join obligations.

This idea therefore treats short-circuit chains as a proving ground for whether
the shared contract is actually general.

### Principle 7: Prepared Data Must Be Authoritative

If prepared control-flow semantics are added, targets must consume them as the
source of truth.

We should not add explicit prepared semantics and then keep x86 reconstructing
the same thing from raw blocks in parallel.

That would only duplicate ambiguity.

## Concrete Scope Boundaries

The exact scope line matters, because this idea sits between idea 60 and
idea 59.

### This Idea Owns These Questions

1. after phi elimination, what records the fact that predecessor A provides
   value X and predecessor B provides value Y to a join
2. whether those provisions are edge-local records, block-entry bindings, or an
   equivalent shared form
3. whether conditional branches consume explicit prepared conditions or plain
   values with a documented semantic contract
4. whether compare-plus-branch can be represented as one shared control-flow
   form before x86 scalar lowering
5. how loop-header join obligations are represented when one incoming edge is a
   backedge
6. what invariants a target may rely on when lowering join-entry values
7. how short-circuit branch chains are guaranteed to use the same shared
   mechanisms instead of special families

### Idea 60 Owns These Neighboring But Different Questions

1. once a value transfer obligation exists, where physically does the value
   live
2. if an obligation becomes a move, what register or slot is used
3. how does stack layout expose canonical offsets
4. how does addressing provenance get consumed
5. how are prepared move resolutions consumed by the target

### Idea 59 Owns These Later Questions

1. once a branch condition and join contract are explicit, how does x86 lower a
   compare, select, arithmetic op, load, store, or call sequence
2. when a compare can be fused with a branch, which exact x86 sequence is
   chosen
3. how does x86 choose legal operand forms and temporary moves

## Non-Goals

This idea deliberately rejects several tempting expansions.

### Non-Goal: Rewrite All Of BIR

The purpose is not to invent an entirely new IR just to solve one backend seam.

### Non-Goal: Force Early Full Machine IR

It may turn out that c4c eventually benefits from a more explicit machine-like
intermediate form, but this idea does not require that conclusion.

It only requires a strong enough shared control-flow contract.

### Non-Goal: Encode X86 Flags Semantics Directly Into Semantic BIR

The desired contract may expose compare-to-branch relationships more clearly,
but that does not mean semantic BIR should suddenly become x86-flags IR.

The contract must remain target-usable, not target-captured.

### Non-Goal: Treat Every Select As Mandatory Branch Lowering

The reference backend's default branch-based select is useful as a design
example, but c4c is not required to lower every select to branches at the
shared level.

The requirement is that select and join semantics become explicit enough that
targets do not reverse engineer them from CFG shape.

### Non-Goal: Hide The Problem With Test Rewrites

Expectation downgrades or testcase-specific matcher growth are not progress
under this idea.

### Non-Goal: Solve Stack Addressing Here

The contract described here will interact with frame and slot ownership, but
frame and slot design itself belongs to idea 60.

## Control-Flow Seams That Must Be Made Explicit

### Seam 1: Compare Result Versus Branch Condition

A compare can participate in at least three broad routes:

1. compare result is a regular value used by non-branch consumers
2. compare result is only consumed by a conditional branch
3. compare result feeds both branch-like and non-branch-like consumers

Those cases should not be left ambiguous.

At a minimum, the prepared handoff must make it possible to distinguish:

- "branch can consume this directly"
- "this is already a materialized value"
- "shared lowering deliberately preserved a materialized value because later
  consumers require it"

Without that distinction, x86 must inspect local context and infer use intent.

### Seam 2: Edge Transfer Versus Block-Entry Binding

After phi elimination, there are two broad conceptual ownership models:

1. predecessor edges own explicit outgoing transfers
2. join blocks own explicit entry bindings satisfied by incoming edges

Either can work.

The current problem is not that one specific model is absent.

The current problem is that no single model is authoritative.

This idea requires a single authoritative answer.

### Seam 3: Join Value Versus Materialized Slot Traffic

A join value may ultimately travel through a stack slot, but that must not be
the only visible semantic representation.

If the only observable prepared story is:

- predecessor stores to a slot
- join loads from the slot

then the target still has to infer whether that slot traffic is:

- former phi replacement
- ordinary memory traffic
- guard result materialization
- loop-carried update materialization

That is not acceptable as the only contract.

### Seam 4: Loop Header Entry Versus Ordinary Join Entry

A loop header is special only because one incoming edge is a backedge and the
entry is revisited.

The representation should still feel like a general join contract plus repeated
control flow, not a bespoke loop-family encoding.

### Seam 5: Short-Circuit Branch Chains Versus Arbitrary Nested Conditionals

A well-designed shared contract should treat short-circuit chains as ordinary
compositions of:

- conditional terminators
- explicit successor-edge obligations
- explicit join-entry assumptions

If short-circuit still needs its own x86-only family, the contract is still too
weak.

## Candidate Representation Directions

This idea is durable source intent, not a final design selection document, but
it still needs to narrow the solution space.

### Direction A: Explicit Edge Transfer Records

In this direction, prepared data would include explicit records per edge such
as:

- predecessor block id
- successor block id
- transferred value bindings
- maybe condition-class metadata if the edge is condition-controlled

Advantages:

- naturally matches predecessor responsibility
- clean for former phi replacement
- loop backedges fit naturally
- easy to reason about join incoming alternatives

Risks:

- may duplicate information if move resolution also later wants edge-local
  records
- can become awkward if block-entry semantics need to compose with location
  decisions owned by idea 60

### Direction B: Explicit Block-Entry Join Bindings

In this direction, a join block would declare what values are available at
entry and how each predecessor satisfies them.

Advantages:

- centers reasoning on join semantics
- may be easier for consumers that lower block by block
- aligns well with "what may this block assume" framing

Risks:

- can still hide edge-local actions if not designed carefully
- may require more indirection when physical moves are later computed

### Direction C: Prepared Control-Condition Objects

In this direction, conditional terminators would not merely carry a generic
value operand. They would carry or reference a prepared condition description,
for example:

- compare op + operands
- truth polarity
- maybe known canonical normalization

Advantages:

- directly addresses compare-to-branch ambiguity
- can enable branch fusion without x86 archaeology
- matches the reference backend's explicit fused compare-branch concept

Risks:

- must avoid overfitting to x86 flags semantics
- must still interoperate with non-branch compare consumers

### Direction D: Hybrid Contract

A realistic design may combine:

- explicit branch condition descriptions
- explicit edge transfer or join-binding records
- later physical move/location consumption by idea 60

This idea does not forbid a hybrid.

It does require that the hybrid still present a single authoritative ownership
line.

## Preferred Direction At The Durable-Idea Level

The most promising durable direction is a hybrid contract with two layers:

1. explicit prepared control-flow semantics
2. explicit prepared join transfer semantics

The target shape should be:

- terminators know more than "cond is some ordinary value"
- joins know more than "reload from whatever slot traffic happened earlier"

That does not force final opcode decisions early.

It only forces semantic responsibility to settle before target lowering.

More concretely, the durable preference of this idea is:

- branch conditions should have a shared prepared semantic form strong enough to
  express compare-driven branch lowering without x86-local recovery
- join obligations should have a shared prepared semantic form strong enough to
  express former phi and loop-carried transfers without x86-local recovery
- the physical location and move realization of those obligations should be
  consumable by idea 60

## What "Strong Enough" Means

The phrase "strong enough" can become hand-wavy unless it is pinned down.

For this idea, a control-flow contract is strong enough if all of the following
hold:

1. x86 can tell whether a conditional branch is compare-driven without scanning
   the surrounding CFG for a lucky shape
2. x86 can tell what values are available at a join without inferring them from
   local-slot patterns
3. loop-header carried values are not identified by recognizing a countdown
   family
4. short-circuit chains can be lowered by consuming ordinary prepared branch
   and join semantics
5. semantically equivalent block rearrangements do not require distinct backend
   matcher families

## Concrete Invariants This Idea Wants

The exact field names can change, but the invariants below should survive.

### Invariant 1: Every Conditional Terminator Has An Authoritative Semantic Condition Story

A conditional terminator must not force the target to infer whether it is:

- branch on materialized bool
- branch on compare result
- branch on slot-reloaded former phi

The prepared handoff must tell the truth directly enough that no whole-shape
recovery is required.

### Invariant 2: Every Former Phi Has One Authoritative Replacement Story

If a phi was removed, there must be one authoritative prepared answer for how
the incoming alternatives become available at the join or header.

### Invariant 3: Join Entry Assumptions Are Explicit

For any block that semantically merges values, the prepared handoff must define
what is true at block entry.

### Invariant 4: Backedges Reuse The Same Join Contract

A loop backedge must satisfy the same style of transfer contract as any other
incoming edge, even if later optimization recognizes loop structure.

### Invariant 5: Target Consumption Does Not Need Incidental Slot Naming

Prepared control-flow semantics must not depend on a target noticing that some
slot name happens to end in a special suffix or occurs in a certain block
position.

### Invariant 6: No X86-Only Recovery Step Is Required For Ordinary Branching

Fast paths may remain, but the ordinary supported route must no longer depend on
bounded whole-shape recognizers.

## Relationship To Idea 60

This section matters because the boundary with idea 60 is easy to blur.

### Why This Idea Comes Before Full Consumption

Idea 60 is about consuming prepared location and frame data correctly.

But there is no point wiring location consumption perfectly if the semantic
meaning of branch and join flow is still fuzzy.

A target can only consume physical moves and frame placements correctly when it
first knows what semantic transfer it is supposed to realize.

That is why this idea is upstream.

### Why This Idea Cannot Swallow Idea 60

It would be easy to say:

- "the join transfer is just whatever move resolution says"

That would be too vague.

Move resolution belongs to the physical realization layer.

This idea is about declaring what join transfers mean before those physical
details are chosen or consumed.

### Practical Boundary

This idea should be able to answer:

- "value A must reach join B from predecessor P"

without yet answering:

- "does that happen via register move, stack slot, spill copy, or rematerialize
  path"

The second question belongs to idea 60.

## Relationship To Idea 59

This idea also sits directly upstream of idea 59.

### Why Scalar Instruction Selection Must Not Start First

Generic scalar instruction selection needs to know:

- what a branch condition semantically is
- whether a compare can feed branch lowering directly
- whether a select remains a select or has already become branch-shaped control
  flow
- how join-entry values are represented

Without those answers, instruction selection is forced to do one of two bad
things:

- re-derive semantics from shape
- select only for very narrow families

That is exactly the current failure pattern.

### What Idea 59 Should Be Able To Assume After This Idea Lands

Idea 59 should be able to assume:

- conditional branch lowering is consuming an explicit prepared condition
  contract
- join-entry value availability is explicit
- loop-carried updates are not hidden in arbitrary slot traffic
- short-circuit branch structures are expressible via the same shared contract

## Why The Old X86 Matcher Growth Is The Wrong Fix

The repo already demonstrated the failure mode:

- one matcher for a simple branch family
- another matcher for a local-slot guard chain
- another matcher for a compare join
- another matcher for a loop shape
- then widen each matcher to tolerate nearby variants

That growth path is always tempting because it fixes the nearest test fastest.

But it fails engineering quality on three fronts:

### It Is Not Stable Under Equivalent Lowerings

Small shared-lowering changes can invalidate the target matcher without changing
semantics.

### It Confuses Capability With Recognition

The backend claims to support a semantic feature when it really supports one
syntax tree of that feature.

### It Blocks Shared Ownership Repair

As long as each new failure is patched in x86, the upstream contract never gets
completed.

This idea exists to force the opposite response:

- pull the missing ownership decision upward
- make the prepared contract explicit
- let later target code consume it normally

## Examples Of The Ownership Gap

The examples here are schematic on purpose. They are durable illustrations, not
executable specs.

### Example 1: Compare Feeding Branch

Semantic shape:

```text
v0 = load x
v1 = const 0
v2 = eq v0, v1
condbr v2, then, else
```

Current weak contract:

- x86 may see only an `i32` promoted boolean and a `CondBranch`
- x86 may need to inspect the producing inst and surrounding blocks to guess
  this is a direct compare-to-branch case

Desired strong contract:

- shared handoff states that the branch condition is compare-driven
- x86 may choose fused `cmp/test + jcc` or a slower path
- x86 does not discover the semantic family from CFG shape

### Example 2: Former Phi At A Join

Semantic shape:

```text
if cond:
  v_then = 1
else:
  v_else = 2
join:
  v_join = phi(v_then, v_else)
  ret v_join
```

Current weak contract:

- phi may be removed
- predecessor stores and join loads may exist
- target may infer from a specific slot pattern that this is really a join

Desired strong contract:

- join materialization obligations are explicit in prepared data
- target knows what value reaches join from each predecessor
- physical realization is later consumed through idea 60

### Example 3: Short-Circuit Guard

Semantic shape:

```text
if a:
  if b:
    ret 1
ret 0
```

Current weak contract:

- target may recognize a special guard-chain family

Desired strong contract:

- two ordinary conditional edges
- explicit join/exit obligations
- no special "guard chain" family is needed to understand the CFG meaning

### Example 4: Loop-Carried Update

Semantic shape:

```text
i0 = n
loop_header:
  i = phi(entry: i0, backedge: i_next)
  if i == 0: goto exit
loop_body:
  i_next = i - 1
  goto loop_header
exit:
  ret i
```

Current weak contract:

- phi removed
- some slot traffic and backedge branch remain
- target recognizes a countdown loop if the shape is lucky

Desired strong contract:

- header join obligations are explicit
- backedge supplies a loop-carried value through the same authoritative
  mechanism as any other predecessor
- target lowers ordinary compare, branch, and transfer semantics

## Detailed Ownership Questions This Idea Must Resolve

### Question 1: What Is The Authoritative Consumer-Facing Shape Of A Conditional Branch?

Potential answers include:

- `CondBranch` with a plain value operand plus strong rules on what that operand
  may represent
- `CondBranch` with an attached prepared condition descriptor
- a transformed branch record that explicitly names compare op and operands

This idea does not lock the exact syntax, but it requires the answer to be
clear, documented, and consumable without shape recovery.

### Question 2: Are Former Phi Alternatives Represented Per Edge Or Per Join?

Either can be acceptable.

The key is that one must be canonical and visible in prepared handoff.

### Question 3: How Do Join Semantics Compose With Physical Move Resolution?

The answer should be:

- semantic transfer is defined here
- physical move realization is handled by idea 60

The semantic layer must not disappear just because a later layer chooses actual
copy locations.

### Question 4: How Are Multi-Use Compare Results Treated?

A compare may feed both:

- branch
- value consumers

This idea must allow that without forcing all compares into one shape.

Possible shared outcomes:

- compare remains a materialized value
- branch gets an explicit relation to the compare source
- a split representation exists with clear rules

But again, the relationship must be explicit.

### Question 5: How Do Loops Reuse The Same Contract Without Special Cases?

This idea must define loop-carried transfer in a way that avoids inventing a
special countdown-loop category.

### Question 6: What Are The Minimal Handoff Invariants Required Before X86?

This idea must eventually produce a written invariant set that x86 and tests can
rely on.

## Proposed Durable Decomposition Inside This Idea

Although this is not a runbook, the design space inside the idea should still
be broken down into stable themes.

### Theme A: Branch Condition Semantics

Subtopics:

- compare-driven branch representation
- truth polarity normalization
- materialized bool versus direct condition semantics
- select relationship where relevant

### Theme B: Join Input Semantics

Subtopics:

- former phi replacement contract
- per-edge versus per-join authority
- block-entry assumptions
- join value naming and stability

### Theme C: Loop-Carried Join Semantics

Subtopics:

- backedge participation
- header-entry obligations
- loop exit obligations
- invariants that avoid loop-family matchers

### Theme D: Short-Circuit And Guard Composition

Subtopics:

- chained conditional successors
- join-entry convergence
- condition inversion stability
- proving that short-circuit is not a special family

### Theme E: Handoff Boundary Tests

Subtopics:

- BIR notes coverage
- x86 handoff boundary coverage
- tests that assert contract, not accidental asm family shape

## Repository Areas Likely Touched By Future Execution

This idea intentionally names primary areas so the durable source intent is not
detached from the codebase.

Primary likely areas:

- `src/backend/bir/lir_to_bir.cpp`
- `src/backend/bir/lir_to_bir.hpp`
- `src/backend/bir/lir_to_bir_module.cpp`
- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/regalloc.cpp`
- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `tests/backend/backend_lir_to_bir_notes_test.cpp`
- `tests/backend/backend_x86_handoff_boundary_test.cpp`

Possible later touchpoints, depending on final representation:

- BIR printer / observation utilities
- prepared module schema definitions
- target-agnostic handoff tests

This list is informative, not exhaustive.

## Test Philosophy For This Idea

Because this idea is upstream of x86 scalar lowering, its tests should not only
assert final asm snippets.

They should assert the shared contract.

### Good Tests Under This Idea

- tests that show phi elimination leaves explicit replacement semantics
- tests that show compare-driven branch semantics are visible before x86 local
  shape recovery
- tests that show loop-carried join obligations are explicit
- x86 handoff tests that prove the target consumes prepared control-flow
  semantics rather than reconstructing them from raw CFG

### Bad Tests Under This Idea

- tests that merely bless one current `try_render_*` family string
- tests that pass only because a narrow matcher is widened again
- tests that rewrite expectations downward to fit target-local gaps

## Acceptance Criteria

This idea should only be considered satisfied when all of the following are
true:

- [ ] there is one documented authoritative shared contract for branch
      condition semantics before x86 scalar lowering
- [ ] there is one documented authoritative shared contract for former phi and
      join-input semantics before x86 scalar lowering
- [ ] loop-carried values use that same shared join contract rather than a
      backend-local loop-family recognizer
- [ ] short-circuit and guard-style control flow are representable through the
      same shared condition and join contracts
- [ ] `PreparedBirModule` or its prepared successor exposes enough control-flow
      meaning that x86 no longer needs whole-shape recovery for ordinary
      branches, joins, and loops
- [ ] the boundary between semantic transfer ownership and physical move/location
      ownership is explicitly documented and aligned with idea 60
- [ ] the boundary between shared control-flow materialization and x86 scalar
      instruction selection is explicitly documented and aligned with idea 59
- [ ] repo tests prove contract-level behavior rather than only preserving one
      matcher family

## Anti-Acceptance Signals

The following are signs that the idea is not actually satisfied even if some
tests turn green:

- x86 still needs to inspect several adjacent blocks to determine whether a
  branch is compare-driven
- x86 still recognizes local-slot guard or loop families as the ordinary route
- phi removal still leaves join meaning implicit in raw load/store traffic
- loop-carried updates are still supported only for one bounded local-slot
  pattern
- short-circuit lowering still relies on dedicated x86 whole-shape recovery

## Dependencies And Ordering

Within the current open-idea set, this idea should be treated as first-class
upstream work.

### Earlier Dependencies

- none required in the current decomposition

### Parallel Neighbor

- idea 60 can make progress in parallel where it clarifies prepared frame and
  value-location consumption, but any design there must leave room for this
  idea's semantic transfer contract

### Later Dependent

- idea 59 should generally consume the contract produced here and should not be
  treated as the phase that invents it

## Risks

### Risk 1: Over-Specifying A Machine-Oriented Condition Form

If the prepared condition representation becomes too x86-shaped, the shared
contract may become target-captured.

Mitigation:

- represent semantic compare/condition relationships, not x86 flags details

### Risk 2: Under-Specifying The Contract Again

If the final design says only "some move will happen" or "some branch condition
is available," x86 will still need shape recovery.

Mitigation:

- demand explicit consumer-facing invariants

### Risk 3: Smuggling Physical Move Ownership Into This Idea

It is easy to accidentally let the semantic transfer representation collapse
into regalloc or stack-layout details.

Mitigation:

- keep the semantic transfer story distinct from the physical realization story

### Risk 4: Leaving Short-Circuit As An Implicit Exception

A design that handles simple if-joins but punts on short-circuit control flow
is incomplete.

Mitigation:

- treat short-circuit and guard chains as required proving families

### Risk 5: Solving Only The Countdown Loop Case

It is tempting to target the current most embarrassing matcher.

Mitigation:

- require a general loop-header/backedge contract, not one loop recipe

## Open Design Questions

The durable idea should preserve these questions so future execution does not
pretend they never existed.

1. Should prepared conditional semantics be attached directly to
   `CondBranchTerminator`, or referenced indirectly through a separate prepared
   condition table
2. Should join transfer authority live on predecessor edges, successor block
   entries, or a hybrid
3. How should multi-use compares be represented so branch consumers can still
   benefit from direct condition lowering without losing regular value semantics
4. How much of former phi observation should remain visible in BIR notes tests
   after the new contract lands
5. Can the existing prepared move-resolution structures be extended cleanly to
   carry semantic join-transfer intent, or should semantic and physical layers
   stay explicitly separate
6. What minimal invariant set should be published in headers so target code can
   consume the contract without peeking at legalization internals

## Migration Expectations

This section is intentionally high level. It names the expected migration shape
without turning the idea into a runbook.

The likely migration story is:

1. identify and document the explicit prepared control-flow contract
2. make legalize or a nearby shared phase produce it
3. surface it through prepared structures
4. update x86 handoff consumption to rely on it
5. delete or de-authorize ordinary whole-shape matcher families that it makes
   unnecessary
6. only then let later scalar lowering consume the clearer handoff

The important constraint is sequence, not exact packet count:

- contract first
- consumer second
- matcher deletion as consequence, not as unsupported collapse

## What Success Should Feel Like In Code Review

When this idea is being implemented correctly, a reviewer should be able to say:

- "the shared code now owns the branch/join semantics"
- "the target is consuming an explicit prepared condition or join-transfer
  record"
- "this same mechanism handles if-joins, short-circuit chains, and loop headers"
- "x86 no longer needs to infer a countdown or guard family from block shape"

What success should not look like:

- "we renamed the matcher"
- "we added one more tolerated block arrangement"
- "we moved the same pattern recovery into a helper"

## Examples Of Good Future Plan Questions

When a later plan is generated from this idea, the first packets should ask
questions like:

- where should explicit prepared branch-condition semantics live
- where should explicit join-transfer semantics live
- what is the narrowest vertical slice that proves if-join and loop-header use
  the same contract
- which existing x86 matcher becomes unnecessary first once the new contract is
  consumed

Those are good plan questions because they derive from the contract seam.

These would be bad plan questions:

- which `try_render_*` should be widened next
- which testcase shape should be recognized first without shared contract work

## Validation Intent

Narrow proof for work under this idea should focus on the contract seam:

- `cmake --build --preset default`
- `ctest --test-dir build -R '^(backend_lir_to_bir_notes|backend_x86_handoff_boundary(_test)?)$' --output-on-failure`

Broader proof should be used when the shared contract changes across multiple
control-flow families:

- `ctest --test-dir build -L x86_backend --output-on-failure`

If a slice changes phi replacement, conditional semantics, or loop-carried
handoff broadly enough, broader backend validation should be treated as normal,
not exceptional.

## First Implementation Questions

The first real execution packet derived from this idea should answer these
questions concretely:

1. what exact prepared structure will become the authoritative representation
   for join-transfer semantics after phi elimination
2. what exact prepared structure will become the authoritative representation
   for conditional branch semantics when compare-driven branches are present
3. which phase will create those structures
4. what minimal invariant set will be documented in headers for target
   consumers
5. which current x86 matcher family will be removed or bypassed first as proof
   that the contract is real

## Durable Summary

The current public x86 route is not primarily blocked by one missing x86 trick.

It is blocked by a missing shared control-flow contract.

Phi elimination currently removes syntax without fully replacing semantic
ownership.

Boolean promotion currently avoids raw target-facing `i1` without fully
replacing branch-condition ownership.

Prepared handoff currently exposes blocks and terminators without fully
exposing what joins and loop headers are allowed to assume.

The reference backend under
`ref/claudes-c-compiler/src/backend`
demonstrates a healthier ownership line:

- shared generation knows whether it is lowering an instruction or terminator
- shared generation has explicit control-flow hooks
- targets provide legal instruction sequences for already-decided semantic
  families

C4c does not need to clone that implementation, but it does need to restore the
same kind of ownership discipline.

This idea is the durable source record of that requirement.
