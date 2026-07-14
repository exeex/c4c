# LIR-to-BIR Native Label-Address Constant Contract

Status: Closed (reopened in-scope repair blocker for
`ideas/open/768_lir_computed_goto_label_address_table_initialization_authority_decomposition.md`
Step 5)
Type: focused LIR-to-BIR/native label-address constant representation and lowering contract
Predecessor: 768 Step 5 direct `LabelAddrExpr` rvalue production

## Goal

Define and prove the smallest native LIR-to-BIR representation and lowering
contract for a direct label-address pointer constant that retains its produced
`LirValueId` identity and can be consumed by an indirect jump without a
synthetic instruction bridge.

## Why This Exists

768 selected direct frontend-LIR `LabelAddrExpr` rvalue production and rejected
its `select i1 true, blockaddress(...), blockaddress(...)` implementation as a
synthetic identity bridge. LLVM 19 accepts `blockaddress(@f, %target)` as a
pointer constant in direct contexts, but rejects `%x = blockaddress(@f,
%target)` because `blockaddress` is not an instruction opcode. The present LIR
operand authority exposes only value, global, and integer alternatives. The
LIR-to-BIR scalar route accepts only SSA, null, and global pointer lowering,
and indirect-jump lowering requires `LirIndirectBrOp.addr_value` to resolve to
a source SSA pointer while `IndirectJumpTerm` carries only `ValueId`.

A native direct identity therefore requires a bounded representation/lowering
contract below the producer; it cannot be supplied by `select`, `gep`,
`bitcast`, rendered-text recovery, or another fabricated SSA value. Raw-BIR,
importer, and broader backend ownership are outside 768 and are not authorized
here.

## In Scope

- Define one native direct label-address constant representation that carries
  the enclosing-function and target-label identity plus the produced
  `LirValueId` needed by the direct `LabelAddrExpr` producer.
- Define the exact LIR-to-BIR lowering/consumption contract that preserves that
  identity as a pointer value suitable for legal direct pointer consumers,
  including `LirIndirectBrOp.addr_value`/`IndirectJumpTerm` and the ordinary
  `LirStoreOp` pointer initializer value operand, without requiring an
  instruction-shaped SSA bridge.
- Implement only the representation and lowering surfaces demonstrably needed
  for that contract, including the minimal Raw-BIR/importer passage only where
  it is indispensable to carry this one native constant, and matching
  validation at the owned boundary.
- Add direct focused positive proof for the native constant route and nearby
  malformed proof for invalid/missing/foreign function, target, or produced
  identity and invalid pointer use, as the final representation requires.
- Record the exact post-acceptance return to 768 Step 5: restore its direct
  frontend-LIR producer packet using this native contract, then run its
  already-selected focused producer proof.

## Out Of Scope

- 768 producer recovery, frontend carrier publication, or its focused
  `LabelAddrExpr` test/proof changes except for the minimal consumer fixture
  needed to prove this contract.
- `IndirBrStmt` or `LirIndirectBrOp.addr_value` publication policy changes,
  external integration cases, automatic-table `DeclRef` decay, table decay,
  and any 764 downstream carrier work.
- Reopening 767 or 769, or changing their accepted table-element or global
  initializer contracts.
- 734 work, broad Raw-BIR/importer expansion, broad backend/MIR/codegen
  redesign, or backend/case work beyond the direct LIR-to-BIR lowering
  boundary.
- Text recovery, testcase-shaped behavior, expectation downgrade, and every
  synthetic `select`, `gep`, `bitcast`, or comparable fabricated value route.

## Acceptance Criteria

- The representation distinguishes a direct label-address pointer constant
  from an instruction result while preserving its function, target label, and
  produced `LirValueId` authority.
- LIR-to-BIR lowering preserves that native constant as an identity-bearing
  pointer value that an indirect jump and ordinary pointer store can consume;
  it does not require LLVM to parse `blockaddress` as an opcode or manufacture
  an SSA bridge.
- Direct focused positive and malformed proofs cover the owned contract; a
  malformed authority or pointer-use form is rejected at the selected boundary.
- The result names whether any Raw-BIR/importer/backend work remains beyond
  the minimal native-constant passage. Such work must become a separately
  scoped blocker rather than enter this initiative.
- 768 resumes exactly at Step 5 with its Steps 1--4 preserved and only its
  native direct producer recovery remaining.

## Reviewer Reject Signals

- Reject `select`, `gep`, `bitcast`, dummy instruction results, or any other
  fabricated SSA identity presented as a native label-address value.
- Reject printed `blockaddress(...)` parsing, text recovery, testcase-name
  routing, expectation downgrades, verifier relaxation, or a test-only facade
  that retains the old no-native-constant failure.
- Reject a Raw-BIR/importer change broader than the selected native-constant
  passage, or any 734, broad backend, carrier, external-case, table-decay,
  767, or 769 change folded into this initiative.
- Reject a representation that loses the current function, target label, or
  produced-value identity, or a lowering route that cannot directly feed the
  specified indirect-jump consumption contract.

## Closure Record — 2026-07-14

Disposition: capability complete for this bounded native LIR-to-BIR contract.

- Implementation commit `e8a0f70b4` establishes the function-owned,
  non-instruction direct label-address constant, its direct-operand printing
  contract, the minimal Raw-BIR/importer passage, verifier checks, and existing
  `IndirectJumpTerm` consumption.
- Proof-state commit `6803f8c25` records the accepted direct fixture:
  native constant to Raw-BIR source value to `IndirectJumpTerm`, plus malformed
  rejection for invalid, missing, duplicate, or foreign authority/value forms,
  non-pointer type, and mismatched direct use/address.
- Supervisor acceptance evidence is a fresh
  `cmake --build --preset default` followed by
  `ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_interface$'`,
  both passing.
- No producer recovery, frontend carrier publication, external integration,
  automatic-table decay, or other downstream capability is claimed here.

Return: resume
`ideas/open/768_lir_computed_goto_label_address_table_initialization_authority_decomposition.md`
at Step 5 — Repair and prove native direct `LabelAddrExpr` rvalue production.
Its Steps 1--4 remain accepted; replace only the rejected synthetic bridge
using this completed native direct-constant contract.

## Reopen Record — 2026-07-14 Ordinary Pointer-Store Consumer Gap

Disposition: reopen this source as an **in-scope contract repair**, not a new
blocker. The completed contract expressly supplies the `LirValueId` required
by 768's direct `LabelAddrExpr` producer, so its legal ordinary pointer-store
consumer is part of the same native direct-constant representation/lowering
promise. The first implementation/proof only demonstrated direct
constant -> Raw-BIR source value -> `IndirectJumpTerm` consumption.

- New boundary evidence: 768 Step 5 needs
  `LirOperandKind::DirectConstant` as `LirStoreOp.val`. The LIR printer's store
  value operand does not admit that kind, and generic function value-use
  verification requires its ID to name an instruction definition even though
  this representation is function-owned and non-instruction.
- Remaining owned repair: admit and render the structured direct constant in
  the ordinary pointer-store value operand; make value-use verification
  recognize its valid function-owned direct definition; and add focused
  positive/malformed coverage for the store route. Preserve all first-closure
  direct indirect-jump evidence.
- Boundaries: no 768 producer recovery, carrier publication, external cases,
  automatic-table decay, 767/769, scalar/broad backend redesign, or synthetic
  instruction/value bridge. A need beyond legal direct-constant ordinary store
  consumption is a separately scoped blocker.
- Return: after fresh build and focused positive/malformed store-consumer
  proof accept this repair, return to 768 Step 5 and retry its producer/test
  packet unchanged.

## Closure Record — 2026-07-14 Reopened Store-Consumer Repair Accepted

Disposition: capability complete for the bounded native label-address constant
contract, including its legal ordinary pointer-store consumer.

- `e8a0f70b4` establishes the function-owned non-instruction direct constant,
  direct Raw-BIR source-value passage, and `IndirectJumpTerm` consumption.
- `6803f8c25` records the accepted initial focused positive/malformed proof.
- `0a2778f0d` adds the structured `LirStoreOp.val` direct-constant admission,
  printing, function-owned definition authority, and focused positive/malformed
  ordinary pointer-store coverage. It preserves the accepted indirect-jump
  route and does not perform producer recovery.
- Supervisor-accepted evidence is a fresh `cmake --build --preset default`
  followed by `ctest --test-dir build -j --output-on-failure -R
  '^backend_lir_to_bir_interface$'`, passing with matching retained root logs.

No producer recovery, frontend carrier publication, automatic-table decay,
external integration, or reopening of 767/769 is claimed. Resume 768 exactly
at Step 5 — Repair and prove native direct `LabelAddrExpr` rvalue production;
its Steps 1--4 remain accepted.
