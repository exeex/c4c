# 241 Phase E5 Route 4 and 5 edge publication identity adapter follow-up

## Goal

Prove and implement one Route 4 or Route 5 edge-publication identity adapter
that consumes a single semantic publication, edge, or join-source fact while
retaining prepared construction, move/home/storage policy, block order,
wrappers, diagnostics, fallback, expected strings, and baselines.

This is a narrow successor idea from the E5 gate artifact:

- `docs/bir_prealloc_fusion/phase_e5_prepared_bir_module_demotion_or_retirement_gate.md`

## Why This Exists

The E5 gate found that no whole `PreparedBirModule` field group and no whole
`PreparedFunctionLookups` group is ready for deletion, privatization, or
aggregate replacement. It did identify Route 4/5 publication or edge/join
identity reads inside `edge_publications` and
`edge_publication_source_producers` as possible one-adapter candidates.

This idea exists to test one such adapter boundary. It must retain prepared
publication construction and target/prepared policy while checking whether one
semantic identity read can move behind an agreement-gated adapter.

## In Scope

- One selected Route 4 or Route 5 publication, edge, or join-source identity
  read inside `edge_publications` or `edge_publication_source_producers`.
- One agreement-gated adapter for that read.
- Prepared fallback for absent, invalid, duplicate/conflict, ambiguous,
  mismatch, unsupported, prepared-only, policy-sensitive, and non-agreement
  cases.
- Retention of prepared publication construction, move/home/storage policy,
  block order, wrappers, printer/debug output, helper-oracle behavior,
  expected strings, and baselines.
- Nearby same-feature proof for related edge or join-source cases.

## Out Of Scope

- Whole `edge_publications` or `edge_publication_source_producers` deletion,
  privatization, or aggregate replacement.
- Whole `PreparedFunctionLookups` or `PreparedBirModule` retirement.
- Draft 155 opening, broad rewrite, or broad successor plan.
- Moving publication construction, move scheduling, storage/home policy, block
  order, wrapper output, branch/output spelling, formatting, instruction
  selection, or emission policy into Route 4, Route 5, or target-neutral BIR
  ownership.
- Expected-string rewrites, helper renames, supported-path downgrades,
  unsupported relabeling, wrapper-output relabeling, timeout masking, or
  baseline refreshes as proof.

## Acceptance Criteria

- Exactly one Route 4 or Route 5 publication, edge, or join-source identity
  read is named and routed through an agreement-gated adapter.
- The positive path consumes route/BIR semantic identity only after agreement
  with the current prepared lookup result.
- Prepared publication construction, move/home/storage policy, block order,
  wrappers, diagnostics, and fallback behavior remain authoritative for every
  adjacent policy-sensitive path.
- Prepared printer/debug output, route-debug output, helper-oracle names and
  status labels, wrapper output, expected strings, supported-path contracts,
  and baselines remain byte-stable unless a separately approved source idea
  changes that contract.
- No public prepared aggregate or lookup group is deleted, hidden, or renamed
  as the claimed progress.

## Required Proof Matrix

The implementation is not acceptance-ready until proof covers:

- the selected positive Route 4 or Route 5 identity read;
- absent, invalid, duplicate/conflict, ambiguous, mismatch, unsupported,
  prepared-only, policy-sensitive, and non-agreement fallback where relevant;
- unchanged prepared lookup delivery and public compatibility behavior;
- unchanged diagnostic/oracle strings and helper status labels;
- unchanged printer/debug, route-debug, wrapper output, expected strings, and
  baselines;
- nearby same-feature publication, edge, or join-source cases proving the
  adapter is semantic and not tied to one named fixture;
- target-output no-change proof for adjacent block-order, move/home/storage,
  branch/output spelling, wrapper, formatting, and emission surfaces.

## Reviewer Reject Signals

- The change claims or implies whole `edge_publications`,
  `edge_publication_source_producers`, `PreparedFunctionLookups`,
  `PreparedBirModule`, or draft 155 retirement readiness.
- The implementation moves publication construction, move scheduling,
  storage/home policy, block order, branch/output spelling, wrapper output,
  formatting, instruction selection, or emission policy into Route 4, Route 5,
  or target-neutral BIR ownership.
- The adapter is a facade rename or public API hiding exercise that preserves
  the old failure mode.
- The patch rewrites expectations, helper names, supported-path status,
  wrapper output, or baselines instead of preserving behavior.
- The change downgrades supported tests to unsupported or weakens public
  prepared fallback, diagnostic/oracle, debug/printer, wrapper, or helper
  contracts.
- The implementation adds named-case shortcuts, fixture-shaped matching, or
  special handling for one known edge/join testcase instead of consuming a real
  semantic publication identity fact.

## Closure Note

Closed on 2026-06-13 after implementing and proving one narrow Route 5
current-block join-source identity adapter.

Accepted scope:

- Selected reader: `current_block_join_prepared_query_source(...)`, backed by
  `build_current_block_join_prepared_query_routing(...)` in
  `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`.
- Selected semantic fact: `routing.sources[instruction_index]`.
- Adapter boundary: consume the Route 5 current-block join-source identity only
  after it agrees with the prepared current-block join source answer.
- Fallback and compatibility proof covered selected positive agreement,
  no-source, memory-source, duplicate/conflict, mismatch, missing route data,
  unsupported-move, adjacent prepared-owned facts, public prepared helper/oracle
  compatibility, and instruction-dispatch compatibility.
- Close-time regression guard passed with `--allow-non-decreasing-passed` using
  canonical `test_before.log` and `test_after.log`; before 3/3 passed, after
  3/3 passed, and no new failing tests were introduced.

Explicit non-claims:

- No whole `edge_publications` retirement readiness is claimed.
- No whole `edge_publication_source_producers` retirement readiness is claimed.
- No `PreparedFunctionLookups` or `PreparedBirModule` deletion,
  privatization, hiding, aggregate replacement, or retirement readiness is
  claimed.
- No draft 155 retirement readiness is claimed.
- Publication construction, parallel-copy scheduling, homes/storage policy,
  block order, wrappers, output spelling, formatting, instruction selection,
  and final emission remain prepared/target-owned.
