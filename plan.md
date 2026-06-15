# Phase F4 edge_publication_source_producers x86 Boundary Map

Status: Active
Source Idea: ideas/open/267_phase_f4_edge_publication_source_producers_x86_consumer_boundary_fail_closed_blocker_map.md

## Purpose

Map whether `PreparedFunctionLookups::edge_publication_source_producers` has a
real x86 consumer boundary, and record the fail-closed blockers that must be
satisfied before any later implementation packet is safe.

## Goal

Produce an evidence-backed blocker map for the source-producer relation that
either names a concrete x86 consumer boundary or records that no implementation
packet is currently authorized.

## Core Rule

This runbook is analysis and blocker mapping only. Do not demote, delete,
privatize, wrap, migrate, or implement `edge_publication_source_producers`
unless a later lifecycle step creates a separate implementation idea with a
named x86 consumer boundary and supported fail-closed route.

## Read First

- `ideas/open/267_phase_f4_edge_publication_source_producers_x86_consumer_boundary_fail_closed_blocker_map.md`
- Definitions and use sites for `PreparedFunctionLookups`
- Route 5 / BIR source-producer identity builders and readers
- x86 backend lowering paths that consume source-producer identity or prepared
  lookup publication
- RISC-V or non-x86 paths only as comparison evidence for applicability

## Current Targets

- `PreparedFunctionLookups::edge_publication_source_producers`
- Prepared source-producer rows
- Route 5 / BIR source-producer identity
- x86 consumer boundaries where prepared and Route 5 / BIR identity could
  disagree
- Compatibility surfaces that expose or preserve the public prepared lookup

## Non-Goals

- Lookup demotion, deletion, privatization, accessor wrapping, or adapter
  migration.
- Any source-producer implementation packet without a named x86 consumer
  boundary.
- Moving target-owned publication/output, immediate encoding, storage,
  addressing, carrier/helper, formatting, wrapper, ABI, register, stack,
  instruction spelling, or emission policy into BIR.
- Broad prepared aggregate retirement.
- Weakening unsupported expectations, helper/oracle statuses, fallback names,
  route-debug output, prepared-printer output, wrapper output, exact target
  output, or baseline behavior.

## Working Model

- Treat prepared `edge_publication_source_producers` as blocked public
  authority until a same-identity semantic authority and consumer path are
  proven.
- A real x86 consumer boundary must join prepared source-producer rows to the
  same Route 5 / BIR producer/source identity and reject disagreement.
- If no exact consumer exists, the correct output is a blocked map, not a
  compatibility wrapper or classification-only change.
- Compatibility surfaces may be retained only as compatibility authority; do
  not hide the old public authority behind renamed fields or accessors.

## Execution Rules

- Preserve source intent in the open idea; use `todo.md` for packet notes and
  evidence gathered during execution.
- Prefer semantic producer/source identity checks over named-fixture or
  testcase-shaped matching.
- For every row, record x86 evidence and either RISC-V comparison evidence or
  explicit non-applicability.
- If a later implementation path appears necessary, stop at the blocker map and
  ask the supervisor for a separate lifecycle idea instead of expanding this
  runbook.
- Do not claim capability progress through expectation rewrites, helper renames,
  status/oracle relabeling, route-debug or printer output changes, or
  classification-only notes.

## Ordered Steps

### Step 1: Inventory Source-Producer Authority

Goal: identify where prepared and Route 5 / BIR source-producer identity is
created, stored, published, and printed.

Primary target: `PreparedFunctionLookups::edge_publication_source_producers`.

Actions:

- Inspect definitions and constructors for `PreparedFunctionLookups`.
- Trace producers of prepared source-producer rows.
- Trace Route 5 / BIR source-producer identity creation and publication.
- List retained compatibility surfaces that expose the prepared lookup.

Completion check:

- The execution notes name the prepared producers, Route 5 / BIR authority
  points, and compatibility surfaces without changing implementation files.

### Step 2: Locate x86 Consumer Boundary

Goal: decide whether x86 has a real consumer of the source-producer relation.

Primary target: x86 lowering or publication paths that read source-producer
identity.

Actions:

- Search x86 backend readers for prepared source-producer lookup use.
- Search x86 backend readers for Route 5 / BIR source-producer identity use.
- Identify any boundary where the two identities must agree.
- If no exact identity consumer exists, record the absence and why an
  implementation packet remains unsafe.

Completion check:

- The execution notes either name one concrete x86 consumer boundary or record
  that no exact consumer exists yet, with file/function evidence.

### Step 3: Build Fail-Closed Row Map

Goal: map every required blocker row and expected fail-closed behavior.

Primary target: all source-producer agreement and disagreement cases required
by the source idea.

Actions:

- Map duplicate, conflict, mismatch, and missing rows.
- Map prepared-only rows.
- Map fallback rows.
- Map `LoadLocal` memory-source rows.
- Map immediate-producer rows.
- Map policy-sensitive rows.
- For each row, name x86 evidence, RISC-V evidence or explicit
  non-applicability, preserved compatibility surfaces, and expected
  fail-closed behavior.

Completion check:

- The execution notes contain a complete row map covering every required row
  and do not leave nearby same-feature cases unexamined.

### Step 4: Decide Implementation Eligibility

Goal: convert the map into a lifecycle decision without expanding this analysis
runbook into implementation work.

Primary target: the completed blocker map.

Actions:

- If a semantic authority and supported x86 consumer path both exist, record the
  minimum conditions a later implementation idea must satisfy.
- If either authority or consumer path is missing, record that no implementation
  packet is safe yet.
- Identify any separate initiative that must be created under `ideas/open/`
  before implementation work proceeds.

Completion check:

- The execution notes state whether implementation is blocked or eligible for a
  separate lifecycle idea, and the decision is tied to the row map evidence.

### Step 5: Validate the Map

Goal: prove the runbook result is a map-only lifecycle slice with no route drift.

Primary target: planning artifacts and gathered evidence.

Actions:

- Check that no implementation files, expectations, helper/oracle statuses,
  route-debug output, prepared-printer output, wrapper output, exact target
  output, unsupported behavior, or baselines were weakened.
- Check that the map does not authorize broad prepared retirement or lookup
  demotion.
- Ask the supervisor whether reviewer scrutiny is needed before closure or a
  follow-up implementation idea.

Completion check:

- The supervisor can decide whether to close, rewrite, or split the active
  lifecycle state from the map and proof notes.
