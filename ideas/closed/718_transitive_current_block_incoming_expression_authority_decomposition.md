# Transitive Current-Block Incoming-Expression Authority Decomposition

Status: Complete
Type: backend authority decomposition
Blocks:
- `ideas/open/717_current_block_routed_value_authority_decomposition.md`
- `ideas/open/716_prealloc_current_block_routing_authority_closure.md`

## Goal

Define and prove the independent authority seams needed to represent a current-
block incoming expression whose published source depends transitively on other
producer values, then compose those seams into stable-key owner facts.

## Why This Idea Exists

Idea 717 Steps 6.1 and 6.2 repeatedly collide with an unchanged supported
contract. The integration path requires both the prepared publication source
`%source` and its producer dependency `%operand`, while the focused routed-
operand contract authorizes only the preserved publication source and rejects
unrelated operands. Rewriting prepared source identity to `%operand` would make
the stable query tautological rather than prove authority.

The no-policy supported path exposes a separate seam: it has an attached owner
but zero applicable facts while still expecting incoming-expression routing.
Consumer-only proof reached 317/320, and preparation tracing found the same
semantic collision. Idea 717 remains open and blocked; its AArch64 integration
test remains an integration contract rather than the discovery surface.

## In Scope

- Prove direct publication-source identity authority independently of producer
  dependency authority.
- Define the transitive producer-dependency closure that may authorize an
  incoming expression, including termination, ambiguity, and conflict rules.
- Define supported semantics for an attached owner with no policy and zero
  applicable facts without treating owner presence alone as authority.
- Compose the proven seams into stable-key owner facts before target
  consumption, without Route 5 or target-local reconstruction.
- Extract focused backend-owned probes, preferably under `tests/backend/case/`,
  with exactly one primary seam per probe.

## Focused Probe Contracts

1. A direct-publication-source probe owns preserved source identity and rejects
   source-identity rewriting.
2. A producer-dependency-closure probe owns which transitive dependencies are
   authoritative, how closure terminates, and how missing or conflicting paths
   fail closed.
3. A no-policy/zero-owner-fact probe owns the supported semantic distinction
   among absent policy, absent owner, and an attached owner with zero facts.
4. A stable-key-composition probe owns conversion of the three proven contracts
   into owner facts without Route 5 or target-local reconstruction.
5. `backend_aarch64_current_block_join_routing` remains integration-only and
   must not define any of these contracts.

Exact focused filenames are selected during baseline and probe extraction so
they follow the registered `tests/backend/case/` conventions and do not copy
the integration monolith.

## Out Of Scope

- AArch64-side authority discovery, Route 5 authority, target-local graph
  traversal, or reconstruction from successor bodies.
- Rewriting publication source identity to the queried dependency.
- Treating owner attachment by itself as proof of an incoming expression.
- Changing, downgrading, or marking unsupported any existing supported
  integration vector.
- Reopening accepted owner storage and lookup lifetime work.
- Closing ideas 717, 716, 713, or 705 during this decomposition initiative.

## Acceptance Criteria

- Each authority seam has one registered focused probe with positive,
  negative, missing, ambiguous, and conflicting cases appropriate to that seam.
- Direct source identity remains preserved and independently queryable.
- Transitive dependency authority follows a general semantic closure rule,
  terminates deterministically, and fails closed for incomplete or conflicting
  evidence.
- No-policy and zero-fact supported behavior is explained and proven by an
  explicit semantic contract rather than owner presence or an expectation
  exception.
- Stable-key owner facts compose all proven categories upstream of AArch64;
  target consumption adds no authority builder or fallback.
- Focused proof and fresh broader backend proof are green before the unchanged
  AArch64 integration contract is used as final composition evidence.
- The composed contract is precise enough to hand execution back to idea 717
  without the `%source`/`%operand` or zero-fact collision.

## Reviewer Reject Signals

- A named integration vector, operand spelling, instruction instance, or fixed
  dependency depth receives a testcase-shaped shortcut.
- Publication `source_value_id` or source name is rewritten to `%operand` (or
  any queried dependency) merely to make a stable-key lookup pass.
- Transitive closure is approximated by a first match, a bounded named chain,
  an unproved subset, or a target-local scan while other applicable producers
  are ignored.
- Owner presence, missing policy, or zero facts silently authorizes a value
  without a focused semantic rule and negative matrix.
- Supported expectations are rewritten, downgraded, or marked unsupported
  without explicit user approval.
- Helper renames, expectation edits, classification-only changes, or probe
  registration alone are claimed as backend capability progress.
- Route 5, target reconstruction, or the exact old `%source`/`%operand`
  collision survives behind a new abstraction name.
- Broad publication, BIR, MIR, or AArch64 rewriting replaces the four focused
  seams without proving them independently.

## Handback Criteria

Hand back to idea 717 only after all four seams are independently registered,
green, and composed into upstream owner facts; unchanged integration vectors
and fresh broader backend proof must then pass without target-local authority.

## Completion Record

- Direct publication-source identity remains preserved and independently
  queryable.
- Producer dependency authority is a deterministic transitive closure that
  fails closed on missing, ambiguous, conflicting, cyclic, or incomplete
  evidence.
- Absent policy, absent owner, attached owner with zero applicable facts, and
  authoritative owner facts are represented distinctly; owner presence alone
  grants no authority.
- Stable-key owner facts compose these contracts upstream, and AArch64 only
  consumes the attached prepared facts without Route 5 authority construction,
  target-local reconstruction, or source-identity rewriting.
- All four focused contracts, the unchanged
  `backend_aarch64_current_block_join_routing` integration vectors, and the
  fresh broader backend proof are green.
- Close-time regression guard compared the matching backend commands in
  `test_before.log` and `test_after.log`: both passed 324/324 tests with no new
  failures.

The handback criteria are satisfied. Execution returns to idea 717 with the
transitive `%source`/`%operand` and zero-fact collision resolved by the durable
upstream authority contract above.
