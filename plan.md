# Backend Test Source Reachability Cleanup Runbook

Status: Active
Source Idea: ideas/open/714_backend_test_source_reachability_cleanup.md

## Purpose

Make every backend test `.cpp` accountable to a supported build or test graph,
remove only proven unreachable surface, and prevent zombie sources from
recurring.

## Goal

Produce a reproducible cross-configuration reachability inventory, clean up
unreachable backend test sources and their dead support, and enforce the
inventory with a machine-checkable guard.

## Core Rule

Treat supported build and test graphs as the reachability authority.  Never
infer that a source is dead from its filename, directory, or absence from one
default configuration.

## Read First

- `ideas/open/714_backend_test_source_reachability_cleanup.md`
- backend test CMake target definitions and CTest registrations
- supported configuration, architecture, and test-option definitions
- existing persistent and opt-in backend test entry points

## Current Scope

- Backend test `.cpp` inventory and every supported graph edge that explains it.
- Intentional build-only contract binaries and optional supported targets.
- Dead registrations, gates, helpers, fixtures, generated expectations, and
  ownership or documentation references tied only to unreachable sources.
- A machine-checkable reachability inventory or guard.

## Non-Goals

- Do not change backend semantics.
- Do not delete optional or architecture-specific tests that remain supported.
- Do not weaken registrations, options, expectations, baseline coverage, or
  boundary contracts to make a source appear unused.
- Do not redesign backend tests beyond proven unreachable-surface cleanup.

## Execution Rules

- Work in small, reviewable packets and update `todo.md` after each packet.
- Use focused Git history only to explain particular leftovers, moves, or
  renames; no other idea or lifecycle range is a prerequisite.
- Record explicit evidence for every deletion and every intentional build-only
  classification.
- For code-changing steps, run build proof, the affected test subset, and the
  broader supported configuration checks required by the blast radius.
- Reject testcase-shaped allowlists and expectation-only or classification-only
  changes presented as cleanup progress.

## Step 1: Establish the supported reachability model

Goal: Define the inventory boundaries and supported configuration matrix before
classifying any source.

Concrete actions:

- Locate backend test roots, CMake target membership, `add_test`/CTest
  registrations, conditional options, architecture gates, and persistent or
  opt-in test entry points.
- Define what counts as compiled, executed, persistent, optional-supported, and
  intentional build-only reachability.
- Record the supported configurations needed to prove union reachability.

Completion check:

- The inventory roots, graph-edge types, and supported configuration matrix are
  explicit and sufficient to classify every backend test `.cpp`.

## Step 2: Inventory and classify every backend test source

Goal: Map every backend test `.cpp` to at least one supported graph edge or flag
it as unexplained.

Concrete actions:

- Enumerate all backend test `.cpp` files.
- Reconcile each file against target membership, CTest registration,
  configuration gates, persistent paths, and intentional build-only roles.
- Use focused Git history to explain suspicious stale files, moves, renames, or
  disconnected support when needed.
- Record unexplained sources and related support surfaces without deleting them
  yet.

Completion check:

- Every source has evidence-backed reachability classification, and each
  unexplained source has enough graph and history evidence for review.

## Step 3: Add the machine-checkable reachability guard

Goal: Detect unexplained backend test sources and lost final graph edges.

Concrete actions:

- Implement an inventory or guard derived from supported graph facts.
- Represent intentional build-only roles explicitly.
- Integrate the guard into an appropriate repository validation path.
- Add focused proof that a new unexplained source or removal of its final graph
  edge fails the guard.

Completion check:

- The guard passes for the explained inventory and fails for representative
  zombie-source conditions without relying on named-case exceptions.

## Step 4: Remove proven unreachable surface

Goal: Delete only sources unreachable from every supported configuration and
clean their exclusively dead support.

Concrete actions:

- Remove evidence-backed unreachable `.cpp` files in reviewable batches.
- Remove now-dead CMake entries, registrations, gates, helpers, fixtures,
  generated expectations, and ownership or documentation references.
- Preserve optional-supported targets, intentional build-only contracts, and
  all stable behavior and boundary expectations.
- Build and run the affected narrow tests after each batch.

Completion check:

- No proven unreachable source or stale support remains, and narrow build/test
  proof is green without contract weakening.

## Step 5: Validate the supported matrix and final inventory

Goal: Demonstrate that cleanup and the guard are sound across supported graphs.

Concrete actions:

- Run clean configure, build, and CTest discovery for the supported matrix.
- Run applicable persistent test paths and confirm they remain non-empty and
  green.
- Run the reachability guard against the final filesystem and graph inventory.
- Use the supervisor-selected regression guard or full validation checkpoint.

Completion check:

- Every remaining backend test `.cpp` is explained, all supported matrix proof
  is green, persistent paths are non-empty where applicable, and the guard
  prevents recurrence.
