# Current Packet

Status: Active
Source Idea Path: ideas/open/801_lir_anonymous_aggregate_layout_type_facts.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Propagate native anonymous aggregate layout facts

## Just Finished

- Lifecycle switch from 754: Steps 1 and 2 are accepted; Step 3 has no code
  or test changes because anonymous aggregate layout facts are absent.
- Step 1 trace: `lir_call_type_ref` in `hir_to_lir/call/target.cpp` is the
  direct-complex anonymous-aggregate creation seam.  Its owner-key miss path
  returns the HIR-rendered `{ float, float }`/`{ double, double }` as a
  text-only `LirTypeRef`; `emit_call_with_result` stores that type in the
  direct `LirCallOp.return_type` while the structured callee signature owns
  its matching `return_type_ref`.  Unary real/imag lowering in
  `expr/misc.cpp` currently independently supplies `llvm_ty(op_ts)` as
  `LirExtractValueOp.agg_type`.  The existing verifier already requires an
  authoritative extract aggregate to identify the current call result and
  requires `call.return_type == extract.agg_type`.
- Existing structured facts are insufficient: `LirTypeRef` owns array element
  type/length and named aggregate identity (`StructNameId`), while named
  layouts live in `LirStructDecl.fields` on `LirModule`.  A bare anonymous
  struct is only `kind == Struct` plus compatibility text; equality and the
  verifier therefore cannot recover ordered field types without parsing that
  display text, which is forbidden by this idea.
- Selected minimum carrier: add an opt-in anonymous-aggregate layout to
  `LirTypeRef`, holding an ordered `std::vector<LirTypeRef>` of field types,
  with a factory that derives the compatibility/render text from those facts.
  Scope its initial construction to selected direct-complex return types and
  preserve the carrier through the call result, callee-signature return type,
  and unary extract aggregate type.  Do not add layout, result, or index data
  to `LirExtractValueOp`, change generic aggregate paths, or parse text.
- Required Step 2 checks: only the opt-in carrier may claim anonymous layout;
  every field must be a valid module type; rendering must be derived from the
  ordered fields rather than trusted text; and type equality/verification must
  retain the complete ordered carrier across call signature, call result, and
  authoritative extract.  The direct-complex constructor must supply its two
  component types in order; index bounds and selected-result-type validation
  remain the later 754 work.
- Candidate proof surfaces for later packets: frontend direct complex calls
  followed by `__real__`/`__imag__` should observe `{ component, component }`
  native fields on the call and matching extract aggregate type.  The nearby
  backend fixture `make_rv64_anonymous_aggregate_return_extractvalue_module`
  is the malformed-form surface: add or convert a typed anonymous carrier,
  then reject missing/malformed ordered fields, stale render mirrors, and
  call/signature/extract carrier disagreement.  These are carrier proofs, not
  testcase-shaped index/result validation.

## Suggested Next

- Step 2 only: implement and propagate the opt-in `LirTypeRef` anonymous
  ordered-field carrier through the selected direct-complex call/extract path,
  with its local verifier invariants.

## Watchouts

- Do not parse `LirTypeRef` display text or add `LirExtractValueOp` field,
  index, result, or Raw-BIR work; 754 resumes only after this handoff.

## Proof

- Step 1 trace-only packet: no code/build/test or root proof-log change.
  AST-backed seam queries covered `lir_call_type_ref` and `LirTypeRef`; the
  recorded source trace covers type creation, storage, verifier coherence,
  and candidate proof surfaces. `git diff --check` passed.
