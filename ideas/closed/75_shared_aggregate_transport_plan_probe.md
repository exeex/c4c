# Shared Aggregate Transport Plan Probe

## Goal

Determine whether aggregate copy chunk/lane planning and transport
decomposition need a new shared prepared aggregate-transport plan, then draft
that shared fact/query shape before any AArch64 implementation migration.

## Why This Exists

The large-owner residue audit classified aggregate copy chunk/lane planning
and transport decomposition across AArch64 instruction and memory owners as
`missing-shared-authority`. The missing shape is a prepared
aggregate-transport plan that names chunks, lanes, source/destination homes,
scratch needs, and whether the transport is stack-backed, register-backed, or
mixed. This must be probed and specified before moving target code.

## Owned Files

- Audit/proposal work in `todo.md` or a later activated plan.
- Candidate implementation files only after the probe proves the contract:
  - `src/backend/mir/aarch64/codegen/instruction.cpp`
  - `src/backend/mir/aarch64/codegen/memory.cpp`
  - shared prealloc aggregate/value-home/addressing surfaces that would own
    the new prepared transport plan.

## In Scope

- Trace existing aggregate copy decomposition through AArch64 instruction and
  memory helpers.
- Identify the exact duplicated decision points: chunk boundaries, lane
  bindings, source/destination homes, scratch needs, and transport kind.
- Propose a prepared aggregate-transport plan/query shape if the duplication is
  real.
- Only after the shared contract exists, migrate AArch64 to consume it while
  retaining target-local instruction selection.

## Out Of Scope

- Implementing AArch64 aggregate copy cleanup before the probe names the shared
  contract.
- Treating ordinary load/store opcode choice or address spelling as shared
  aggregate transport authority.
- Combining this route with general memory address cleanup.
- Rewriting aggregate testcase expectations to avoid the missing authority.

## Proof Expectations

- Probe notes naming at least one stack-backed, one register-backed, and one
  mixed or lane-sensitive aggregate transport path, if those paths exist.
- For an implementation phase, focused aggregate copy tests proving the new
  prepared plan drives AArch64 lowering without regressing local instruction
  selection.
- Regression guard logs before accepting any code slice.

## Reviewer Reject Signals

- The route adds AArch64 named-case matching for one aggregate copy shape
  instead of specifying a reusable prepared transport plan.
- A patch migrates code before proving which aggregate transport fact is
  missing.
- A local helper rename is presented as shared-authority progress while chunk,
  lane, home, or scratch decisions remain duplicated.
- Tests are weakened or unsupported expectations are downgraded without
  explicit approval.
- General memory cleanup is mixed into this missing-authority probe.

## Closure Note

Closed 2026-06-01 after the Step 1 inventory, Step 2 contract draft, and Step
3 implementation established and consumed shared aggregate-transport authority
for the lane-sensitive AArch64 byval register publication path. The accepted
implementation publishes prepared aggregate transport facts from shared
prealloc call planning and requires AArch64 lowering to consume those facts
instead of re-deriving lane/source/chunk authority locally.

Closure evidence:
- Implementation commits: `042dd3b3a`, `b0ae0a27e`, and `5c28fb5a5`.
- Reviewer report: `review/idea75_step3_aggregate_transport_review.md` found
  no blocking route-quality issues.
- Focused repair guard: 3/4 before with
  `backend_aarch64_call_boundary_owner` failing, then 4/4 after repair.
- Broader backend guard: `^backend_` passed 169/169 before and 169/169 after
  with no new failures.

Remaining target-local residue: the large indirect byval selected-source path
continues to use the existing selected frame-slot address materialization. The
reviewer accepted this as target-local residue for idea 75, not a blocker for
the shared aggregate-transport authority completed here.
