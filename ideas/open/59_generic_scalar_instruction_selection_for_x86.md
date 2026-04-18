# Generic Scalar Instruction Selection For Prepared X86

Status: Open
Created: 2026-04-18

## Why This Idea Exists

The current prepared x86 route still treats scalar code generation as a
collection of bounded matcher families.

Those matcher families can emit some working assembly.

They do not establish a durable backend contract.

They do not scale across nearby shapes.

They do not create one place where scalar lowering policy actually lives.

That is why this idea exists.

This idea is not asking for "more x86 support" in the vague sense.

It is asking for one specific missing layer:

generic scalar instruction selection over prepared inputs that are already
owned upstream.

The missing layer sits after:

- idea 58 makes control-flow and join semantics explicit
- idea 60 makes value homes, frame facts, addresses, and move obligations
  explicit

The missing layer sits before:

- final x86 text emission for scalar operations

Without that layer, the backend has no honest place to answer ordinary
questions such as:

- how to lower an add when both inputs are stack-resident
- how to lower a compare when the result only feeds a branch
- how to lower a select without reconstructing CFG shape
- how to lower a load or store once the canonical address is already known
- how to lower a call once the move plan and ABI placement are already known

Today those questions are answered inconsistently.

Some are answered inside one-off helpers.

Some are answered by whole-function pattern recognition.

Some are answered by hand-built lane logic.

Some are not answered at all.

That arrangement is exactly what produced the `try_*` surface in the prepared
x86 emitter.

This idea exists to replace that arrangement with a durable scalar lowering
contract.

## Short Thesis

Prepared x86 does not mainly lack opcodes.

Prepared x86 lacks the contract that chooses legal scalar instruction
sequences from:

- explicit scalar semantics
- explicit control-flow semantics
- explicit value locations
- explicit addressing facts
- explicit move obligations

The output of this idea is not "support for one more testcase".

The output of this idea is one honest layer where scalar lowering happens
without whole-shape matcher growth.

## What This Idea Is

This is a durable source idea.

It is not a packet plan.

It is not a runbook.

It is not claiming that all scalar lowering should land in one patch.

It exists to define the source intent and ownership boundary for a missing
backend layer.

That layer should survive later planning changes.

That layer should survive test churn.

That layer should survive movement between implementation slices.

## What "Instruction Selection" Means Here

In this repository, "instruction selection" must not be interpreted as the
trivial rule:

`BIR_ADD -> add`

That rule is too thin to describe real machine lowering.

For this idea, scalar instruction selection means:

given one prepared scalar operation and its prepared context,

choose a legal x86 instruction sequence and operand form that:

- respects x86 operand constraints
- respects value locations from idea 60
- respects control-flow semantics from idea 58
- respects move obligations from idea 60
- preserves the semantics of the scalar operation
- does not rely on whole-function or whole-CFG pattern recognition

The unit of selection is therefore not "one opcode".

The unit of selection is "one scalar semantic operation plus its prepared
inputs".

Sometimes that lowers to one instruction.

Sometimes it lowers to several.

Sometimes it fuses with branch emission.

Sometimes it consumes an existing move plan.

Sometimes it must refuse a particular operand form and choose a scratch-based
sequence instead.

That is the contract this idea owns.

## Why The Current Prepared X86 Route Drifted Into Matchers

The current prepared x86 path is missing a clean scalar lowering surface.

Instead of:

- per-operation dispatch
- per-terminator dispatch
- operand-form selection
- canonical call sequence emission
- canonical compare and branch fusion

the route fell back to recognizing larger supported shapes.

Those shapes are not random.

They are the result of a missing layer.

If a backend has no generic way to lower:

- arithmetic over canonical locations
- compare plus branch
- select
- load and store
- call

then it naturally starts to ask:

"Does this whole block or whole function look like one of the few shapes I
already know how to spell?"

That is what a `try_*` family is doing.

The `try_*` function is not merely ugly naming.

It is evidence that the route is selecting over whole shapes instead of over
prepared scalar operations.

This idea exists specifically to reject that route.

## Current Gap In One Sentence

The prepared route has scalar semantics,
but it does not yet have a generic scalar selector that consumes prepared
control-flow and prepared storage facts.

## Comparison With The Reference Backend

The reference backend under
`ref/claudes-c-compiler/src/backend`
does not solve this problem by matching entire function shapes.

It solves it by layering.

That layering matters more than any one helper.

### Reference Observation 1: Shared Generation Dispatch Exists

In the reference backend,
`generation.rs`
owns a shared generation loop.

That loop:

- generates one function at a time
- iterates blocks
- dispatches each instruction
- dispatches each terminator

The shared driver is explicit.

It does not ask whether the function belongs to one pre-approved family.

It asks which instruction or terminator it is looking at.

Then it calls the relevant trait method.

Examples visible in the reference design:

- `Instruction::BinOp` dispatches to `emit_binop`
- `Instruction::Load` dispatches to `emit_load`
- `Instruction::Store` dispatches to `emit_store`
- `Instruction::Select` dispatches to `emit_select`
- `Instruction::Call` dispatches to `emit_call`
- fused compare-branch lowering has a named path instead of being discovered
  from arbitrary later shape recovery

That is already a huge architectural difference from the current prepared x86
route in this repository.

### Reference Observation 2: Shared Trait Contract Exists

The reference backend has a large `ArchCodegen` trait.

That trait is not important because it is large.

It is important because it expresses ownership.

The shared generation layer knows:

- when a load is being lowered
- when a store is being lowered
- when a binop is being lowered
- when a compare is being lowered
- when a select is being lowered
- when a call is being lowered
- when a branch is being lowered

The backend implementation supplies the architecture-specific behavior.

The algorithmic split is therefore:

- shared framework owns dispatch and many common patterns
- architecture layer owns legal instruction spelling and architecture-specific
  constraints

That arrangement is exactly what the prepared x86 route in this repository
does not yet have.

### Reference Observation 3: Operand And Frame Facts Are Consumed, Not Rebuilt

The reference x86 path consumes:

- stack layout
- register assignments
- slot addresses
- ABI classification

Those are not rediscovered by matching a whole function.

Those are used as selector inputs.

This is the key difference.

Instruction selection becomes possible only because the selector has honest
inputs.

The selector is not inventing those inputs locally.

### Reference Observation 4: Module Decomposition Matches Operation Families

The reference x86 codegen is split across modules such as:

- arithmetic
- comparison
- memory
- calls
- prologue
- returns

This is not merely code style.

It indicates that the backend is organized by lowering concern.

The prepared x86 route here has been organized too much by recognized shape.

This idea adopts the reference lesson that operation-family decomposition is
the correct scalar lowering boundary.

### Reference Observation 5: A Single Compare Can Lower Differently Depending On Use

The reference path explicitly supports compare-branch fusion.

That design matters.

It shows that instruction selection is use-sensitive.

A compare that only feeds a branch does not necessarily need materialized
boolean storage.

A compare that feeds a value path may need a different route.

That distinction is part of selector policy.

The current prepared x86 route lacks a stable place for that policy.

### Reference Observation 6: Calls Are Treated As Sequences, Not As One Opcode

The reference backend does not pretend that `Call` maps directly to one
instruction.

It computes stack argument space.

It emits stack arguments.

It emits register arguments.

It emits the call instruction.

It emits cleanup.

It stores the result.

This idea adopts the same perspective:

scalar instruction selection includes choosing scalar call sequences once
prepared move and ABI facts already exist.

### Reference Observation 7: Memory Lowering Chooses Operand Forms

The reference backend's memory path chooses among:

- direct slot access
- indirect pointer access
- aligned alloca addressing
- folded constant offsets

The important point is not any one helper.

The important point is that memory lowering is driven by canonical address
class and value location.

The current prepared x86 route still reconstructs too much of that privately.

Idea 60 is meant to fix the ownership of those inputs.

This idea depends on that.

### Reference Observation 8: The Shared Layer Makes It Harder To Add Shape Overfit

When dispatch is per-instruction and per-terminator,
it becomes much harder to "solve" a failing case by adding a whole-function
matcher.

That is a feature.

It forces new work to land in reusable lowering logic.

This idea explicitly wants that pressure.

## The Current Repository Gap Versus The Reference Design

The reference backend already has:

- a shared generation driver
- a backend trait contract
- explicit operation-family lowering surfaces
- canonical slot and register consumption
- canonical call emission decomposition

The current prepared x86 route in this repository still lacks the equivalent
prepared-route scalar contract.

The gap is not that the current repository forgot the mnemonic for `add`.

The gap is that the current repository has no durable place to decide:

- whether an `add` can be in-place
- whether it needs a scratch register
- whether a compare should materialize a boolean
- whether a select should use `cmov` or branchy lowering
- whether a call result can stay in a register or must be copied immediately
- whether an address can be folded directly or needs staging

Those decisions are the selector.

Without that layer,
the emitter has drifted into lane logic.

This idea exists to define the missing layer.

## Goal

Define and eventually implement a generic scalar lowering contract for prepared
x86 that:

- consumes canonical control-flow semantics from idea 58
- consumes canonical value-location, address, frame, and move facts from
  idea 60
- lowers scalar operations by operation family rather than by whole-function
  shape
- chooses legal x86 instruction sequences instead of assuming a one-opcode map
- removes the need for new bounded `try_*` scalar matcher growth

## Success Condition In Plain Language

After this idea is satisfied,
the x86 backend should be able to look at one prepared scalar operation and
say:

"I know what this operation means,
I know where its values live,
I know whether its compare feeds control flow or a value,
I know how its address is represented,
I know which copies are already upstream obligations,
and I can choose a legal x86 sequence without reconstructing the entire
function."

That is the success condition.

## This Idea Owns

This idea owns the scalar instruction-selection contract for prepared x86.

That ownership includes all of the following.

### 1. Per-Operation Scalar Lowering Policy

This idea owns the rule that scalar operations should be lowered by operation
family,
not by whole-function or whole-CFG shape.

### 2. Legal Operand-Form Selection For Scalar Operations

This idea owns the decision logic that turns prepared scalar inputs into legal
x86 operand forms.

Examples:

- register plus register
- register plus immediate
- accumulator plus secondary register
- stack-resident source loaded into scratch first
- compare fused directly into conditional branch

### 3. Arithmetic And Bitwise Lowering Policy

This idea owns generic scalar lowering policy for integer arithmetic and
bitwise operations once operand locations are known.

That includes:

- add
- sub
- and
- or
- xor
- mul
- shifts
- division and remainder routes where applicable

### 4. Compare Lowering Policy

This idea owns the scalar selector policy for compares.

That includes the question of whether the compare:

- materializes a value
- feeds branch lowering directly
- feeds select lowering
- uses flags only transiently

### 5. Compare-Branch Fusion Policy

Once idea 58 makes branch semantics explicit,
this idea owns the selector policy for whether and how compare plus branch are
lowered as one scalar lowering route.

### 6. Scalar Select Lowering Policy

This idea owns the route for lowering scalar `select`.

That includes choosing among:

- `cmov`-style lowering
- branchy lowering
- compare-result reuse when valid

The point is not that only one route must exist.

The point is that one documented selector policy must own the decision.

### 7. Scalar Memory-Operation Selection

Once idea 60 provides canonical addresses and value homes,
this idea owns the final scalar load and store selection logic.

It does not own address provenance itself.

It owns selecting a legal x86 memory access sequence from that provenance.

### 8. Scalar Call Sequence Selection

Once idea 60 provides move obligations and ABI placement facts,
this idea owns selecting the scalar call emission sequence.

That includes:

- direct versus indirect call spelling
- sequencing of already-decided argument moves
- result capture
- scalar return-value materialization policy

### 9. Use-Sensitive Lowering Rules

This idea owns the fact that one scalar semantic operation may lower
differently depending on its prepared use.

Examples:

- compare feeding branch
- compare feeding materialized boolean
- load used only as a source for a call move
- add whose result can stay in an assigned register

### 10. Anti-Overfit Rejection At The Scalar Layer

This idea owns the rule that scalar progress must not be measured by adding
new named matcher families.

If the only way a slice works is by recognizing a larger shape,
that is evidence that the scalar contract is still missing.

## This Idea Does Not Own

This idea intentionally does not own several adjacent concerns.

Those exclusions are part of the source intent.

### 1. It Does Not Own CFG, Join, Or Loop Semantics

This idea does not decide:

- how join inputs are represented after phi elimination
- how loop-carried values are surfaced
- how branch condition semantics are represented in prepared form

Those belong to idea 58.

This idea consumes those semantics.

It does not invent them.

### 2. It Does Not Own Value Homes

This idea does not decide:

- which values live in registers
- which values live in stack slots
- which moves occur at joins
- which moves occur for call arguments or returns

Those belong to idea 60.

This idea consumes those facts.

### 3. It Does Not Own Frame Size Or Slot Offsets

This idea does not own:

- frame sizing
- frame alignment
- slot assignment
- stack-address provenance

Those are idea 60 concerns.

### 4. It Does Not Own General Backend Restructuring Beyond Scalar Lowering

This idea does not require that the entire backend immediately match the
reference architecture in every detail.

It does require that scalar lowering ownership become honest.

### 5. It Does Not Permit New Whole-Shape Matcher Expansion

This idea explicitly rejects progress routes such as:

- adding a new whole-function matcher for one arithmetic family
- adding a new whole-CFG matcher for one compare pattern
- extending `try_*` growth as a substitute for generic scalar lowering

### 6. It Does Not Own Aggregate Or Vector Strategy Beyond Scalar Boundaries

If a future route needs aggregate or vector lowering strategy,
that must be owned separately unless it is strictly necessary for scalar value
transport already covered by idea 60.

### 7. It Does Not Own Re-Legalizing BIR Into A Different Shared IR By Itself

This idea may eventually motivate a better lowering surface.

It does not by itself claim ownership of designing an entirely new backend IR.

Its boundary is the prepared scalar selector contract.

## Relationship To Idea 58

Idea 58 owns control-flow and join materialization semantics.

This idea depends on that ownership.

Concretely,
this means the scalar selector must not be the place that re-discovers:

- which predecessor contributes a join input
- whether a compare result is a branch condition or a materialized value
- whether a loop latch updates a carried value
- how short-circuit control flow is represented

If the selector has to infer those things from block arrangement,
idea 58 is still missing the needed contract.

This idea is downstream of that fix.

## Relationship To Idea 60

Idea 60 owns prepared storage, frame, address, and move consumption.

This idea depends on those inputs.

Concretely,
this means the selector must not be the place that re-decides:

- slot layout
- address roots
- frame offsets
- register assignments
- join copies
- call argument copies
- return copies

If the selector has to invent those,
idea 60 is still missing the needed contract.

This idea is downstream of that fix.

## Scalar Selector Inputs

This section exists because the current repository often talks about support
families without first naming what inputs the lowering layer actually needs.

A generic selector cannot exist without explicit inputs.

For this idea,
the selector inputs are part of the contract.

### 1. Prepared Scalar Operation Kind

The selector must know the operation family.

Examples:

- add
- sub
- and
- or
- xor
- shift
- compare
- select
- load
- store
- call
- return-adjacent scalar transfer where relevant

### 2. Scalar Type Class

The selector must know the scalar type class.

Examples:

- i1-like logical condition already normalized away from target-facing form
- i8
- i16
- i32
- i64
- pointer-sized scalar
- scalar float classes if the prepared route grows them under this idea's
  surface

This idea is centered on generic scalar lowering.

It cannot rely on implicit width guesses.

### 3. Operand Locations

The selector must know where operands already live.

Examples:

- assigned register
- assigned stack slot
- immediate constant
- direct frame-relative memory location
- indirect address already materialized as a value

These facts must come from idea 60.

### 4. Destination Location Or Destination Ownership

The selector must know whether the result:

- must end up in a particular register
- must be stored into a stack slot
- can remain in a currently selected working register until a later required
  move
- is consumed immediately by a branch or call sequence

### 5. Prepared Control-Flow Context

For compare,
select,
and branch-adjacent scalar operations,
the selector must know their control-flow context from idea 58.

Examples:

- compare feeds branch directly
- compare feeds a materialized scalar
- select has already been normalized into an explicit branch-owned route or
  remains a value-level select

### 6. Prepared Addressing Facts

For load and store,
the selector must know:

- direct slot versus indirect address
- base plus constant offset if already canonicalized
- address class and legality constraints

Those facts come from idea 60.

### 7. Prepared Move Obligations

For call and some result-routing cases,
the selector must know which copies are already mandatory.

Those copies must not be silently re-decided locally.

### 8. ABI Facts Already Settled Upstream

For call and return-adjacent scalar behavior,
the selector must know enough ABI facts to emit the right sequence without
reclassifying everything from scratch.

This does not mean the selector owns ABI classification.

It means the selector consumes the already settled ABI placement contract.

### 9. Scratch And Clobber Policy

The selector must know what scratch policy it is allowed to use.

If scratch use is unconstrained or implicit,
the selector boundary is still too weak.

The prepared route needs a documented answer.

### 10. Legal Fusion Opportunities

The selector must know when fusion is semantically permitted.

Examples:

- compare plus conditional branch
- compare plus select
- load plus address folding where already canonicalized

The selector must not invent semantic fusion that upstream contracts did not
permit.

## Scalar Selector Outputs

The selector output is not "assembly text" in the abstract.

The selector output is a legal scalar lowering decision.

In the current repository,
that may still be emitted directly to x86 assembly text.

Even then,
the output contract should be thought of as:

- chosen operand form
- chosen instruction sequence
- chosen scratch usage
- chosen result placement behavior
- chosen fusion behavior

That output must be explainable from prepared inputs.

If it cannot be explained from those inputs,
the lowering route is still implicit.

## Non-Goals

This idea includes several strong non-goals so that later planning does not
quietly re-expand the scope.

### Non-Goal 1: Recreating The Entire Reference Backend Architecture All At Once

The reference backend is useful as a design comparison.

This idea does not claim that the current repository must mirror its exact file
graph immediately.

What must be mirrored is the ownership discipline.

### Non-Goal 2: One Giant Scalar Rewrite In One Slice

This idea is not claiming that arithmetic,
compare,
select,
memory,
and call lowering must all land together.

It is saying they must all eventually belong to the same contract family.

### Non-Goal 3: Tactical Matcher Growth

This idea explicitly rejects "temporary" growth that hardens into the new
baseline.

That includes:

- one more compare lane
- one more guard lane
- one more countdown variant
- one more direct call family

Those are route drift under this idea.

### Non-Goal 4: Owning Upstream Semantic Normalization

If a scalar lowering slice discovers that a needed semantic normalization is
missing upstream,
that is evidence to push back to idea 58 or idea 60,
not evidence that this idea should silently absorb the missing contract.

### Non-Goal 5: Testcase-First Support Claims

This idea rejects the notion that support should be claimed primarily in terms
of one named failing test.

Tests are evidence.

They are not the contract.

## Why `BIR_ADD -> x86 add` Is Not Enough

This section is explicit because the repository discussion keeps returning to
this intuition.

The intuition is understandable.

It is also incomplete.

### Example 1: Memory-To-Memory Arithmetic Is Not Legal

Suppose the prepared scalar operation is:

`t3 = add t1, t2`

If:

- `t1` lives in a stack slot
- `t2` lives in a different stack slot
- `t3` must end up in a third stack slot

the selector cannot emit:

`add [mem], [mem]`

because x86 does not permit memory-to-memory ALU forms like that.

The selector must choose a legal sequence.

For example:

- load one operand to a working register
- add the other from memory or another register
- store the result if required

That is instruction selection.

It is not solved by naming the mnemonic.

### Example 2: Compare May Not Need A Materialized Boolean

Suppose the prepared scalar semantics are equivalent to:

- compare x against zero
- branch on equality

If the compare feeds only the branch,
the selector may choose:

- compare or test
- conditional jump

without ever materializing a boolean result.

That is not the same as:

- compute a boolean value
- store it somewhere
- load it again for branch

The selector needs use-sensitive policy.

### Example 3: Select Is Strategy Selection, Not Mnemonic Lookup

A scalar select may lower through:

- compare and conditional move
- branchy control flow plus value transfer
- another prepared route permitted by idea 58

The selector must choose.

No single `select` mnemonic answers that.

### Example 4: Call Is A Sequence

A scalar call often requires:

- already-decided move setup
- direct or indirect call spelling
- result capture
- cleanup

The selector must sequence those pieces.

`call` by itself is not the contract.

### Example 5: Operand Width And Extension Behavior Matter

An `i32` route and an `i64` route are not interchangeable.

Signedness,
zero-extension,
and subregister semantics all matter.

The selector must own those rules.

## Current Failure Modes This Idea Addresses

### Failure Mode 1: Arithmetic Support Is Still Family-Shaped

Current prepared x86 arithmetic support is concentrated in narrow families.

That means nearby semantically equivalent forms often fail unless the whole
shape matches an existing lane.

The deeper problem is not arithmetic itself.

The deeper problem is that no generic scalar arithmetic selector exists over
prepared locations.

### Failure Mode 2: Compare And Branch Ownership Is Split Between Layers

Without idea 58,
compare-to-branch semantics are ambiguous.

Without this idea,
even once those semantics exist,
there is still no stable scalar lowering owner that chooses:

- compare instruction form
- flag usage
- branch emission sequence

### Failure Mode 3: Select Has No Durable Lowering Home

A scalar select currently risks being:

- unsupported
- folded into a shape family
- reinterpreted through hand-built compare or branch logic

That is ownership failure.

This idea names select lowering as a first-class scalar selector concern.

### Failure Mode 4: Load And Store Are Entangled With Route-Specific Logic

Even after idea 60 clarifies canonical addresses,
the backend still needs one place that chooses legal load and store sequences.

Without that place,
memory lowering remains coupled to specific route families.

### Failure Mode 5: Call Lowering Is Still Too Lane-Oriented In The Prepared Path

Direct calls,
simple helper calls,
and scalar returns are still too tightly coupled to narrow routes in the
prepared emitter.

This idea owns the generic scalar call-lowering surface once idea 60 provides
honest ABI and move inputs.

### Failure Mode 6: The Route Encourages Overfit

As long as new support is easiest to express by adding one more matcher,
the repository will keep drifting toward overfit.

This idea changes the intended path of least resistance.

## Owned Contracts In More Detail

This section makes the owned contract more concrete.

### Contract A: Operation-Family Dispatch

Prepared x86 scalar lowering should be organized around explicit operation
families.

At minimum,
the durable surface should make room for:

- arithmetic and bitwise
- shifts
- compare
- compare plus branch fusion
- select
- scalar load
- scalar store
- scalar call
- scalar result capture

Whether these live in one file or many is not the source-idea question.

The durable requirement is that these concerns have explicit lowering homes.

### Contract B: Location-Aware Lowering

Every scalar family under this idea must lower from canonical prepared
locations.

If a lowering path needs to know where an operand is,
it must ask the prepared ownership layer from idea 60.

It must not reconstruct the answer locally from raw BIR shape.

### Contract C: Use-Aware Compare Lowering

Compare lowering must be explicit about whether it is lowering:

- a value-producing compare
- a compare fused into branch lowering
- a compare reused by select lowering if the contract permits

This is one of the most important missing contracts in the current route.

### Contract D: Flag Usage Policy

x86 flags are not an incidental detail.

They are part of scalar lowering policy.

The prepared scalar selector must have a documented answer for:

- when flags are produced
- how long their meaning is assumed live
- when they are consumed immediately
- when a compare result must be materialized instead of relying on flags

This can remain a local x86 concern.

It still must be explicit.

### Contract E: Load And Store Selection Over Canonical Addresses

Once idea 60 provides the address class,
this idea owns selecting legal scalar memory operations.

That includes choosing between:

- direct frame-relative access
- indirect pointer-based access
- folded constant-offset access
- scratch-based sequences when direct forms are not legal

### Contract F: Call Sequence Selection Over Canonical ABI Moves

This idea owns the final scalar call sequence over prepared ABI and move facts.

It does not own recomputing those facts.

It does own:

- direct call spelling
- indirect call spelling
- ordering of already-owned argument setup
- result capture sequence
- scalar cleanup behavior that belongs to the target emission stage

### Contract G: Result Placement Policy

For each scalar family,
the selector must have a defined rule for how results are left behind.

Examples:

- keep in assigned register
- write to assigned stack slot
- hand off directly to a consuming move plan

This policy is part of instruction selection.

### Contract H: Reusable Helper Boundaries

This idea wants reusable helpers that reflect scalar concerns,
not testcase names.

Good helper boundaries resemble:

- emit integer arithmetic from two prepared scalar operands
- lower compare to flags and branch
- lower load from canonical address class
- materialize call result to assigned destination

Bad helper boundaries resemble:

- render local countdown loop shape
- render bounded helper family
- render one special guard chain

## Decomposition By Scalar Family

This section is intentionally longer than a normal note.

The current repository has suffered from treating scalar support as one blurry
mass.

That blur made it easy to smuggle shape matchers into the route.

This source idea instead decomposes by scalar family.

### Arithmetic And Bitwise Family

This family covers:

- add
- sub
- and
- or
- xor
- mul
- possibly div and rem depending on the prepared route width of the slice

The family owns:

- legal operand forms
- width handling
- scratch selection when both operands are stack-resident
- result placement

The family does not own:

- where operands live
- whether a CFG shape is a "recognized arithmetic family"

Questions this family must answer:

- can the destination be the same as one source when the assigned location
  permits it
- when is an immediate form legal
- when does the selector need a secondary register
- how are stack-resident operands staged without inventing new local layout
  logic
- what width-specific extension rules are required for the prepared scalar type

Evidence of success for this family is not:

- a new matcher for parameter-plus-immediate

Evidence of success is:

- arithmetic lowering works over canonical prepared operand locations

### Shift Family

Shift operations are close to arithmetic.

They still deserve explicit mention because x86 shift constraints differ from
ordinary binary ALU forms.

This family owns:

- legal count-operand handling
- width-specific shift form choice
- result placement under canonical destination ownership

This family is a good example of why selector inputs matter.

Even if BIR says "shift",
the x86 route still needs to choose a legal form.

### Compare Family

The compare family covers value-producing compare behavior.

It owns:

- compare instruction form
- width-aware compare behavior
- result materialization policy when a scalar boolean-like value is actually
  required

This family depends heavily on idea 58.

If idea 58 does not make compare consumers explicit enough,
the compare selector cannot know whether it is lowering a value or a branch
condition.

### Compare Plus Branch Family

This family covers the direct selector route where compare semantics feed
conditional control flow.

It owns:

- compare emission suitable for branch consumption
- immediate branch emission
- canonical true and false edge spelling once control-flow ownership is
  explicit

This family should replace ad hoc compare-branch shape families.

### Select Family

The select family covers scalar conditional value selection.

It owns:

- strategy selection
- legality of a `cmov`-style route where applicable
- legality of a branchy route where applicable
- interaction with compare lowering and prepared branch semantics
- destination placement

This family must not quietly recreate join or branch semantics that idea 58 is
supposed to own.

### Load Family

The load family covers scalar loads from canonical prepared addresses.

It owns:

- legal memory operand selection
- width-appropriate load form
- folded constant-offset access when the prepared address says it is valid
- scratch usage when indirect paths require staging
- destination placement

It does not own:

- discovering the address root
- guessing local-slot ancestry

Those are upstream ownership concerns.

### Store Family

The store family covers scalar stores to canonical prepared addresses.

It owns:

- legal store sequence selection
- staging when direct forms are not legal
- width-appropriate store forms
- respecting canonical address class

It does not own:

- redesigning address provenance
- privately inventing local-root reconstruction

### Call Family

The call family covers scalar call emission once argument and result ownership
has already been made explicit.

It owns:

- direct or indirect call instruction form
- ordering and materialization of prepared argument moves
- result capture
- scalar cleanup behavior at the emission stage

It does not own:

- reclassifying call arguments from source language type rules
- inventing a separate call ABI plan from scratch

### Scalar Result Transfer Family

Some scalar behavior sits between an operation and its final consumer.

Examples:

- call result capture
- immediate transfer to assigned storage
- use of a result by a prepared move obligation

This idea owns the selector policy for those scalar transfers when they are the
direct continuation of a scalar operation family.

## Architectural Principles This Idea Adopts From The Reference Backend

The current repository should not copy the reference backend mechanically.

It should adopt several of its architectural principles.

### Principle 1: Lower By Operation, Not By Whole Shape

This is the most important principle.

### Principle 2: Consume Prepared Facts Instead Of Reconstructing Them

Selector inputs must be canonical.

### Principle 3: Let Shared Or Upstream Layers Own Semantics

The scalar selector should not be the first place that control-flow meaning
becomes explicit.

### Principle 4: Make Reuse Easy And Overfit Hard

A new scalar support slice should naturally land in an operation-family helper,
not in a testcase-shaped lane.

### Principle 5: Keep Call, Memory, Compare, And Arithmetic As Distinct Concerns

They interact.

They still need explicit homes.

### Principle 6: Encode Use-Sensitive Lowering As Policy

Compare feeding branch is different from compare feeding a materialized value.

That difference should be policy,
not incidental behavior.

## Anti-Patterns This Idea Explicitly Rejects

### Anti-Pattern 1: Whole-Function Scalar Recognition

If scalar lowering requires identifying an entire function as one family,
the selector contract is still missing.

### Anti-Pattern 2: CFG-Shape Recovery Inside X86 Scalar Helpers

If a helper must inspect predecessor or successor shape just to decide how to
lower an add,
compare,
or select,
the control-flow contract is still missing.

### Anti-Pattern 3: Emitter-Local Storage Reconstruction

If a scalar helper reconstructs slot layout or address roots privately,
idea 60 has not yet been honored.

### Anti-Pattern 4: Naming Helpers After Test Shapes

A helper named after a testcase or narrow route is almost always evidence of
overfit.

### Anti-Pattern 5: Counting Progress By New Supported Families

Progress under this idea is not "we support one more family name".

Progress is "one scalar concern now lowers generically over prepared inputs".

### Anti-Pattern 6: Sneaking Runbook Detail Into Source Intent

This idea may list decomposition and ordering.

It should still remain durable.

It should not become a packet checklist.

## Expected Selector Boundaries

This section is intentionally concrete.

The current state suffers from vague references to "generic lowering" without
naming boundaries.

This idea expects boundaries that look roughly like these concerns:

- scalar arithmetic lowering over prepared locations
- scalar compare lowering over prepared locations and control-flow context
- scalar select lowering over prepared conditions and destinations
- scalar load lowering over canonical addresses
- scalar store lowering over canonical addresses
- scalar call emission over prepared ABI and move facts

Whether these end up as methods,
classes,
files,
or modules is not the source-idea requirement.

The source-idea requirement is that they become explicit,
reusable,
and not shape-specific.

## Possible File Targets

This idea is source intent,
not a runbook,
but it should still name the files likely to embody the contract.

Primary likely targets include:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `src/backend/mir/x86/codegen/emit.cpp`
- any new x86 codegen files that split scalar lowering by concern
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/regalloc.cpp`
- `src/backend/prealloc/legalize.cpp`
- tests that currently encode prepared x86 handoff and route expectations

The important point is not the exact file list.

The important point is that the contract likely spans:

- upstream prepared interfaces
- x86 scalar lowering code
- tests that prove contract-level behavior

## Interactions With Existing Prepared X86 Emitter Code

This idea does not assume that the current emitter file disappears immediately.

It does assume that the role of that file changes.

Today too much scalar behavior in the prepared emitter is organized as support
lanes.

Under this idea,
the emitter should instead become a home for:

- dispatch over prepared scalar concerns
- legal scalar sequence emission
- small reusable helpers named after operation concerns

The emitter should stop being a shape recognizer.

## Migration Intent

This source idea is not a runbook,
but it does need durable migration intent.

That intent is:

move from shape-owned scalar support
to operation-owned scalar support.

More concretely:

- arithmetic families should collapse into generic arithmetic lowering
- compare families should collapse into compare and compare-branch lowering
- select-like routes should collapse into generic select lowering
- direct load and store families should collapse into generic memory lowering
- direct call families should collapse into generic scalar call lowering

This migration is the point of the idea.

## Ordering Within This Idea

This section is intentionally not a plan,
but it does define durable ordering assumptions.

### Ordering Assumption 1: Do Not Activate This Idea Before 58 And 60 Are Honest Enough

If idea 58 does not yet expose branch and join semantics cleanly,
the compare and select parts of this idea will be forced back into inference.

If idea 60 does not yet expose value locations,
frame facts,
addresses,
and move obligations cleanly,
the arithmetic,
memory,
and call parts of this idea will be forced back into reconstruction.

So the durable ordering rule is:

this idea should usually follow substantive progress in both 58 and 60.

### Ordering Assumption 2: Arithmetic Or Compare Is A Better First Scalar Surface Than "Everything"

If a future runbook needs to pick a first scalar surface,
this source idea considers arithmetic or compare-plus-branch more natural early
surfaces than "all scalar lowering at once".

That is not a packet decision.

It is a durable decomposition note.

### Ordering Assumption 3: Call Lowering Should Consume Upstream ABI Facts, Not Lead Them

Call lowering should not become the place where move obligations are defined.

That belongs upstream.

### Ordering Assumption 4: Memory Lowering Should Follow Canonical Address Ownership

If address provenance is still emitter-local,
memory lowering here will drift immediately.

## Decomposition Candidates

These are not execution packets.

They are durable decomposition candidates that a future plan may turn into
steps.

### Candidate A: Generic Integer Arithmetic Lowering

This candidate would focus on:

- add
- sub
- and
- or
- xor
- mul
- shift

over canonical prepared locations.

The goal would be to establish the first honest scalar selector surface.

### Candidate B: Generic Compare And Compare-Branch Lowering

This candidate would focus on:

- compare as value
- compare fused to branch
- explicit flag usage policy

The goal would be to displace compare-oriented shape lanes.

### Candidate C: Generic Select Lowering

This candidate would focus on:

- scalar select strategy
- interaction with compare policy
- destination placement

The goal would be to give select a durable lowering owner.

### Candidate D: Generic Scalar Load And Store Lowering

This candidate would focus on:

- direct addresses
- indirect addresses
- constant offset folding if canonicalized
- staged scratch-based memory sequences

The goal would be to stop route-specific local-memory support growth.

### Candidate E: Generic Scalar Call Lowering

This candidate would focus on:

- scalar call sequencing
- already-owned argument moves
- result capture
- direct and indirect call forms

The goal would be to remove lane-specific call support.

## Expected Questions A Future Plan Must Answer

These questions belong in the source idea because they define what it means to
do the work honestly.

### Question 1: What Is The First Stable Selector API Surface?

The repository will need one explicit answer for how the prepared x86 path asks
to lower a scalar operation.

Whether that surface is a method family,
dispatcher,
or helper set is not the source-idea decision.

The requirement is that it exists.

### Question 2: How Are Prepared Locations Presented To Scalar Lowering?

This depends on idea 60.

The scalar layer needs one canonical query surface for:

- source operand location
- destination location
- address class
- required move obligations

### Question 3: How Is Compare Use Expressed?

This depends on idea 58.

The scalar layer needs to know:

- compare feeds branch
- compare feeds select
- compare feeds value materialization

without rediscovering that from shape.

### Question 4: What Scratch Model Is Allowed?

The scalar layer needs a disciplined answer for temporary register use.

If scratch choice is entirely ad hoc,
selector behavior will remain fragile.

### Question 5: What Proof Demonstrates Genericity?

The repository should not accept a scalar slice merely because one testcase
turns green.

The proof should show that:

- the new scalar route works over a class of prepared operand-location forms
- the route is not keyed to one CFG shape
- the route does not require new matcher growth

## Invariants This Idea Wants The Repository To Be Able To State

Once this idea is substantially realized,
the repository should be able to state all of the following honestly.

### Invariant 1

Prepared x86 scalar lowering is organized by scalar operation family,
not by whole-function shape.

### Invariant 2

Prepared x86 scalar lowering consumes canonical prepared value and address
facts,
not emitter-local reconstructions.

### Invariant 3

Prepared x86 compare lowering has explicit policy for branch consumers,
value consumers,
and select consumers.

### Invariant 4

Prepared x86 load and store lowering uses canonical address classes and legal
x86 operand-form selection.

### Invariant 5

Prepared x86 scalar call lowering consumes upstream ABI and move obligations
instead of inventing parallel local policy.

### Invariant 6

Support claims for scalar routes are framed in terms of reusable lowering
behavior rather than named matcher families.

## Evidence That This Idea Is Not Yet Satisfied

At the time this source idea is being written,
several conditions show that the contract is still missing.

### Evidence 1

The prepared route still has a large history of `try_*` matcher growth.

### Evidence 2

The repository discussion still falls naturally into phrases such as:

- local-slot guard family
- compare-branch family
- countdown loop family
- direct helper family

Those phrases are evidence that the support surface is shape-oriented.

### Evidence 3

There is not yet one clear answer for where generic scalar lowering policy
lives.

### Evidence 4

The prepared route still depends too much on emitter-local reasoning for memory
and value transport.

### Evidence 5

The current route is still vulnerable to a new failing case being "fixed" by a
new matcher rather than by extending one scalar family.

## How This Idea Interacts With Testing

Tests matter.

They are not the source contract.

This section exists so that later planning does not confuse evidence with
ownership.

### Good Test Evidence Under This Idea

Good evidence includes:

- one scalar family works across multiple operand-location combinations
- compare and branch lowering behave consistently across several CFG contexts
  already normalized by idea 58
- load and store lowering work across direct and indirect canonical addresses
- call lowering works from prepared move plans rather than route-local setup

### Weak Test Evidence Under This Idea

Weak evidence includes:

- one named testcase is now green
- one old matcher family expectation was rewritten
- one support string was broadened without generic lowering proof

### Bad Test Evidence Under This Idea

Bad evidence includes:

- downgrading tests to make the route look cleaner
- adding expectations that bless a new matcher family
- proving only the exact shape that the new code recognizes

## Acceptance Criteria

This idea is not complete until evidence exists for all of the following.

- [ ] prepared x86 has at least one explicit scalar lowering surface organized
      by operation family rather than by whole-shape matcher
- [ ] scalar arithmetic lowering can consume canonical prepared operand
      locations without adding a new scalar `try_*` family
- [ ] compare lowering has explicit use-sensitive policy for at least value and
      branch consumption
- [ ] scalar compare-to-branch lowering consumes explicit control-flow
      semantics from idea 58 instead of reconstructing them from block shape
- [ ] scalar load and store lowering consume canonical address and storage
      facts from idea 60 instead of rebuilding address provenance locally
- [ ] scalar call lowering consumes prepared move and ABI facts rather than
      route-local call family logic
- [ ] progress under this idea is described as reusable scalar lowering
      behavior, not as one more supported family name
- [ ] at least one prior matcher-shaped scalar support surface becomes
      unnecessary because the generic scalar route now exists
- [ ] the repository can explain the scalar lowering policy in terms of
      prepared inputs and legal x86 sequence choice

## Validation Intent

Validation under this idea should prove generic scalar behavior.

It should not merely prove that one named shape remains matched.

Base validation intent:

- `cmake --build --preset default`
- `ctest --test-dir build -R '^backend_' --output-on-failure`

When a slice changes compare,
select,
branch-adjacent scalar behavior,
validation should include the tests that exercise prepared x86 handoff and
branch semantics.

When a slice changes load,
store,
or call behavior,
validation should include the tests that exercise prepared location and address
consumption from idea 60.

When a slice becomes broad enough to affect several scalar families,
validation should escalate to broader x86 backend proof.

This source idea intentionally does not pin exact future subsets more narrowly
than that.

Those are runbook decisions.

## Primary Targets

Likely primary code and contract targets include:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `src/backend/mir/x86/codegen/emit.cpp`
- any new x86 scalar-lowering support files created to separate concerns
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/regalloc.cpp`
- `src/backend/prealloc/legalize.cpp`
- tests around prepared x86 handoff boundaries
- tests around prepared storage and branch ownership once ideas 58 and 60 land

The exact list may grow.

The durable point is that the scalar selector contract likely spans prepared
interfaces and x86 lowering code.

## Source-Idea-Level Open Questions

These are durable open questions that belong at the source-idea layer rather
than only in a future plan.

### Open Question 1

Should the prepared x86 route adopt an explicit per-operation dispatch surface
similar in spirit to the reference backend's shared generation layer,
or should it keep a prepared-module-specific dispatcher that still respects the
same ownership split?

This idea does not force one exact answer.

It does force the repository to choose one.

### Open Question 2

What is the minimum scalar selector API that is honest enough to prevent new
shape matcher growth?

### Open Question 3

How much compare-use information must idea 58 expose so that compare and select
lowering here remain free of CFG-shape inference?

### Open Question 4

How much location and move information must idea 60 expose so that arithmetic,
memory,
and call lowering here remain free of storage reconstruction?

### Open Question 5

What form should the repository use to express legal x86 operand-form choice so
that:

- helpers remain reusable
- tests can target contract behavior
- backend drift becomes easier to detect

## Rationale For The Current Three-Idea Split

This section exists because this idea used to be too short and too vague when
viewed beside ideas 58 and 60.

The split now should be understood as follows.

### Idea 58

Makes control-flow and join meaning explicit.

### Idea 60

Makes storage,
frame,
address,
and move facts explicit.

### Idea 59

Consumes those explicit facts to choose scalar x86 instruction sequences.

That split is not arbitrary.

It reflects the same kind of layering that makes the reference backend more
coherent:

- semantics and ownership first
- instruction-family lowering second

## What Completion Would Feel Like

This final section is intentionally plain.

If the repository has satisfied this idea,
backend conversations should stop sounding like:

- "we need one more guard family"
- "we need one more countdown route"
- "we need one more compare matcher"

and start sounding like:

- "the arithmetic selector does not yet handle stack-plus-immediate"
- "the compare-branch selector needs explicit prepared condition ownership"
- "the load selector is missing one canonical indirect address class"
- "the call selector is not yet consuming prepared result moves"

That shift in vocabulary is not cosmetic.

It is how the repository will know that scalar lowering ownership has moved out
of matcher families and into a real backend contract.
