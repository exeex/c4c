Status: Active
Source Idea Path: ideas/open/842_lir_restricted_first_class_value_unions.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add the restricted boundary union carrier

# Current Packet

## Just Finished

Completed plan.md Step 1 inventory for first-class boundary carriers.

Current carriers and receipt points:

- Call argument/result uses `LirCallOp.return_type`, `arg_type_refs`,
  `callee_signature`, and `structured_args`; helpers in
  `call_args_ops.hpp` build/parse the argument mirrors, verifier checks
  call return/signature/argument refs, printer renders through
  `format_lir_call_site`, and call operands are visited through
  `structured_args` when complete.
- PHI uses `LirPhiOp.type_str` plus `LirPhiIncoming.value`; verifier checks
  `type_str` with `require_module_type_ref`, requires non-empty incoming
  entries, checks incoming operand kinds and current-function value authority,
  and separately verifies predecessor/successor occurrence authority. Printer
  renders the single type once and each incoming value/label pair.
- Select uses `LirSelectOp.type_str`, `cond`, `true_val`, and `false_val`;
  verifier checks the type ref and value operands, with an existing integer
  authoritative path that requires native result authority and comparison
  condition evidence. Printer renders the one type for both arms.
- Return uses `LirRet.type_str`, optional `value_str`, and selected
  `return_value_parameter_authority`; verifier permits void/no-value,
  non-void/value, authoritative integer immediates or SSA values, and the
  direct-scalar parameter return tuple. Printer renders `ret void` or
  `ret <type> <value>`.

Selected Step 2 boundary target: PHI value boundary. Add a minimal
PHI-local restricted value type carrier for `LirPhiOp.type_str`, attach it as a
compatibility mirror first, and prove scalar integer/floating, vector,
aggregate, and pointer alternatives while rejecting function, void, opaque,
metadata-like/runtime text, and unbounded payload cases.

## Suggested Next

Execute plan.md Step 2 for PHI only: introduce the restricted PHI boundary
union carrier and checked construction/access APIs, keep `LirPhiOp.type_str`
as a compatibility mirror, and add focused verifier coverage for admitted and
wrong-kind alternatives.

## Watchouts

Admitted matrix for the selected PHI boundary:

- Scalar: integer and floating type refs are admitted; reuse the 841 compact
  scalar evidence as family precedent without coupling PHI to `LirBinOp`.
- Vector: admitted when represented as a bounded vector `LirTypeRef` already
  accepted by `require_module_type_ref`.
- Aggregate: admitted for structured struct/union/anonymous aggregate refs that
  pass existing module aggregate/known-struct checks.
- Pointer: admitted as an evidenced pointer alternative because current PHI
  producers include pointer PHIs, including VA/source-pointer paths.

Wrong-kind exclusions for Step 2:

- Reject function type refs, void PHI value types, opaque/runtime-text-only
  refs, metadata-like non-value refs, partially parsed call-signature text, raw
  `args_str` payloads, and any universal value bag/generic ID/RTTI adapter.
- Do not migrate call, select, or return in the PHI carrier packet.
- Do not delete `LirPhiOp.type_str`; deletion waits for later named consumer
  migration and Step 4.

Missing evidence: none blocking for PHI Step 2. Call remains larger because
argument/result/signature mirrors and raw compatibility parsing are intertwined;
select and return are viable later targets but have narrower existing scalar
authority assumptions that should not shape the first union API.

## Proof

Inventory proof command: `git diff --check`.

Suggested focused Step 2 proof command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'; } > test_after.log 2>&1`.
