Status: Active
Source Idea Path: ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Producer Boundary And Decide Split

# Current Packet

## Just Finished

Step 1 - Inspect Producer Boundary And Decide Split completed inspection of
the 20-row scalar/signature/control lane and found that the lane must be split
before implementation.

Row evidence:
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_rows.md`
  records 10 `scalar-control-flow semantic family` rows, 9
  `function-signature semantic family` rows, and 1 `scalar-binop semantic
  family` row.
- Scalar-control-flow representatives include `src/20000314-3.c` function
  `attr_eq`, plus `src/20080502-1.c`, `src/930614-1.c`, `src/980604-1.c`,
  `src/ieee/fp-cmp-8*.c`, `src/ieee/pr38016.c`, `src/pr35456.c`, and
  `src/pr39501.c`.
- Function-signature representatives include `src/20050316-3.c` function
  `test1`, plus `src/20071029-1.c`, `src/ieee/pr72824-2.c`, `src/pr60960.c`,
  `src/pr70903.c`, `src/pr71626-1.c`, `src/pr71626-2.c`, `src/simd-6.c`, and
  `src/zero-struct-2.c`.
- The scalar-binop row is `src/960513-1.c` function `f`.

Code evidence and boundary decision:
- All three topics share the same outer BIR admission/reporting funnel:
  `BirFunctionLowerer::note_function_lowering_family_failure`,
  `latest_function_failure_note`, and `lower_module` in
  `src/backend/bir/lir_to_bir/module.cpp` create the visible
  `semantic lir_to_bir failed outside ... latest function failure` module note.
  This is only a failure publication boundary, not one semantic producer.
- Function-signature failures are produced before block/instruction lowering in
  `BirFunctionLowerer::lower()` when `infer_function_return_info()` fails or
  `lower_function_params()` fails. The concrete parameter producer is
  `lower_function_params_with_layouts()` in
  `src/backend/bir/lir_to_bir/call_abi.cpp`.
- Scalar-control-flow failures are produced by CFG/terminator/phi lowering in
  `BirFunctionLowerer::lower()`, `collect_phi_lowering_plans()`,
  `lower_block_phi_insts()`, `initialize_aggregate_phi_state()`,
  `apply_pending_aggregate_phi_copies()`, and `lower_block_terminator()` in
  `src/backend/bir/lir_to_bir/module.cpp`.
- Scalar-binop failures are produced inside the instruction lowering boundary:
  `lower_scalar_or_local_memory_inst()` in
  `src/backend/bir/lir_to_bir/memory/coordinator.cpp` calls the scalar helpers
  in `src/backend/bir/lir_to_bir/scalar.cpp`, including
  `lower_scalar_binary_opcode()` and `lower_scalar_binop_operands()`, before
  emitting `bir::BinaryInst`.

Decision: split before implementation. The shared admission note is not a
shared repairable semantic producer. The independent implementation boundaries
are function-signature lowering, scalar-control-flow/CFG lowering, and
scalar-binop instruction lowering.

## Suggested Next

Ask the plan owner to split this active lifecycle state into separate
function-signature, scalar-control-flow, and scalar-binop producer ideas before
any implementation packet is delegated.

## Watchouts

- Do not implement this combined runbook as one repair route; doing so would
  conflate independent BIR producers behind a shared diagnostic funnel.
- Do not route function-signature rows to ABI/RV64 lowering before proving BIR
  publication correctness.
- Do not claim progress through expectation rewrites, unsupported downgrades,
  allowlist changes, or named-case shortcuts.
- The single scalar-binop row should not be used as broad scalar progress.

## Proof

Inspection-only lifecycle packet; no build or test proof required. AST-backed
function lookup was attempted, and targeted source/doc inspection supplied the
boundary evidence. No `test_after.log` was produced because the delegated proof
explicitly required no build/test proof.
