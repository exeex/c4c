# 270 Phase F5 memory_accesses prepared-only same-consumer fail-closed proof

## Closure

Closed as an explicit blocker-map result from the required first step. The
current x86 and riscv proof surfaces do not expose a supported same-consumer
row that directly reads `PreparedFunctionLookups::memory_accesses`; continuing
inside this idea would require synthetic stale prepared state or scope drift.

Durable blocker map:

- Exact public-field consumers found:
  `src/backend/mir/aarch64/codegen/alu.cpp`,
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`, and
  `src/backend/prealloc/prepared_lookups.cpp`.
- Current x86 adjacent route:
  `render_agreed_route3_load_local_statement_memory_operand(...)` reaches
  Route 3/Route 5 source-memory agreement through
  `PreparedEdgePublication::source_memory_access` and
  `PreparedAddressingFunction`, not through the public
  `PreparedFunctionLookups::memory_accesses` field.
- Current riscv search result:
  riscv emit paths accept `PreparedFunctionLookups`, but no supported searched
  route directly reads `lookups->memory_accesses`.
- Nearest exact public consumer is AArch64 scalar ALU lowering for
  `f.load_local`, which is outside this idea's x86/riscv target.
- Follow-up is tracked in
  `ideas/open/271_phase_f5_x86_riscv_memory_accesses_public_consumer_fixture_support.md`.

## Goal

Prove one bounded prepared-only fail-closed surface for
`PreparedFunctionLookups::memory_accesses`: a still-public memory lookup
consumer must reject a self-consistent prepared-only source-memory row when
the matching Route 3/Route 5 semantic authority is absent or non-agreeing,
while preserving prepared compatibility output.

This is a proof packet, not a demotion packet. It must not delete, privatize,
wrap, or hide `memory_accesses`.

## Why This Exists

Closed idea
`ideas/closed/265_phase_f4_memory_accesses_unsupported_stale_fail_closed_proof_map.md`
kept `PreparedFunctionLookups::memory_accesses` public because selected-path
and helper-level guardrails do not prove every still-public consumer rejects a
self-consistent prepared-only row at the same Route 3/Route 5 authority
boundary.

The closest useful next step is not broad memory lookup retirement. It is a
single same-consumer proof packet that demonstrates how one public consumer
fails closed when prepared data looks internally coherent but lacks matching
route/BIR semantic authority.

## In Scope

- Choose one existing public `memory_accesses` consumer reached by current
  x86 or riscv proof surfaces.
- Prefer the selected x86 Route 3 `LoadLocal` source-memory agreement path if
  it can express a prepared-only row through supported fixtures.
- If the x86 path cannot express the row without synthetic stale state, choose
  the nearest riscv source-memory consumer that can express the same
  prepared-only authority gap with stable output.
- Prove that the consumer does not treat the prepared-only row as semantic
  Route 3/Route 5 agreement.
- Preserve helper/oracle statuses, prepared status strings, fallback names,
  route-debug output, prepared-printer output, wrapper output, exact target
  output, and baseline behavior.
- Record whether this proof reduces the PO-family blocker from idea 265 or
  exposes a more specific fixture/support gap.

## Required First Step

Before changing tests or implementation, identify the exact consumer and row:

- the prepared lookup or publication row being treated as prepared-only;
- the missing or non-agreeing Route 3 memory/source fact;
- the missing or non-agreeing Route 5 publication/source owner, if relevant;
- the current public compatibility output that must remain stable;
- why the row is reachable through supported fixtures rather than hand-built
  stale prepared state.

If no supported same-consumer prepared-only row exists, close with a blocker
map and create a narrower fixture-support idea instead of adding synthetic
test-only state.

## Out Of Scope

- `memory_accesses` demotion, deletion, privatization, wrapping, or public API
  hiding.
- Broad `PreparedFunctionLookups`, `PreparedBirModule`, or draft 155
  retirement.
- Stale-publication, byte-offset drift, or cross-publication mismatch proof
  unless the selected prepared-only row naturally requires one of those cases.
- Moving storage, addressing, source-home, ABI, layout, register, stack,
  wrapper, formatting, emission, instruction spelling, or exact target-output
  policy into BIR.
- Weakening expectations, helper/oracle statuses, fallback names, unsupported
  behavior, route-debug output, prepared-printer output, wrapper output, or
  baseline tests.

## Acceptance Criteria

- One same-consumer prepared-only row is named and proved or explicitly
  blocked by missing fixture support.
- The proof shows that internally coherent prepared memory data is not accepted
  as route/BIR semantic agreement without matching Route 3/Route 5 authority.
- Missing, prepared-only, unsupported, fallback, and policy-sensitive behavior
  remains fail-closed for the selected row.
- Public prepared compatibility output remains observable and byte-stable.
- The closure note states whether a later `memory_accesses` proof packet
  should continue with stale-publication, byte-offset drift, or
  cross-publication mismatch rows.

## Suggested Proof Scope

Use the narrowest matching backend tests for the selected consumer. Candidate
families include:

- x86 route-debug / handoff-boundary tests if the selected x86 `LoadLocal`
  path is used;
- riscv prepared edge-publication tests if the selected riscv source-memory
  path is used;
- prepared lookup helper tests if helper/oracle compatibility rows are touched.

Any code-changing slice needs matching before/after proof and regression guard
using canonical `test_before.log` and `test_after.log`.

## Reviewer Reject Signals

- The proof uses hand-built stale prepared state that an earlier supported
  route would reject, while claiming same-consumer coverage.
- The slice proves only helper-level lookup behavior but claims target
  consumer fail-closed behavior.
- The patch accepts prepared-only memory data as Route 3/Route 5 semantic
  agreement.
- The patch hides public `memory_accesses` authority behind a renamed BIR,
  route, private-accessor, adapter, wrapper, or compatibility helper.
- The patch weakens expected output, unsupported status, helper/oracle status,
  fallback behavior, route-debug output, prepared-printer output, wrapper
  output, exact target output, or baselines.
- The patch claims `memory_accesses`, `PreparedFunctionLookups`, or
  `PreparedBirModule` demotion readiness from this one proof packet.
