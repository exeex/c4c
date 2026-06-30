# Stack-Home Branch Operand Materialization Plan

Status: Active
Source Idea: ideas/open/451_stack_home_branch_operand_materialization.md

## Purpose

Decide whether RV64 branch consumers may materialize branch operands or
conditions from stack homes using explicit prepared authority.

## Goal

Classify and, if justified, implement a bounded stack-home branch
operand/condition materialization route without weakening the existing
GPR-compatible branch publication predicates.

## Core Rule

Do not infer stack-home branch operands, condition values, freshness, or loads
from raw BIR shape, stack-slot spelling, block order, filenames, function
names, or representative testcase shape. Accepted materialization must consume
explicit prepared stack-home facts.

## Read First

- ideas/open/451_stack_home_branch_operand_materialization.md
- ideas/closed/449_pointer_relational_branch_publication.md
- build/agent_state/449_step4_residual_disposition/
- ideas/open/442_pointer_value_memory_provenance_publication.md
- src/backend/mir/riscv/codegen/object_emission.cpp

## Current Target

- Representative family: `930930-1` pointer relational branch rows with
  stack-slot operand or condition homes.
- Known prior status: ideas 441 and 449 completed GPR-compatible branch
  publication routes; stack homes remained fail-closed.
- First question: whether the relevant prepared rows carry sufficient stack
  object identity, offset, width, load policy, clobber safety, value freshness,
  and consumer boundary facts.

## Non-Goals

- Pointer equality/inequality branch publication completed by idea 441.
- Unsigned pointer relational branch publication completed by idea 449 for
  GPR-compatible homes.
- Select-result branch publication or move-bundle target materialization.
- Pointer-value/provenance publication from idea 442.
- Generic stack-to-register materialization.
- Inferring values from raw BIR, stack-slot spelling, representative shape, or
  one prepared dump layout.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline/log
  churn.
- Touching `review/`, `test_before.log`, `test_after.log`,
  `test_baseline.log`, or `test_baseline.new.log`.

## Working Model

Stack-home branch materialization is a separate consumer policy. The plan must
first prove whether the stack-home facts are coherent and fresh enough for a
branch operand/condition load. If not, the residual should route to the
prepared producer/provenance owner rather than broadening RV64 lowering.

## Execution Rules

- Step 1 is audit/classification only; do not edit implementation there.
- Any implementation must preserve the existing GPR-compatible branch
  predicates and add an explicit stack-home policy instead of weakening them.
- Missing, stale, incoherent, unsupported, or policy-ambiguous stack-home
  branch values must stay fail-closed.
- Classification-only proof:

```sh
git diff --check
```

- Code/test proof, if a materialization packet is selected:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

## Steps

### Step 1: Audit Stack-Home Branch Evidence

Inspect the `449_step4` residual evidence and current prepared/object output
for stack-home branch operand or condition rows. Record each row's operand
home, stack object identity, offset, width, freshness/clobber facts, pointer
provenance status, and first owning layer. Completion means `todo.md` contains
a representative audit table and identifies whether Step 2 can define a
materialization contract or must split a producer/provenance blocker.

### Step 2: Define Stack-Home Branch Materialization Contract

Specify the accepted prepared facts for stack-home branch operands or
conditions: object identity, offset, width, load policy, clobber safety,
freshness, consumer boundary, and fail-closed cases. Completion means
`todo.md` records the contract and names target files/tests if implementation
is justified, or records the exact blocker.

### Step 3: Implement Or Route First Materialization Packet

If Step 2 finds a bounded consumer packet, implement only that packet with
focused coverage. If no implementation is justified, record the precise
blocker and route it to a separate source idea. Completion means proof passes
for the selected packet or lifecycle state records the split/blocker.

### Step 4: Residual Disposition And Close Readiness

Re-probe the representative rows and classify any remaining first owner.
Completion means the source idea closes, remains active with one exact
in-scope packet, or splits a separate durable follow-up.
