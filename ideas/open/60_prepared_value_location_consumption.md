# Prepared Value-Location, Frame, Addressing, And Move Consumption For X86

Status: Open
Created: 2026-04-18
Last-Updated: 2026-04-18
Owner-Layer: `prepare` handoff contract consumed by the x86 prepared route
Depends-On:
- idea 58 for shared CFG, join, and loop materialization ownership
Blocks:
- idea 59 generic scalar instruction selection for x86

## Summary

This idea exists because the current x86 prepared-module route still treats
prepared storage and frame data as advisory instead of canonical.

Upstream `prepare` phases already compute a substantial amount of information
that a target backend should consume directly:

- stack objects
- frame slots
- frame size
- frame alignment
- liveness intervals
- call points
- register assignments
- stack-slot assignments
- spill and reload records
- move-resolution records for joins, calls, and returns

The public x86 prepared emitter does not yet honor that ownership line.
Instead, it still rebuilds local-slot layout from raw BIR, re-derives stack
offsets, re-decides several copy obligations locally, and privately infers
address provenance from BIR memory shapes. That creates exactly the kind of
backend-local reasoning that later grows into `try_*` capability lanes.

This idea defines the durable source intent for fixing that ownership seam.
It does not define a packet order or a runbook. It defines what must become
true about the boundary between:

- semantic BIR and prepared BIR
- prepared BIR and x86 prepared emission
- prealloc-owned facts and x86-owned spelling

The durable thesis is:

`prepare` must own the canonical answers to:

- where a value lives
- which stack object or frame slot represents a thing
- how stack-relative addressing is formed
- what copies or moves are required at joins, calls, and returns
- what frame shape and adjustment obligations exist

The x86 prepared route must consume those answers. It must not privately
reconstruct them from raw function shape.

This is not a cosmetic cleanup. It is a route correction. Without this idea,
any attempt to build generic scalar instruction selection will rest on unstable
storage and addressing assumptions, and the backend will continue to drift
toward shape-matching rather than lowering.

## Why This Idea Exists

The current repository already contains the data structures that imply a clear
ownership boundary.

`src/backend/prealloc/prealloc.hpp` defines:

- `PreparedStackObject`
- `PreparedFrameSlot`
- `PreparedStackLayout`
- `PreparedLivenessValue`
- `PreparedLivenessFunction`
- `PreparedPhysicalRegisterAssignment`
- `PreparedStackSlotAssignment`
- `PreparedMoveResolution`
- `PreparedRegalloc`
- `PreparedBirModule`

That file is not describing a tiny hint system. It is describing a handoff
model. The data model is already broad enough to say:

- prealloc knows stack objects
- prealloc knows frame slots
- prealloc knows frame-size facts
- prealloc knows value locations
- prealloc knows move obligations

`src/backend/prealloc/prealloc.cpp` then explicitly notes that prealloc owns
the route before MIR lowering. That note matters because the current public
x86 prepared emitter is effectively substituting for a missing later lowering
layer. If the prepared emitter is the current public consumer, it must consume
prepared ownership honestly rather than reopening those decisions locally.

The current x86 prepared route does not do that.

`src/backend/mir/x86/codegen/prepared_module_emit.cpp` still contains
emitter-local layout rebuilding near the top of the file:

- local `LocalSlotLayout`
- local `build_local_slot_layout`
- emitter-local stack offset derivation
- local rendering of `[rsp + N]`
- repeated frame adjustment based on the locally rebuilt size

That means the public x86 prepared route is making claims that should already
have been settled upstream.

The route is therefore split:

- upstream computes canonical storage and movement facts
- x86 prepared emission often ignores them
- x86 rebuilds enough of the same territory to make a narrow lane work

That split is exactly the architectural gap this idea owns.

## The Real Problem

The real problem is not that x86 uses the wrong helper name.

The real problem is that the x86 prepared emitter is still trying to answer
questions that are no longer target-spelling questions.

Examples:

- "Which stack slot represents this local?" is not an x86 spelling question.
- "What is the frame size?" is not an x86 spelling question.
- "Does this value have a register home or a stack home?" is not an x86
  spelling question.
- "Which copies are required before a join edge or call boundary?" is not an
  x86 spelling question.
- "How do I form the address of this local-memory object?" is usually not a
  question that should start from raw BIR again once a prepared handoff exists.

The x86-only question should be narrower:

- given canonical prepared ownership, how do I spell the legal x86 sequence?

Whenever x86 has to rediscover the ownership facts themselves, the backend has
already lost the layering battle.

## Route Context In This Repo

The current public route is not yet a traditional machine-IR pipeline.

The public backend entry still prepares semantic BIR and then hands a prepared
module to x86 emission. That means the prepared emitter is currently the public
consumer of upstream prepare results.

Because this route exists today, the prepared emitter has only two honest
choices:

1. truly consume prepared storage, frame, and move ownership
2. keep reconstructing private backend-local storage and movement logic

The second route is what causes lane inflation.

This idea exists to force the first route.

## Comparison Against The Reference Backend

The reference backend under:

- `ref/claudes-c-compiler/src/backend/generation.rs`
- `ref/claudes-c-compiler/src/backend/traits.rs`
- `ref/claudes-c-compiler/src/backend/x86/codegen/emit.rs`

does not solve the problem by pretending every IR instruction maps to one
machine instruction.

Instead, it solves the problem by introducing a disciplined shared contract.

The broad layering in the reference is:

1. shared generation dispatch
2. architecture trait contract
3. architecture-specific implementation modules

The key point for this idea is not "copy the reference verbatim."

The key point is what that reference layering refuses to let the architecture
consumer do.

The shared framework in `generation.rs` decides:

- when prologue happens
- when parameter storage happens
- when a block boundary invalidates caches
- when an instruction is dispatched
- when a terminator is dispatched
- when compare-and-branch fusion is available
- when GEP offsets can be folded into load/store addressing

The architecture layer then consumes that contract through `ArchCodegen`.

Important methods include:

- `calculate_stack_space`
- `emit_prologue`
- `emit_epilogue`
- `emit_store_params`
- `get_phys_reg_for_value`
- `emit_reg_to_reg_move`
- `emit_load`
- `emit_store`
- `emit_select`
- `emit_call`

That reference design matters here for one specific reason:

the architecture layer consumes precomputed ownership facts rather than
reconstructing them from whole-function shape.

It still performs target-specific decisions, but they are target decisions over
an agreed handoff, not whole-function reverse engineering.

The current repo gap is therefore not merely "we need more code."

The gap is:

- the reference has a stronger consumer contract
- the current x86 prepared route has a weaker and partially ignored contract

This idea does not claim that the current repo must adopt the reference file
layout or trait API literally.

It does claim that the current repo must adopt the same ownership discipline:

- shared/prepared layers own storage and move facts
- the target consumer consumes them
- target spelling is not allowed to become target-local storage analysis

## Why The Reference Matters Specifically For This Idea

Idea 59 will own generic scalar instruction selection.

That later work depends on having answers to questions like:

- when lowering a load, what address am I loading from
- when lowering a store, is the source already in a register or stack home
- when lowering a compare, where do the operands live
- when lowering a call, what moves are already mandated by upstream
- when lowering a return, which copy path is canonical

The reference backend already treats those questions as inputs to per-operation
lowering rather than as whole-function pattern deductions.

This idea therefore exists to establish the equivalent contract in this repo
before scalar lowering tries to use it.

## Explicit Scope Statement

This idea owns the prepared-handoff side of storage, frame, addressing, and
move consumption.

It is not a generic x86 codegen rewrite.

It is not a CFG semantics idea.

It is not a scalar opcode selection idea.

It is a boundary idea.

It answers:

- what storage and frame facts are canonical before x86 emission begins
- how x86 must look those facts up
- what x86 is no longer allowed to rediscover locally

## This Idea Owns

This idea owns all of the following contracts and ownership boundaries.

### A. Canonical Consumption Of Prepared Stack Layout

This idea owns the rule that stack objects and frame slots are canonical
prepared data, not emitter-local rediscovery.

That includes:

- stack-object lookup by function and source entity
- mapping from object to frame slot
- stack slot offsets
- frame size
- frame alignment

It also owns the rule that x86 prepared emission must consume those facts from
prepared data rather than synthesizing a parallel slot-layout model from raw
BIR local slots.

### B. Canonical Consumption Of Prepared Value Location

This idea owns the rule that assigned storage for prepared values is a prepared
handoff fact.

That includes:

- register assignments
- stack-slot assignments
- split status
- spill status
- rematerialization-related prepared facts where they exist

The x86 prepared route may still need target-specific spelling logic, but it
must not privately decide a replacement home model for values already owned by
prepared regalloc.

### C. Canonical Consumption Of Prepared Move Resolution

This idea owns the rule that move obligations across joins, calls, and returns
belong to prepared move resolution when that data exists.

That includes:

- join-edge copies
- call-argument moves
- call-result moves
- return moves

This idea does not own the full CFG semantics of why a move is needed. Idea 58
owns the control-flow materialization side. This idea owns the prepared
consumption boundary once move-resolution data exists.

### D. Canonical Consumption Of Frame Metadata

This idea owns the rule that frame-size and frame-alignment data are prepared
facts consumed by x86 prepared emission.

That includes:

- prologue stack adjustment size
- epilogue stack restoration size
- canonical frame alignment facts
- the distinction between frame metadata and target spelling

The target still spells `sub rsp, N`, stack probes, callee-saved saves, and
restores. But the reason `N` is what it is should not come from emitter-local
layout rediscovery.

### E. Canonical Consumption Of Address Provenance

This idea owns the boundary between semantic memory-address expressions and the
prepared facts needed by x86 to form legal stack-relative or pointer-relative
addresses.

That includes:

- what object or slot a local-memory address refers to
- whether the base is a stack object, frame slot, or pointer value
- what byte offset is canonical
- whether the route is direct stack access or indirect pointer access
- what minimum access-size and alignment assumptions are valid

This does not mean that target-specific addressing forms disappear.

It means x86 should not have to infer stack-root provenance from scratch by
inspecting raw BIR memory-address shapes that upstream has already prepared.

### F. The Negative Contract

This idea also owns the negative boundary:

x86 prepared emission must stop doing these things as if they were legitimate
sources of truth:

- rebuilding local-slot layout from raw BIR as the main frame source
- inventing a parallel stack-offset model
- inferring value homes without consulting prepared regalloc
- inventing ad hoc call or return copy obligations where prepared move data
  should be authoritative
- hiding frame/address decisions inside narrow emitter-local matcher lanes

## This Idea Does Not Own

This idea is intentionally not the owner of several nearby concerns.

### It Does Not Own CFG And Join Semantics

Idea 58 owns:

- control-flow materialization after phi elimination
- canonical representation of join semantics
- loop-carried storage/control-flow handoff
- compare/branch/join ownership as a shared contract

This idea only owns how x86 consumes the prepared move and location facts
generated around that control-flow model.

### It Does Not Own Generic Scalar Opcode Selection

Idea 59 owns:

- arithmetic opcode selection
- compare lowering policy
- select lowering policy
- scalar call-sequence selection in the sense of instruction-family choice
- legal instruction-sequence selection over known operand locations

This idea only ensures those later decisions have canonical inputs.

### It Does Not Own Permission To Add New Special Cases

This idea explicitly rejects route drift where the backend compensates for a
missing prepared handoff by adding new x86-local shape matchers or local-slot
reasoning shortcuts.

This idea is therefore not satisfied by:

- adding another emitter-local candidate layout helper
- adding more stack-address special casing around a narrow testcase family
- reintroducing value-home heuristics in x86 because a prepared consumer is not
  yet wired up

### It Does Not Own Full Machine-IR Introduction

This idea can be satisfied without inventing a whole new public machine IR,
provided the prepared contract becomes strong enough and the x86 consumer uses
it honestly.

The architectural requirement is ownership correctness, not a specific file or
IR brand name.

## Anti-Goal

The anti-goal of this idea is to let x86 prepared emission become a second
stack-layout and move-planning pass.

If future work under this idea leaves x86 still recomputing the same facts
under a different helper name, then the route has not improved.

## Current State In This Repo

The current prepared pipeline already exposes the intended ownership shape.

### Prepared Stack Layout Exists

`PreparedStackLayout` contains:

- `objects`
- `frame_slots`
- `frame_size_bytes`
- `frame_alignment_bytes`

That is already a canonical storage structure in spirit.

### Prepared Liveness Exists

`PreparedLivenessFunction` and `PreparedLivenessValue` expose:

- intervals
- definition and use points
- call points
- block boundaries
- whether values cross calls
- whether values require home slots

That is already enough to justify a downstream consumer trusting upstream
placement rather than rediscovering lifetime facts locally.

### Prepared Regalloc Exists

`PreparedRegalloc` and related records expose:

- allocation status
- physical register assignment
- stack-slot assignment
- move resolution
- spill and reload records

That means the public route already has a designed place to say where values
live and what copies must happen.

### The Prepared X86 Consumer Still Bypasses Too Much

The top of `prepared_module_emit.cpp` still constructs:

- `LocalSlotLayout`
- `build_local_slot_layout`
- emitter-local stack memory render helpers
- emitter-local frame-size computation from local slots

The same file then reuses this local layout throughout multiple narrow routes.

This is the most obvious proof that the ownership boundary is still broken.

## Evidence Of The Gap

The gap is not theoretical. It appears in multiple concrete forms.

### Gap 1: Emitter-Local Stack Layout Reconstruction

At the top of the prepared x86 emitter, raw BIR local slots are walked and
repacked into a locally computed `LocalSlotLayout`.

That local layout becomes the source for:

- offsets
- stack memory operands
- frame size
- prologue adjustments
- epilogue adjustments

That is precisely the work that `PreparedStackLayout` is supposed to make
unnecessary.

### Gap 2: Address Rendering Starts From Raw BIR Again

The emitter also renders local addresses by examining:

- `MemoryAddress`
- `base_kind`
- `base_name`
- local byte offsets

This is acceptable only if prepared handoff never claimed ownership of local
address provenance.

But this idea states that the handoff must own that provenance. If x86 keeps
deriving stack-root identity from raw BIR memory-address shapes, then stack and
address ownership remains split.

### Gap 3: Frame Adjustment Is Tied To Local Rediscovery

The emitter performs repeated `sub rsp, N` and `add rsp, N` based on the local
rebuilt layout.

That means frame sizing is still coupled to emitter-local lane logic instead of
prepared frame metadata.

### Gap 4: Value Homes Are Not The Canonical Input

Prepared regalloc can already describe where a value is supposed to live.

The prepared x86 emitter instead still hardcodes several narrow register paths
and stack-slot assumptions inside lane logic.

That means later generic lowering would still not have one canonical place to
ask:

- where is operand A
- where is operand B
- where should result C go

### Gap 5: Move Plans Are Not Yet The Main Copy Source

Prepared move-resolution data exists specifically to encode movement that is
required by control-flow joins and ABI boundaries.

If x86 still invents call, join, or return movement locally, then there are
two competing truths:

- upstream planned moves
- backend ad hoc copies

That is not a stable handoff.

## Why This Is A Shared Capability Idea Rather Than An X86 Cleanup Note

The temptation is to describe this as:

"make x86 cleaner."

That description is too weak and too local.

This is a shared capability idea because it clarifies the contract between:

- BIR memory and address modeling
- prealloc stack-layout ownership
- prealloc liveness ownership
- prealloc regalloc ownership
- prepared move planning
- x86 prepared emission

Even if the first consumer is x86, the durable source intent is broader:

once a prepared contract exists, targets should consume that contract rather
than duplicating ownership reasoning.

## Design Principle

Prepared data is not debug output.

Prepared data is not best-effort annotation.

Prepared data is the canonical ownership boundary for the public prepared route.

Every detailed requirement in this idea follows from that principle.

## Durable Design Questions This Idea Must Answer

The source idea must answer all of these questions durably, even if packet
ordering is deferred to a later runbook.

### Question 1

What is the canonical object that x86 queries to learn the frame shape of a
function in the prepared route?

### Question 2

What is the canonical object that x86 queries to learn where a prepared value
lives at use time?

### Question 3

What is the canonical object that x86 queries to learn which stack slot
represents a stack object or home slot?

### Question 4

What is the canonical object that x86 queries to learn which copies are
required at joins, calls, and returns?

### Question 5

How is local-memory address provenance represented so that x86 does not need to
reconstruct the stack root from raw BIR memory syntax?

### Question 6

What is the exact boundary between upstream ownership and x86 spelling when a
target-specific decision is still required?

### Question 7

What forms of emitter-local fallback are explicitly forbidden because they
recreate split ownership?

## Intended End State

When this idea is satisfied, a maintainer inspecting the x86 prepared route
should see a much narrower set of responsibilities.

The prepared x86 consumer should primarily do these things:

- consult prepared frame metadata
- consult prepared value-location data
- consult prepared move-resolution data
- consult canonical prepared address provenance
- emit x86 instructions and data directives accordingly

The prepared x86 consumer should not primarily do these things:

- repack local slots
- infer canonical object roots from raw syntax
- decide where values live by local heuristics
- recalculate frame size from local slot enumeration
- decide movement obligations that already belong to prepared planning

## Detailed Ownership Boundary

This section states the durable contract more concretely.

### Ownership Boundary 1: Stack Objects

Upstream ownership:

- identify stack-worthy objects
- record object ids
- record source names and source kinds
- record size and alignment
- record whether addresses escape
- record whether a home slot is required

X86 ownership:

- use that information to form legal x86 accesses
- use that information to select the right operand spelling

X86 non-ownership:

- deciding whether the object exists
- deciding whether the object needs a home slot
- deciding how object identity is reconstructed from local-slot names if
  prepared objects already answer that question

### Ownership Boundary 2: Frame Slots

Upstream ownership:

- assign frame slots to prepared objects
- compute offsets
- compute size and alignment
- compute frame-size aggregate facts

X86 ownership:

- turn slot plus offset facts into legal x86 memory operands
- incorporate target prologue and epilogue conventions

X86 non-ownership:

- building a parallel frame-slot table from BIR local slots
- privately repacking or renumbering slots
- redefining frame size

### Ownership Boundary 3: Value Locations

Upstream ownership:

- determine which values are register-assigned
- determine which values are stack-assigned
- expose location status
- expose allocation class

X86 ownership:

- use canonical location queries during lowering
- emit legal transfer or compute sequences

X86 non-ownership:

- inventing a separate home model because a lane happens to prefer `eax`
- ignoring prepared location facts and treating them as optional

### Ownership Boundary 4: Move Resolution

Upstream ownership:

- identify required copies before and after joins
- identify required copies for call boundaries
- identify required copies for returns
- expose source storage and destination storage

X86 ownership:

- materialize the move plan with legal x86 instructions

X86 non-ownership:

- deciding ad hoc that a join/call/return copy should happen differently when
  the prepared route already says otherwise

### Ownership Boundary 5: Address Provenance

Upstream ownership:

- define what the address base means
- define whether the base is local stack, frame slot, global, or pointer value
- define canonical byte offset relative to that base

X86 ownership:

- choose legal x86 addressing form
- choose register materialization sequence if direct addressing is not possible

X86 non-ownership:

- reconstructing canonical stack provenance from raw BIR naming conventions
- inferring aggregate-root identity by emitter-local tricks

## Relationship To Idea 58

Idea 58 owns the shared control-flow side of the problem.

This idea intentionally assumes that values crossing joins or loop boundaries
will eventually be described by a stable shared materialization contract.

This idea then owns the prepared storage side of consuming that materialization.

The distinction matters:

- Idea 58 answers "what movement or join semantics exist."
- Idea 60 answers "once that movement is planned upstream, where is it stored
  and how is it consumed as canonical prepared data."

Without idea 58, join and loop semantics are unstable.

Without this idea, even a good join model still gets reinterpreted locally by
x86.

Both are needed.

## Relationship To Idea 59

Idea 59 depends on this idea because generic scalar instruction selection is
not possible in a stable way if value locations and addresses are not already
canonical.

To lower a scalar add, compare, load, store, select, or call correctly, the
selector needs stable inputs such as:

- source location
- destination location
- frame operand form
- address base kind
- required copies around ABI boundaries

If those are not canonical, scalar lowering degenerates into either:

- more x86-local heuristics
- more whole-shape matchers

That is exactly what the source-idea set is trying to stop.

## What The Reference Backend Teaches

This section captures the design lessons from the reference without requiring a
verbatim port.

### Lesson 1: Shared Dispatch Narrows Architecture Responsibility

In the reference, `generation.rs` is the place where function generation,
block iteration, instruction dispatch, and several optimization-level folds are
driven.

This means x86 is not asked to rediscover:

- whether a block is the entry block
- whether a compare can fuse with a branch
- whether a GEP offset can fold into a memory access

The architecture consumer receives a narrower and more honest job.

This repo's prepared route may not use the exact same architecture, but this
idea adopts the same discipline:

upstream and shared prepared ownership should shrink, not expand, what x86 must
figure out locally.

### Lesson 2: Value Location Queries Are First-Class

The reference `ArchCodegen` trait includes `get_phys_reg_for_value`, plus copy
helpers that treat register allocation as a first-class input to codegen.

That means copy emission is not based on a narrow lane guessing where values
live. It asks a canonical location source.

This repo already has prepared regalloc data structures capable of enabling the
same principle. This idea exists to make that principle true for the prepared
x86 route.

### Lesson 3: Frame Ownership Is Queried, Not Rebuilt

The reference path computes stack space before emission and then consumes it in
prologue, parameter storage, and later accesses.

This is the conceptual model this idea wants:

- frame layout is prepared once
- x86 consumes it
- x86 does not secretly build a second layout as the real truth

### Lesson 4: Address Folding Still Respects Ownership

The reference can fold GEP offsets into load/store addressing, but it does so
within a shared framework that already knows what the address base means.

That distinction matters:

address optimization is fine; address provenance duplication is not.

### Lesson 5: Calls Consume A Structured Contract

The reference call path is not just "emit call".

It consumes structured ABI and argument-placement facts to emit a legal call
sequence.

This idea applies the same lesson to this repo:

when prepared move resolution and value-location data exist, the x86 prepared
consumer should consume them as the structured call-boundary contract rather
than rebuilding mini ABI plans inside lanes.

## Required Source-Idea Outcomes

This idea is satisfied only if the source tree reaches outcomes that preserve
these durable statements.

### Outcome 1

A maintainer can point to one canonical prepared query path for:

- frame size
- frame alignment
- frame-slot offset
- value location
- move resolution

### Outcome 2

`prepared_module_emit.cpp`, or whatever later replaces the prepared x86
consumer, no longer needs to define an emitter-local `LocalSlotLayout` as the
real frame source.

### Outcome 3

Local-memory lowering no longer depends on x86 privately inferring local-root
ownership from raw BIR memory syntax where prepared ownership should have
already resolved the root.

### Outcome 4

At least one non-trivial join/call/return movement family is demonstrably
driven by prepared move data rather than backend-local copy invention.

### Outcome 5

Frame entry and exit behavior in the prepared route is driven by prepared frame
facts, with x86 only responsible for target spelling and target ABI details.

## Concrete Contract Areas

The rest of this idea enumerates the contract areas in more detail.

## Contract Area A: Prepared Stack Objects

Prepared stack objects must be the canonical identity for things that own
storage in the frame.

The object identity should survive independently of emitter-local naming tricks
or local-slot enumeration order.

Minimum durable expectations:

- object identity is stable enough to be consumed downstream
- object records expose source-facing meaning
- object records expose size and alignment
- object records expose whether the address escapes
- object records expose whether a permanent or required home slot exists

Why this matters:

if x86 still has to infer object identity from local-slot names or raw BIR
shapes, then stack storage ownership has not actually been handed off.

## Contract Area B: Prepared Frame Slots

Prepared frame slots must be the canonical answer to:

- which object occupies a given slot
- what the slot offset is
- what the slot size is
- what the slot alignment is

Durable requirement:

x86 may translate a slot to a machine operand, but it may not replace the slot
assignment system with a second local one.

## Contract Area C: Prepared Value Identity

Prepared values must be the canonical identity for movable results that later
location queries reference.

That means at minimum:

- the consumer can identify the prepared value corresponding to a BIR-level use
- the consumer can query location and movement data from that identity

If x86 cannot reliably map a use site to prepared value identity, then all
later location consumption claims will remain partial.

This idea therefore implicitly requires enough binding between prepared value
records and x86 consumption sites for canonical lookup to work.

## Contract Area D: Prepared Allocation Status

A downstream prepared consumer must be able to tell whether a value is:

- unallocated
- assigned a register
- assigned a stack slot
- split
- spilled

This is not only debug metadata.

It changes lowering legality.

Example:

- if a value is assigned a register, direct register use may be legal
- if a value is assigned a stack slot, memory-operand formation may be legal
- if a value is split or spilled, the move or reload plan may matter

The durable requirement is that these statuses become live consumer inputs, not
dead report fields.

## Contract Area E: Register Assignment Queries

The consumer must have a canonical way to ask:

- is this prepared value assigned a physical register
- if so, which one

This does not force one exact API shape.

It does force one exact ownership rule:

x86 must not guess.

## Contract Area F: Stack-Slot Assignment Queries

The consumer must have a canonical way to ask:

- does this value have a prepared stack-slot assignment
- if so, which slot and offset

Again, the durable rule is that the answer comes from prepared data, not from a
local x86 reconstruction of stack homes.

## Contract Area G: Move-Resolution Queries

The consumer must have a canonical way to ask:

- what move sequence is required before entering this join
- what move sequence is required before or after this call
- what move sequence is required for this return path

This idea does not require that all move families be solved at once.

It does require that once a move family is represented in prepared data, x86
consumes that representation instead of creating a competing rule set.

## Contract Area H: Address Provenance Queries

The consumer must have a canonical way to ask:

- what is the base object of this address
- is the base a stack object, frame slot, global, or pointer value
- what byte offset is already canonical
- when is direct stack addressing permitted
- when must a pointer register be materialized

This is the contract area that most directly prevents x86-local memory
special-casing from growing back.

## Contract Area I: Prologue And Epilogue Inputs

The consumer must have canonical prepared inputs for:

- frame-size bytes
- frame-alignment bytes
- saved-register slot relationships where applicable
- any other shared frame-shape facts required by the prepared route

X86 still owns:

- actual emitted instructions
- stack probes
- callee-saved save/restore spelling
- target-specific prologue policy

But it should not own recomputing the frame amount from local-slot enumeration.

## Contract Area J: Parameter Home And Entry Storage

The reference backend explicitly stores parameters into stack homes using a
shared contract after prologue.

This repo does not need to mimic the exact flow, but the same ownership lesson
applies.

If prepared data says parameters or entry values require a home slot, then x86
should consume that canonical storage plan rather than inventing lane-local
entry handling.

## Non-Goals

This idea is not trying to solve every backend problem.

The following are explicit non-goals.

### Non-Goal 1

It does not promise complete x86 backend completeness.

### Non-Goal 2

It does not promise to eliminate every special case in one slice.

### Non-Goal 3

It does not define the final scalar lowering strategy for all arithmetic,
compare, or select operations.

### Non-Goal 4

It does not require literal adoption of the reference backend trait names.

### Non-Goal 5

It does not require introducing a brand-new public machine IR if the existing
prepared route can be made to honor the ownership contract honestly.

### Non-Goal 6

It does not treat expectation rewrites or testcase downgrades as valid progress.

### Non-Goal 7

It does not allow x86-local lane growth as a substitute for prepared ownership.

## Explicit Rejection Of Bad Fixes

The following are route failures under this idea.

### Rejected Fix A

Replace `build_local_slot_layout` with a differently named helper that still
recomputes the frame from raw local slots.

### Rejected Fix B

Keep prepared frame data but let x86 continue to prefer its local layout as the
real source when the two disagree.

### Rejected Fix C

Handle one failing memory testcase by adding a new x86-local matcher for that
exact address shape instead of extending canonical address provenance.

### Rejected Fix D

Introduce partial prepared value-location lookup but continue routing key call
or return movements through separate backend-local rules.

### Rejected Fix E

Claim completion because a few backend tests pass while the prepared emitter is
still plainly recalculating frame and slot ownership.

## Detailed Repo Gap Against The Reference

This section compares the current repo more directly against the reference
layering because the difference matters for future planning.

### Reference Side

The reference effectively has:

- shared generation that sequences function emission
- explicit stack-space calculation before code emission
- explicit prologue emission with known frame size
- explicit parameter-store phase
- per-instruction dispatch
- value-location-aware copy helpers
- arch-specific lowering modules

### Current Repo Side

The current prepared x86 route effectively has:

- public handoff at `PreparedBirModule`
- prepared data structures rich enough to describe storage
- a prepared emitter that still reconstructs storage in places
- multiple x86-local capability lanes that encode their own frame and location
  assumptions

### Architectural Difference

The reference says:

- architecture code consumes prepared or shared storage facts

The current route often says:

- architecture code rediscovers storage facts when needed

This idea is the durable source decision to remove that difference.

## Decomposition Of The Problem

This section is still source-level, not runbook-level. It exists so future plan
generation has a concrete decomposition to draw from.

### Decomposition Theme 1: Frame Truth

Subproblem:

- make prepared frame metadata the only source of frame truth in the prepared
  route

Key questions:

- how does x86 query frame-size bytes
- how does x86 query frame-alignment bytes
- how does x86 query slot offsets
- how are local ad hoc frame computations removed

Failure mode to avoid:

- dual frame truths where prepared data exists but x86 still computes a second
  one

### Decomposition Theme 2: Value-Location Truth

Subproblem:

- make prepared allocation results the canonical answer to where a value lives

Key questions:

- how does x86 look up the prepared value identity for a use site
- how does x86 query register assignment
- how does x86 query stack-slot assignment
- how are stack-only and register-only special lanes retired from owning the
  truth

Failure mode to avoid:

- prepared location data exists but x86 only consults it in cosmetic or narrow
  paths

### Decomposition Theme 3: Address-Provenance Truth

Subproblem:

- make prepared address facts the canonical answer for local-memory addressing

Key questions:

- what record identifies the storage root
- how is base kind represented
- how are byte offsets carried
- how do aggregate fragments and local subobjects map onto the canonical root

Failure mode to avoid:

- x86 still has to reverse engineer stack roots from `MemoryAddress::LocalSlot`
  naming and raw byte offsets

### Decomposition Theme 4: Move-Plan Truth

Subproblem:

- make prepared move resolution the canonical source for boundary copies

Key questions:

- what is the first family to wire through completely
- how are join moves queried
- how are call moves queried
- how are return moves queried
- how does x86 spell those moves without redefining their meaning

Failure mode to avoid:

- upstream and x86 both claim authority over the same boundary-copy problem

### Decomposition Theme 5: Prepared Consumer API Shape

Subproblem:

- make the consumer-side lookup path explicit and testable

Key questions:

- is a direct query API needed
- is data currently discoverable enough through `PreparedBirModule`
- what helper layer belongs in `prepare` versus `x86`

Failure mode to avoid:

- leaving the contract implicit so future work slips back into raw-structure
  spelunking and local heuristics

## First-Order Implementation Questions

These are durable source questions, not packet instructions.

### Question Set 1: Mapping BIR Uses To Prepared Values

How does x86 map a specific consumer operand or result site to the corresponding
prepared value record?

The answer must be strong enough that:

- generic lowering can query value locations
- move resolution can target the right values
- tests can verify canonical lookup behavior

### Question Set 2: Canonical Address Rooting

What exact upstream representation lets x86 learn:

- this address refers to stack object X
- with frame slot Y
- at byte offset Z

without rediscovering that chain itself?

### Question Set 3: Home Slots Versus Temporary Slots

How are values that require home slots distinguished from values that merely
happen to spill?

Why this matters:

- x86 addressing legality
- ABI entry and exit behavior
- correctness of aggregate/local-memory references

### Question Set 4: Call Boundary Consumption

When prepared move resolution already computes call-boundary moves, what is the
consumer protocol for x86 to materialize them?

The answer must separate:

- planned movement
- target spelling

### Question Set 5: Return Boundary Consumption

When a return path requires moving a value into a return register or home
location, which part is prepared truth and which part is x86-specific spelling?

### Question Set 6: Join Boundary Consumption

Once idea 58 provides the control-flow side, how are join-edge move plans read
by x86 without the emitter having to match narrow join shapes?

## Example Failures This Idea Should Prevent

These are not acceptance tests by themselves. They are route examples.

### Failure Example 1

A new local-memory testcase fails because x86 cannot recognize the exact stack
shape and a patch adds another lane that re-derives local offsets from local
slot names.

This idea says that is the wrong fix.

### Failure Example 2

A call-related testcase fails and x86 adds a direct call lane that hand-places
arguments without consulting prepared move and value-location facts.

This idea says that is the wrong fix.

### Failure Example 3

A return path works only when the value is already in `eax`, so x86 adds a
special return lane that assumes the value home locally.

This idea says that is the wrong fix.

### Failure Example 4

A future scalar selector tries to lower loads and stores directly but still has
to call emitter-local stack-root inference helpers because the address
provenance contract never became canonical.

This idea says the prerequisite was not met.

## Required Tests And Evidence Areas

This section is still durable and intentionally high level. It names evidence
areas that future plans should cover.

### Evidence Area A: Prepared Stack Layout Tests

The repo should have evidence that prepared stack layout remains stable and
queryable for downstream consumption.

Relevant areas include:

- `tests/backend/backend_prepare_stack_layout_test.cpp`

### Evidence Area B: Prepared Liveness Tests

The repo should have evidence that prepared liveness and value identity are
still sufficient for downstream location reasoning.

Relevant areas include:

- `tests/backend/backend_prepare_liveness_test.cpp`

### Evidence Area C: Prepared Handoff Boundary Tests

The repo should have evidence that x86 consumes canonical prepared frame and
location facts rather than reconstructing them.

Relevant areas include:

- `tests/backend/backend_x86_handoff_boundary_test.cpp`

### Evidence Area D: Local-Memory Route Tests

Because local-memory support is a key symptom area, later proof should include
coverage showing that local-memory load/store routes are repaired through
canonical handoff, not matcher expansion.

Relevant upstream symptom files include:

- `src/backend/bir/lir_to_bir_memory.cpp`

### Evidence Area E: Call And Return Boundary Tests

Once move consumption is wired in, proof should include at least one non-trivial
call or return movement family using prepared move data as the source of truth.

## Validation Intent

This idea is lifecycle-independent, but its future runbooks should aim for
validation that proves ownership transfer, not only narrow asm coincidence.

Minimum expected proof categories:

- build succeeds
- prepared stack-layout tests remain green
- prepared liveness tests remain green
- prepared handoff boundary tests cover canonical x86 consumption

Illustrative narrow commands:

- `cmake --build --preset default`
- `ctest --test-dir build -R '^(backend_prepare_stack_layout|backend_prepare_liveness|backend_x86_handoff_boundary(_test)?)$' --output-on-failure`

Escalation examples when the contract blast radius expands:

- `ctest --test-dir build -j --output-on-failure -R '^backend_'`

The exact proving commands belong in later plan work, not this source idea.

## Acceptance Criteria

This idea is complete only when all of the following are true in substance.

- [ ] The prepared x86 route no longer rebuilds local stack layout as the real
      source of frame truth.
- [ ] Frame size and frame alignment consumed by x86 prepared emission come from
      canonical prepared metadata.
- [ ] X86 can query canonical prepared location information for at least the
      value families needed by the prepared public route.
- [ ] X86 can query canonical prepared stack-slot information instead of
      reconstructing stack homes locally.
- [ ] X86 can form local stack accesses from canonical prepared address
      provenance rather than rediscovering roots from raw memory syntax.
- [ ] At least one non-trivial move-resolution family is consumed directly from
      prepared move data.
- [ ] The ownership line between `prepare` and x86 is documented in code and
      test evidence strongly enough that future scalar lowering can rely on it.
- [ ] No accepted progress under this idea depends mainly on adding new x86
      shape matchers or expectation downgrades.

## Completion Smell Test

Even if the formal acceptance boxes look green, the route is not complete if a
reviewer can still truthfully say:

- x86 has a secret second frame-layout system
- x86 still guesses where values live
- x86 still invents boundary copies that prepared data should own
- x86 still infers stack-root identity from raw syntax in ordinary cases

## Migration Guidance For Future Plans

This is not a runbook, but the durable source idea should still preserve the
right migration direction.

Future plans derived from this idea should prefer:

- making one canonical prepared lookup path real
- deleting one backend-local truth source
- proving the new ownership line with boundary tests

Future plans derived from this idea should avoid:

- broad rewrites with no ownership checkpoint
- mixing scalar opcode selection into the same packet before location and frame
  truth exist
- claiming local-memory progress through x86 matcher growth

## Open Questions

These remain intentionally open at source-idea level.

### Open Question 1

Does prepared address provenance need new explicit records, or can existing
`PreparedStackLayout` plus BIR memory records be bound together with a clear
consumer contract?

### Open Question 2

How much of current prepared move-resolution data is already sufficient for x86
consumption, and where are the missing fields if any?

### Open Question 3

What is the smallest honest first consumer of prepared value locations:

- loads and stores
- call arguments
- call results
- returns
- join copies

### Open Question 4

Should the prepared consumer API live as helper functions under `prepare`, or
is direct structured access to `PreparedBirModule` sufficient once the data
model is clarified?

### Open Question 5

What test additions best prove that x86 is consuming canonical prepared truth
rather than simply arriving at the same asm through a second private analysis?

## Risks

### Risk 1: Partial Adoption Risk

The route may adopt prepared location queries for some easy value families but
leave the old local layout logic in place for the hard families. That would
produce a mixed-truth system.

### Risk 2: Shape-Regression Masking

Some current tests may still pass because lane-local recomputation produces the
same answer as prepared data on simple examples. That can hide ownership drift.

### Risk 3: Naming-Only Cleanup Risk

A refactor may rename helpers and move code around without actually deleting the
split ownership.

### Risk 4: Premature Scalar Lowering

If idea 59 starts consuming unstable location/address facts too early, the repo
will encode new selector logic around unstable inputs and recreate the same debt
at a different layer.

### Risk 5: Overfitting Via Boundary Tests

A narrow handoff boundary test might be updated to match a current local x86
lane rather than proving the ownership contract. That would be route drift.

## Review Guidance

Reviewers evaluating future work under this idea should ask:

- did this patch delete or merely relocate local frame reconstruction
- did x86 gain a canonical prepared query path or just another fallback
- did move consumption become more upstream-driven or remain backend-local
- did local-memory addressing become more canonical or more matcher-shaped
- does the proof show contract repair or only testcase coincidence

## Files Likely To Matter

This source idea is not a runbook, but these files are likely to remain central
to the ownership seam:

- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/stack_layout.cpp`
- `src/backend/prealloc/stack_layout/slot_assignment.cpp`
- `src/backend/prealloc/liveness.cpp`
- `src/backend/prealloc/regalloc.cpp`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir.hpp`
- `src/backend/bir/lir_to_bir_memory.cpp`
- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `tests/backend/backend_prepare_stack_layout_test.cpp`
- `tests/backend/backend_prepare_liveness_test.cpp`
- `tests/backend/backend_x86_handoff_boundary_test.cpp`

Reference comparison files that informed this idea:

- `ref/claudes-c-compiler/src/backend/generation.rs`
- `ref/claudes-c-compiler/src/backend/traits.rs`
- `ref/claudes-c-compiler/src/backend/x86/codegen/emit.rs`

## Durable Conclusion

The durable conclusion of this idea is simple, even if the implementation work
is not.

The x86 prepared route must stop acting like the final owner of:

- frame truth
- value-home truth
- stack-root truth
- boundary-copy truth

Those truths belong to the prepared handoff.

X86 should spell them, not rediscover them.

That is the contract this idea exists to make durable.
