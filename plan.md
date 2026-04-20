# Generic Scalar Instruction Selection For Prepared X86

Status: Active
Source Idea: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Activated from: ideas/closed/61_stack_frame_and_addressing_consumption.md

## Purpose

Turn idea 59 into an execution runbook that replaces prepared-x86 whole-function
matcher growth with ordinary per-operation and per-terminator instruction
selection over authoritative prepared inputs.

## Goal

Make x86 scalar lowering dispatch on prepared operations, operands, and
terminators so legality decisions are local and machine-shaped instead of being
encoded as bounded whole-function matcher families.

## Core Rule

Do not add new whole-function or named-testcase x86 matcher families. Repair
the lowering structure by consuming prepared CFG, value-home, move-bundle, and
addressing facts through ordinary selector helpers and per-op dispatch.

## Read First

- `ideas/open/59_generic_scalar_instruction_selection_for_x86.md`
- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- `src/backend/prealloc/prealloc.hpp`

## Scope

- define an x86 prepared-function/block context that exposes authoritative
  prepared inputs to scalar lowering
- extract legality-oriented selector helpers for scalar values, memory
  operands, compares, calls, and terminators
- migrate covered scalar operations and terminators onto per-op dispatch
- prove the route with build plus narrow backend/x86 coverage, then broaden
  when the dispatch blast radius grows

## Non-Goals

- introducing a separate machine IR in the same slice
- x86 peephole optimization
- target-independent SSA rewrites
- reopening prepared CFG ownership from idea 58, prepared value-home ownership
  from idea 60, or frame/address ownership from idea 61

## Working Model

- upstream prepared ownership already publishes CFG meaning, value homes,
  move obligations, and frame/address facts for the covered route
- x86 instruction selection should ask prepared data what an operation means,
  then choose a legal machine spelling
- selector helpers should answer operand and legality questions, not recover
  whole-program semantics from function shape
- routine execution should prefer small migrations of coherent instruction or
  terminator families over broad emitter rewrites

## Execution Rules

- prefer extracted selector helpers and per-op dispatch over growth in `try_*`
  whole-function matcher families
- keep legality decisions target-local and semantic ownership upstream
- update `todo.md`, not this file, for routine packet progress
- require `build -> narrow proof` for each code slice
- escalate to broader backend validation when a slice changes shared selector
  helpers or several narrow instruction-family packets have landed

## Step 1: Establish Prepared Dispatch Surface

Goal: define the x86 prepared-function and block context plus the top-level
dispatch boundaries needed for ordinary scalar lowering.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`

Actions:

- execute this step through the ordered substeps below rather than treating all
  prepared-dispatch cleanup as one undifferentiated packet stream
- keep this step structural; do not widen it into full family migration yet
- use the Step 1.3 audit to decide whether one last context-only seam remains
  or whether the next packet should move into Step 2 selector extraction

Completion check:

- Step 1.1 through Step 1.3 are complete
- the x86 prepared route has a visible dispatch surface that later packets can
  target without first re-deriving whole-function matcher structure

### Step 1.1: Establish Shared Function Dispatch Context

Goal: replace repeated raw prepared-x86 argument bundles with one
authoritative function-dispatch context.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`

Actions:

- identify or extract one authoritative prepared-function context surface that
  can hand scalar lowering the current function, block, control-flow,
  value-home, move-bundle, and addressing views
- make the top-level prepared entry route consume that shared context instead
  of expanding the same function-wide inputs through long raw helper argument
  lists
- keep packet-specific closures or emit sinks explicit at the call sites that
  actually own them

Completion check:

- the prepared x86 route has one visible function-dispatch context surface
  that later packets can target without re-deriving shared prepared inputs

### Step 1.2: Route Structural Entry And Return Helpers Through The Context

Goal: move the current structural entry and return helper families onto the
shared dispatch context without widening into selector work.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`

Actions:

- route the existing compare-driven entry, single-block return, and adjacent
  structural helper families through the shared function-dispatch context
- collapse remaining raw helper signatures for the covered structural route
  when the only repeated inputs are shared prepared function facts
- keep this substep limited to context plumbing and dispatch cleanup rather
  than per-op instruction-family migration

Completion check:

- the covered structural entry and return helpers consume the shared
  function-dispatch context instead of parallel raw argument bundles

### Step 1.3: Audit Remaining Dispatch Seams And Hand Off To Selector Work

Goal: confirm whether any last Step 1 structural seam still needs context-only
cleanup before Step 2 selector extraction begins.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`

Actions:

- inspect the remaining prepared-x86 scalar path for any function-wide raw
  argument unwrapping that would force one more context-only packet
- if one real seam remains, collapse it without widening into selector or
  family migration work
- otherwise treat Step 1 as structurally exhausted and move the next packet to
  Step 2 helper extraction

Completion check:

- either the last structural seam is removed or the route is explicitly ready
  for Step 2 selector extraction without another Step 1 rewrite

## Step 2: Extract Operand And Legality Selectors

Goal: provide reusable selector helpers for the prepared scalar route.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- helper extraction sites under x86 codegen if needed

Actions:

- execute this step through the ordered substeps below rather than treating
  all remaining selector work as one undifferentiated stream
- extract or refine selector helpers for named/immediate scalar values, stack
  and symbol memory operands, prepared compare planning, and bounded call-lane
  legality
- keep helper contracts framed around prepared inputs and x86 legality, not
  testcase-shaped whole-function success conditions
- preserve upstream ownership boundaries: do not re-derive CFG meaning, value
  homes, or frame/address provenance locally

Completion check:

- Step 2.1 through Step 2.3 are complete
- covered scalar lowering questions can be answered through local selector
  helpers instead of by growing another bounded whole-function matcher

### Step 2.1: Exhaust The Remaining Local Guard Selector Seams

Goal: finish the local guard-path selector audit so any remaining cleanup is a
real reusable legality surface rather than cosmetic refactoring.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`

Actions:

- inspect the nearby local `i32` guard operand/value rendering recursion and
  extract it only if the result is a reusable selector helper with a clear
  prepared-legality contract
- keep the local `i16` and `i32` arithmetic guard routes aligned when a helper
  is truly shared, but do not widen into family migration or broad emitter
  cleanup
- if no real selector seam remains, treat the local guard audit as exhausted
  and hand off to the next Step 2 substep instead of forcing one more refactor

Completion check:

- the remaining local guard rendering logic is either extracted into a real
  selector helper or explicitly judged structurally exhausted

### Step 2.2: Consolidate Shared Scalar Operand And Compare Selectors

Goal: finish the reusable selector surfaces for scalar values, memory operands,
  and compare planning that other prepared-x86 lowering packets should consume.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- helper extraction sites under x86 codegen if needed

Actions:

- consolidate selector entry points for named/immediate scalar operands, stack
  and symbol memory operands, and prepared compare planning where nearby
  lowering sites still assemble those legality answers ad hoc
- keep helper contracts centered on prepared inputs and x86 legality decisions,
  not on specific testcase shapes or one whole-function success path
- stop when nearby scalar lowering callers can ask selector helpers for operand
  and compare decisions without re-deriving local prepared facts inline

Completion check:

- the remaining covered scalar operand and compare questions route through
  shared selector helpers instead of local ad hoc scans

### Step 2.3: Extract Bounded Call-Lane Legality Selectors

Goal: isolate the bounded scalar-call legality surface needed before Step 3
  family migration starts.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- adjacent x86 helper extraction sites if needed

Actions:

- extract or refine the bounded prepared call-lane legality helpers required by
  the currently covered scalar call route
- keep the route framed as selector-based legality answers over prepared
  move-bundle and operand facts rather than matcher growth
- stop once the remaining Step 2 blocker for scalar-call routing is a clear
  helper surface that Step 4 can consume

Completion check:

- the covered scalar-call legality questions are answered by selector helpers
  instead of open-coded call-lane checks

## Step 3: Migrate Covered Scalar Instruction Families

Goal: move the covered scalar instruction families onto per-operation
instruction selection.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- focused backend/x86 proof coverage matched to each migrated family

Actions:

- migrate binary, load/store, select, and related covered scalar instruction
  families through per-op dispatch and selector helpers
- keep each packet bounded to one coherent family or adjacent helper surface
- delete or isolate matcher-only branches only when the replacement path is
  already driven by prepared per-op dispatch

Completion check:

- Step 3.1 through Step 3.4 are complete
- the covered scalar instruction families are emitted through ordinary
  instruction selection over prepared inputs, not through whole-function shape
  recognition

### Step 3.1: Migrate Core Binary And Cast Families

Goal: move the already-covered scalar binary and adjacent cast families onto
named per-inst dispatch surfaces.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`

Actions:

- route the covered local scalar binary lanes through explicit per-inst helpers
  instead of open-coded family-shaped rendering paths
- keep any adjacent cast-family extraction limited to real per-op dispatch
  reuse that shares the same prepared selector contracts
- stop once the covered binary and cast route is expressed as instruction
  family dispatch rather than inline matcher-shaped control flow

Completion check:

- the covered scalar binary and adjacent cast families lower through named
  per-inst dispatch helpers over prepared inputs

### Step 3.2: Consolidate Local And Same-Module Load/Store Families

Goal: move the covered scalar load/store families onto shared per-op helpers
without reopening unrelated memory ownership.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`

Actions:

- consolidate the covered scalar load/store renderers so local and same-module
  memory families reuse shared operand-selection and emission helpers
- keep the route bounded to scalar load/store legality and spelling, not broad
  emitter cleanup or terminator migration
- stop once the covered load/store families no longer depend on repeated
  inline family-specific rendering branches

Completion check:

- the covered local and same-module scalar load/store families route through
  shared per-op helpers instead of duplicated family-specific branches

### Step 3.3: Isolate The Bounded Helper-Function Instruction Lane

Goal: collapse the remaining bounded helper-function scalar instruction lane
onto one explicit per-inst dispatch surface.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`

Actions:

- extract the covered helper-function scalar instruction lane into a named
  dispatcher when that lane still open-codes already-covered family handling
- keep this substep bounded to the helper-function lane and the scalar
  families it already covers; do not widen into call-lane or terminator work
- stop once helper-lane scalar instruction selection uses one local per-inst
  dispatch surface instead of inlined family loops

Completion check:

- the bounded helper-function scalar lane is driven by a named per-inst
  dispatcher over the already-covered families

### Step 3.4: Audit Remaining Scalar Family Seams And Hand Off

Goal: determine whether any real covered scalar instruction-family seam still
remains before Step 4 call and terminator migration begins.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`

Actions:

- inspect the remaining prepared-x86 scalar route for any uncovered but
  already-supported instruction family that still lacks ordinary per-op
  dispatch
- if one real family seam remains, keep the next packet bounded to that
  family and land it without widening into call or terminator migration
- otherwise declare Step 3 structurally exhausted and move the next packet to
  Step 4 instead of forcing another helper-only refactor

Completion check:

- either one final real scalar instruction family is identified for a bounded
  Step 3 packet or the runbook explicitly hands off the next work to Step 4

## Step 4: Migrate Covered Terminator And Call Families

Goal: move the covered branch and call emission paths onto prepared
per-terminator and per-call selection.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`

Actions:

- migrate covered compare-and-branch, boolean-branch, and scalar call families
  onto prepared control-flow, move-bundle, and operand selectors
- keep unsupported families explicitly unsupported rather than adding new
  matcher-shaped acceptance paths
- stop when the remaining x86-only decisions are machine legality and spelling,
  not whole-function semantic recovery

Completion check:

- the covered branch and call routes dispatch through prepared per-terminator
  or per-call selection instead of bounded whole-function x86 matchers

## Step 5: Validate The Route

Goal: prove the new scalar instruction-selection structure without relying on
one named testcase.

Actions:

- require a fresh build for every accepted slice
- choose the narrowest proving subset that exercises the migrated instruction
  or terminator family
- broaden validation when shared selector helpers or dispatch boundaries change
  across multiple backend buckets, or when the route is being treated as a
  milestone
- reject slices whose practical effect is matcher growth or expectation
  weakening rather than selector-based capability repair

Completion check:

- accepted slices have fresh proof logs and validation proportional to the
  scalar-lowering blast radius
