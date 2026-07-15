# Native Vector Authority Carrier Publication Runbook

Status: Active
Source Idea: ideas/open/811_lir_native_vector_authority_carrier_publication.md
Activated from: 754 Step 8 separate-blocker switch

## Purpose

Publish the reusable vector authority prerequisite required before 754 can
select and implement one remaining vector row.

## Core Rule

Use checked current-function IDs and native vector/index/mask facts only.
Presentation strings remain compatibility mirrors and must never select,
recover, or repair authority.

## Read First

- `ideas/open/811_lir_native_vector_authority_carrier_publication.md`
- `ideas/open/754_lir_aggregate_vector_value_identity_convergence.md`
- `src/codegen/lir/ir.hpp`, `src/codegen/lir/operands.hpp`,
  `src/codegen/lir/types.hpp`, and `src/codegen/lir/verify.cpp`
- `src/codegen/lir/hir_to_lir/expr/binary.cpp` and `expr/misc.cpp`

## Non-Goals

- Do not implement or select an 754 vector row.
- Do not parse display strings or widen into non-vector authority families.
- Do not modify Raw-BIR, target lowering, MIR, or emission.

## Steps

### Step 1 - Specify the native vector carrier boundary

Goal: identify the smallest reusable structured carrier that represents
current-function result/use IDs, vector lane/element shape, index value/type,
and ordered shuffle mask lanes for the three existing vector seams.

Actions:

- inspect only the three vector op schemas, their verifier admission, and the
  `expr/binary.cpp` splat plus `expr/misc.cpp` vector-index lowering seams;
- define the carrier ownership and fail-closed conditions without attaching an
  operation-specific 754 verifier contract;
- record the concrete producer seams, positive matrix, malformed matrix, and
  excluded row-level behavior in `todo.md` before code changes.

Completion check: one bounded carrier contract and implementation packet exist;
no 754 vector row is selected or enabled.

### Step 2 - Publish structured vector identities and facts

Goal: add the Step 1 carrier and populate it from the existing vector seams.

Actions:

- preserve result/use `LirValueId`s through the seam;
- publish native lane/element, index value/type, and shuffle mask-lane facts;
- retain display strings only as checked mirrors and keep non-vector producers
  unchanged.

Completion check: carrier facts are present for the bounded seams and absent
or invalid forms fail closed without row-level operation validation.

### Step 3 - Verify carrier coherence and focused coverage

Goal: prove carrier ownership and fact coherence independently of 754 rows.

Actions:

- verify missing, unknown/foreign, and incoherent IDs/facts reject;
- add nearby carrier-focused valid and malformed coverage for each fact family;
- keep insert/extract/shuffle operation semantics out of the tests and verifier
  contract.

Completion check: focused carrier coverage proves valid and malformed behavior
without a display-text fallback or 754 operation claim.

### Step 4 - Prove and return the handoff

Goal: obtain the required build and regression proof, then hand off to 754.

Actions:

- run a fresh build, focused proof, and supervisor-selected matching regression
  guard; request a full baseline if shared-surface accumulation requires it;
- record the accepted carrier contract, proof, and exact 754 return point.

Completion check: accepted proof exists and 754 can resume at Step 9 for a
fresh one-row vector audit.
