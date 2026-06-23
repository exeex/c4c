# RV64 Nested Select Chain Store Source Publication Plan

Status: Active
Source Idea: ideas/open/320_rv64_nested_select_chain_store_source_publication.md

## Purpose

Repair RV64 prepared lowering for deeper nested select-chain store-source
publication when simple pointer-typed select publication already succeeds but
later ternary joins still lack destination-access facts and truncate emitted
code.

## Goal

Make the RV64 prepared store-source/select-chain path publish usable
destination access for nested ternary joins exposed by `src/00144.c`, without
matching candidate names, `tern.end.*` labels, or emitted instruction text.

## Core Rule

Repair the prepared store-source/select-chain publication mechanism. Do not
claim progress by preserving only the already-fixed early pointer-typed select
stores while later nested store-source records still report
`missing_destination_access`.

## Read First

- `ideas/open/320_rv64_nested_select_chain_store_source_publication.md`
- Idea 316 Step 5 evidence for `src/00144.c` under
  `build/rv64_c_testsuite_probe_latest/triage_316_step5/`
- Existing focused pointer-typed select publication coverage from idea 316,
  so this route does not duplicate the already repaired simple case.

## Scope

- Nested select-chain publication into store sources on RV64.
- Prepared store-source destination-access facts for later ternary join blocks.
- Pointer/integer select chains only where they feed the nested store-source
  publication path exposed by `src/00144.c`.
- Focused backend coverage beyond the simple pointer-typed local publication
  already repaired.

## Non-Goals

- Simple pointer-typed select publication into local pointer slots already
  repaired by idea 316.
- Stack-homed fused compare/control-flow emission for `src/00077.c` and
  `src/00143.c`.
- Generic local frame-slot address publication from idea 312.
- i16 local-array select/store publication from idea 321.
- Aggregate/byval repair, function-pointer repair, external-call policy, or
  broad switch/control-flow repair.

## Working Model

The failure family is expected to start after early pointer stores emit. Later
nested ternary joins feed store-source publication, but prepared records still
lack destination-access facts around `tern.end.23`, `tern.end.33`,
`tern.end.54`, and `tern.end.63`. RV64 output can then degrade into repeated
self-moves such as `mv t0, t0` and truncate before a complete function return.

## Execution Rules

- Use prepared store-source/select-chain facts as the source of authority.
- Do not match `src/00144.c`, `tern.end.*`, exact source ternary shape, or
  observed instruction text.
- Keep proof focused on nested select-chain store-source publication; do not
  absorb idea 321's halfword local-array/select-store residual.
- Preserve existing simple pointer-typed select publication behavior while
  adding coverage for deeper nested store-source joins.
- Validation ladder for implementation steps: build, focused backend test,
  candidate reprobe, then backend regression guard when the route reaches a
  close-quality checkpoint.

## Step 1: Normalize Nested Store-Source Evidence

Goal: reproduce the current `src/00144.c` failure and identify the first
missing prepared publication fact.

Actions:

- Reprobe `src/00144.c` through BIR, prepared-BIR, RV64 emit, clang link, and
  qemu when link succeeds.
- Inspect prepared store-source/select-chain records around the later ternary
  joins that previously reported `missing_destination_access`.
- Compare the first bad point against already-green simple pointer-typed select
  publication coverage to keep the failure boundary clear.
- Record whether the emitted RV64 still shows self-moves, truncation, missing
  return, or a different first failure.

Completion checks:

- Fresh artifacts identify the first nested select-chain store-source failure.
- The evidence distinguishes this route from simple pointer-typed select
  publication and from idea 321's i16 local-array/select-store residual.
- `todo.md` records the exact focused candidates, proof commands, and
  classification.

## Step 2: Add Focused Nested Select-Chain Store-Source Coverage

Goal: create focused backend coverage for nested select-chain store-source
publication with destination-access facts for later ternary joins.

Actions:

- Add a focused test shape that reaches nested pointer/integer select chains
  feeding a store source after the simple pointer-typed publication case.
- Assert prepared facts strongly enough to catch missing destination-access at
  nested joins.
- Assert generated RV64 strongly enough to catch self-moves that hide missing
  destination access and truncation before `ret`.
- Keep the fixture independent of `src/00144.c`, fixed `tern.end.*` names, and
  observed instruction text.

Completion checks:

- Focused dump/codegen coverage fails for the current nested publication gap or
  is marked with the repo's expected-repair convention.
- Existing simple pointer-typed select publication tests remain green.
- The new coverage is narrow enough that a repair can target store-source
  publication rather than broad control-flow or ABI behavior.

## Step 3: Repair Prepared Destination-Access Publication

Goal: make prepared store-source/select-chain data carry destination access for
deeper nested ternary joins.

Actions:

- Locate where prepared store-source records are created or propagated for
  nested select-chain joins.
- Publish or preserve destination-access facts through the nested chain using
  the same semantic authority as adjacent prepared store-source paths.
- Avoid target-local source scans or RV64-only duplication of prepared facts.
- Re-run focused prepared dump tests and simple pointer-typed select neighbors.

Completion checks:

- Focused prepared records no longer show `missing_destination_access` for the
  nested store-source path.
- The repair is not tied to `tern.end.*`, `src/00144.c`, or a fixed source
  ternary spelling.
- Previously repaired simple pointer-typed select publication remains green.

## Step 4: Repair RV64 Consumption and Emission

Goal: ensure RV64 lowering consumes the nested store-source publication facts
and emits complete code for the focused path.

Actions:

- Update RV64 lowering only where it consumes the repaired prepared
  store-source/select-chain facts.
- Remove self-move/truncation behavior caused by missing destination access.
- Run focused dump/codegen/runtime proof, then reprobe `src/00144.c`.
- If `src/00144.c` advances to a separate mechanism, classify that mechanism
  with emitted-code evidence instead of expanding this route silently.

Completion checks:

- Focused RV64 codegen no longer truncates or hides missing destination access
  behind self-moves.
- `src/00144.c` either emits, links, and runs under qemu, or has a concrete
  residual classification outside idea 320.
- No broad switch/control-flow, ABI, or idea 321 work is mixed into this step.

## Step 5: Reprobe and Close Classification

Goal: decide whether idea 320 is complete under its source acceptance criteria.

Actions:

- Re-run the focused backend proof and the `src/00144.c` candidate probe.
- Compare against existing backend regression logs or generate a matching close
  guard if closure is proposed.
- Split any evidence-backed residual into a new open idea only if it is
  separate from existing ideas.

Completion checks:

- Focused nested select-chain store-source publication coverage is green.
- `src/00144.c` is either `supported-linked-pass` or has a concrete
  out-of-scope residual classification.
- Close-time regression guard evidence is ready for plan-owner review.
