# BIR Return-Chain Oracle Equivalence Runbook

Status: Active
Source Idea: ideas/open/178_bir_return_chain_oracle_equivalence.md

## Purpose

Prove the BIR-owned Route 8 return-chain answers match the existing prepared
return-chain helper answers before consumer migration or prepared API
contraction.

## Goal

Add focused oracle-equivalence coverage comparing Route 8 terminal and
next-operand lookup results with
`find_prepared_return_chain_terminal_value(...)` and
`find_prepared_return_chain_next_operand_value(...)`.

## Core Rule

Treat prepared return-chain helpers as the oracle for this idea. Do not migrate
AArch64 consumers, hide prepared helpers, or broaden Route 8 beyond
same-function, same-block scalar binary chains ending at a BIR return
terminator.

## Read First

- `ideas/open/178_bir_return_chain_oracle_equivalence.md`
- `ideas/closed/177_bir_return_chain_schema_index.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

## Current Targets

- Build Route 8 and prepared answer sets for the same function, block,
  instruction position, and chain value.
- Assert exact equivalence for accepted terminal answers.
- Assert exact equivalence for accepted immediate next-operand answers.
- Assert no usable answer for rejected, ambiguous, and conflicting cases.
- Keep prepared return-chain helpers public and unchanged.

## Non-Goals

- Do not change AArch64 consumers to prefer Route 8.
- Do not contract, hide, rename, or privatize the prepared return-chain API.
- Do not change accepted return-chain semantics outside equivalence-test needs.
- Do not import homes, registers, ABI return placement, scratch policy, alias
  decisions, ALU record construction, or emission order into Route 8.
- Do not broaden the route beyond same-function, same-block scalar binary
  chains ending at a BIR return terminator.

## Working Model

- Route 8 and prepared lookups must be queried with matching logical keys:
  function/block identity, instruction position, and chain value.
- Accepted cases must agree on terminal return value and immediate
  next-operand value, including the absence of a next operand when that is the
  valid answer.
- Rejected, ambiguous, and duplicate-conflict cases must fail closed on both
  routes; no test should accept a winner for conflicting answers.
- Existing prepared helpers remain the source of oracle truth until consumer
  migration has its own active idea.

## Execution Rules

- Prefer extending the existing backend/BIR lookup helper tests over creating a
  new framework.
- Keep assertions semantic: compare values and fail-closed statuses, not
  implementation details such as container layout.
- If an equivalence fixture exposes a Route 8 bug, fix the semantic lookup or
  indexing rule rather than weakening expectations.
- Build narrowly after code-changing packets:
  `cmake --build build --target backend_prepared_lookup_helper_test`.
- Focused proof should include:
  `ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure`.
- Escalate validation if public headers, `PreparedFunctionLookups`, route
  facades, AArch64 context projection, or consumer code changes.

## Ordered Steps

### Step 1: Inspect Oracle Surfaces and Fixture Keys

Goal: identify the exact common key and helper surfaces for comparing Route 8
and prepared return-chain answers.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:

- Inspect Route 8 key, record, index, terminal lookup, and next-operand lookup
  helpers.
- Inspect prepared return-chain key construction and terminal/next-operand
  lookup helpers.
- Identify the existing Route 8 schema/index fixtures that can be extended
  into oracle-equivalence fixtures.
- Record the chosen fixture and helper shape in `todo.md` before code changes.

Completion check:

- The executor can name how the same function, block, instruction position,
  and chain value will be queried through both Route 8 and prepared helpers.

### Step 2: Add Positive Terminal Equivalence Coverage

Goal: prove accepted Route 8 terminal answers exactly match prepared terminal
answers.

Primary target:

- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:

- Build Route 8 and prepared lookup indexes for the same positive same-block
  return-chain fixture.
- Compare accepted terminal value answers for at least one chain value.
- Include a case where the terminal value is reached through the route's
  normal same-block walk, not a manually shaped one-off record.

Completion check:

- Positive terminal oracle assertions fail if either route returns a different
  terminal value or no accepted answer.

### Step 3: Add Positive Next-Operand Equivalence Coverage

Goal: prove accepted Route 8 immediate next-operand answers exactly match
prepared next-operand answers.

Primary target:

- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:

- Reuse or add a positive same-block fixture with an immediate next instruction
  consuming the current chain value.
- Compare accepted next-operand value answers from Route 8 and prepared
  helpers.
- Also assert the agreed absence of a next-operand answer where the terminal
  chain element has no next operand.

Completion check:

- Positive next-operand oracle assertions fail if either route returns a
  different next operand, invents an answer, or omits a valid answer.

### Step 4: Add Rejected and Ambiguous Fail-Closed Equivalence Coverage

Goal: prove unsupported and ambiguous shapes produce no usable answer through
either route.

Primary target:

- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:

- Cover rejected fixtures such as unsupported opcode, unnamed chain value,
  unnamed terminal return value, broken same-block walk, non-return terminator,
  cross-block chain, and missing instruction key where applicable.
- Cover ambiguous cases already represented by Route 8 status or omission.
- Assert both terminal and next-operand queries fail closed when the fixture
  has no valid answer.

Completion check:

- Negative and ambiguous oracle assertions fail if either route produces a
  usable terminal or next-operand answer.

### Step 5: Add Duplicate-Conflict Equivalence Coverage

Goal: prove duplicate publications with conflicting terminal or next-operand
answers fail closed without choosing a winner.

Primary target:

- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:

- Add or reuse conflict fixtures for duplicate terminal answers.
- Add or reuse conflict fixtures for duplicate next-operand answers.
- Assert both Route 8 and prepared helper queries expose no usable terminal or
  next-operand answer for conflicting duplicate publications.

Completion check:

- Conflict oracle assertions fail if either route selects one conflicting
  answer as usable.

### Step 6: Validate and Hand Off

Goal: leave oracle-equivalence coverage green and ready for the consumer
migration idea.

Actions:

- Run the delegated build and focused CTest command.
- Summarize proof in `todo.md`.
- Confirm the slice leaves AArch64 consumers and prepared helper visibility
  unchanged.
- Identify `ideas/open/179_bir_return_chain_consumer_migration.md` as the next
  lifecycle prerequisite only after oracle equivalence is complete.

Completion check:

- Focused oracle tests are green, `todo.md` records proof, and the diff does
  not include consumer migration or prepared API contraction.
