# Project-Wide C++20 Host Toolchain Contract Runbook

Status: Active
Source Idea: ideas/open/802_project_wide_cpp20_host_toolchain_contract.md

## Purpose

Move all in-tree host implementation and native test targets to one required,
non-extension C++20 contract, then use that contract for one bounded NodeKind
registry authoring-readability convergence without changing its public helper
API or semantics.

## Core Rule

Keep host-build language policy separate from c4cll language-under-test modes.
Do not broaden the migration into general C++20 modernization, dependency
upgrades, BIR redesign, or idea 732.

## Read First

- `ideas/open/802_project_wide_cpp20_host_toolchain_contract.md`
- root and in-tree `CMakeLists.txt` files that declare a C++ standard
- `src/backend/bir/core/ir.hpp`
- `tests/backend/bir/backend_bir_node_kind_schema_test.cpp`

## Non-Goals

- Do not change c4cll source-language feature gates or fixture modes.
- Do not introduce typelist, concept-heavy, or class-type-NTTP DSLs.
- Do not change the NodeKind pass-facing helper API, classifications, stage
  admission, payload behavior, SSA semantics, or single-registry authority.
- Do not revise, activate, or execute idea 732.

## Execution Rules

- Preserve unrelated user changes and commit only coherent validated slices.
- Establish matching before/after proof before accepting code-bearing work.
- Prefer one CMake-3.20-compatible standard authority and explicit exceptions
  over duplicated target-local version pins.
- Keep NodeKind compile-time diagnostics local and readable while retaining
  registry-wide completeness, uniqueness, and relational validation.

## Step 1: Audit host-standard ownership and capture baseline

Goal: classify every C++17 declaration and establish the exact pre-change proof.

Actions:

- Enumerate `cxx_std_17`, `CXX_STANDARD 17`, `CMAKE_CXX_STANDARD 17`, and manual
  standard flags in versioned/generated build inputs.
- Separate production/native host targets, third-party boundaries, and
  language-under-test fixtures.
- Identify available supported host toolchains and the current effective
  compiler/standard-library versions without inventing unsupported claims.
- Capture the matching clean configure/build/full relevant CTest baseline in
  canonical `test_before.log`.

Completion check:

- Every occurrence is classified, the intended single authority is named, and
  a reproducible before command/log exists.

## Step 2: Establish the project-wide C++20 authority

Goal: make required non-extension C++20 the central host-build contract.

Actions:

- Implement one CMake-3.20-compatible authority consumed by all in-scope
  production and native test targets.
- Remove redundant local C++17 pins and preserve reviewed third-party
  boundaries.
- Add actionable configure-time rejection for unsupported compiler, mode, and
  required standard-library capability.
- Verify representative compile commands contain the effective C++20 mode.

Completion check:

- No in-scope target remains C++17 or compiler-default, supported clean build
  succeeds, and unsupported probes fail at configure time with clear guidance.

## Step 3: Publish the durable host-toolchain contract

Goal: make the C++20 requirement discoverable for future target authors.

Actions:

- Document the central authority, non-extension policy, CMake floor, supported
  compiler/standard-library floor, and future-target consumption pattern.
- Explicitly distinguish host C++20 from c4cll input language modes.
- Ensure documented support claims match the probes and available acceptance
  evidence from Step 2.

Completion check:

- A future agent can identify the required standard and correct CMake usage
  without reverse-engineering target-local declarations.

## Step 4: Converge the NodeKind C++20 authoring surface

Goal: make adding or reviewing a NodeKind a named, locally validated C++20 task.

Primary targets:

- `src/backend/bir/core/ir.hpp`
- `tests/backend/bir/backend_bir_node_kind_schema_test.cpp`

Actions:

- Replace long positional registry construction with designated initialization
  or an equally explicit named schema builder.
- Add typed stage, arity, and refinement policies only where they prevent
  category mixing.
- Force per-entry construction/validation at compile time with actionable
  failure locality, while retaining whole-registry completeness, uniqueness,
  and relational checks.
- Preserve every current kind exactly once and keep all existing pass-facing
  compile-time/runtime helpers and semantics compatible.
- Extend focused compile-time/runtime tests for the named authoring contract,
  invalid combinations, completeness, and helper compatibility.

Completion check:

- The registry site visibly uses C++20, entries read as named schemas, invalid
  local entries fail locally, global invariants still hold, and focused tests
  prove no public or semantic drift.

## Step 5: Run final migration proof and scope review

Goal: prove the combined toolchain and bounded NodeKind changes are complete.

Actions:

- Run a clean configure and complete build with the accepted host toolchain.
- Capture `test_after.log` using the exact Step 1 CTest command and compare it
  against `test_before.log` with the regression guard.
- Confirm no target/test was disabled, no expectation weakened, and no
  language-under-test fixture was mechanically migrated.
- Review the diff for opportunistic C++20 refactors, duplicate standard
  authorities, NodeKind DSL overengineering, semantic drift, and any idea-732
  change.

Completion check:

- Matching proof has no new regression or reduced supported coverage, all
  source acceptance criteria are satisfied, and the plan is ready for explicit
  plan-owner closure judgment.
