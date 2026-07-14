# LIR Automatic Local Label-Address Table Initializer Authority

Status: Open (active blocker for
`ideas/open/768_lir_computed_goto_label_address_table_initialization_authority_decomposition.md`
Step 5)
Type: focused automatic local initializer direct-constant authority contract
Predecessor: 768 Step 5 after accepted 770 native direct-constant contract

## Goal

Preserve structured native label-address direct-constant authority through
automatic local label-address table initializer emission, so each nested table
element reaches its owned generic consumer with its operand identity intact.

## Why This Exists

768's uncommitted Step 5 direct `LabelAddrExpr` producer creates a
function-owned native direct constant and its focused proof passes. A fresh
broader `^frontend_lir_` guard instead regresses `frontend_lir_call_type_ref`
(6/7) with `LirStoreOp.val: must not be empty`. Reverting its local-declaration
`stmt.cpp` authority change leaves the same failure: nested automatic
`void *table[] = { &&first, &&second };` lowering through the
coordinator/initializer-list route loses the direct operand. This is an
automatic local initializer emission gap, not a direct-rvalue producer,
carrier, verifier, Raw-BIR, or backend gap.

## In Scope

- Trace and repair the generic automatic local initializer-list/coordinator
  route needed for a nested label-address table element to retain its existing
  structured native direct-constant operand authority through emission.
- Make the smallest generic direct consumer change only if demonstrably
  required by that automatic initializer route; preserve function, target
  label, pointer type, and produced-value identity.
- Add direct focused frontend-LIR positive proof for automatic local
  `void *table[] = { &&first, &&second };`-style initialization and nearby
  malformed coverage for lost, invalid, foreign, or non-pointer structured
  authority at the owned boundary.
- Prove with a fresh build, the dedicated focused frontend-LIR test, and the
  direct `^frontend_lir_` guard that exposed this route.
- Return 768 exactly to Step 5 after acceptance; preserve Steps 1--4 and
  770's closed native-constant contract.

## Out Of Scope

- Automatic-table `DeclRef` decay-to-GEP implementation, table-decay redesign,
  broad rvalue/table/initializer redesign, or local-object authority work.
- `IndirBrStmt`/`LirIndirectBrOp.addr_value` carrier publication, verifier
  relaxation, external integration, 764, Raw-BIR/importer, backend, MIR, or
  codegen work.
- Reopening 767 or 769; synthetic bridges, printed/raw-text recovery,
  testcase-shaped routing, expectation downgrades, or accepting the
  uncommitted 768 Step 5 packet.

## Acceptance Criteria

- Automatic local nested label-address table initialization preserves the
  existing structured direct constant with valid function, target-label,
  pointer-type, and produced-value identity until its owned emission consumer.
- Direct focused frontend-LIR positive coverage and nearby malformed cases
  reject loss or invalid authority without weakening unrelated pointer
  contracts.
- A fresh build, focused proof, and `^frontend_lir_` guard pass; the prior
  `frontend_lir_call_type_ref` empty-store-value failure no longer occurs.
- No carrier, backend, Raw-BIR, verifier-relaxation, automatic-table decay, or
  broader rvalue/table redesign is claimed; 768 returns exactly to Step 5.

## Reviewer Reject Signals

- Reject a synthetic `select`, `gep`, `bitcast`, dummy instruction, or other
  fabricated identity bridge in place of structured direct-constant passage.
- Reject raw/printed-label parsing, rendered-text probes, testcase-name
  branching, expectation downgrades, or a targeted `call_type_ref` patch that
  does not establish generic initializer authority.
- Reject carrier/verifier/Raw-BIR/backend changes, 764 work, 767/769 reopening,
  automatic-table `DeclRef` decay, or broad rvalue/table rewrites presented as
  progress.
- Reject tests that prove only a named external case, omit malformed
  structured-authority coverage, or retain the empty `LirStoreOp.val` failure
  behind a renamed abstraction.
