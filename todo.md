# Current Packet

Status: Active
Source Idea Path: ideas/open/741_lir_structured_operand_and_terminator_identity_decomposition.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement the narrowest generic carrier

## Just Finished

- Plan Step 5 packet 4 implements CC-GEP-1 for ordinary selected-global array
  decay.
- `LirGepIndex` is the single typed-or-raw index carrier. Authoritative form
  owns `LirTypeRef` plus `LirOperand`; explicit raw presentation preserves
  unowned producers without parsing or a parallel mirror. The printer renders
  typed facts directly.
- The focused producer allocates its GEP result with `fresh_value`, attaches
  the selected global's `LinkNameId`, and emits two typed native i64 zero
  indices. Other GEP producers remain raw monostate compatibility.
- The verifier structurally gates authoritative GEPs, resolves global base
  ownership, validates result/index alternatives and immediate
  representability, and includes authoritative SSA indices in function-local
  ownership checks.

## Suggested Next

- Execute the next Step 5 packet for CC-RET-1, populating only the bounded
  scalar return type/value authority and its owned verifier coverage.

## Watchouts

- Existing raw GEP initializer sites construct raw `LirGepIndex` compatibility
  directly. The backend adapter parses only raw presentation; authoritative
  indices copy native type/operand facts directly, resolve native immediates
  before text, and never use authoritative SSA display aliases. It adds no new
  semantic receipt and the GEP importer boundary remains unchanged.
- Any authoritative GEP fact activates the complete contract: native result,
  uniquely owned global base, and a nonempty all-typed index list. Mixed raw
  indices reject rather than being parsed or inferred from presentation.
- Return authority remains unpopulated. CC-RET-1 must not infer value identity
  from `%tN` or parse return text, and new-BIR receipt remains a later step.

## Proof

- `cmake --build --preset default` passed.
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$'
  --output-on-failure` passed 1/1 with typed producer/printer inspection,
  neighboring extent and SSA-index coverage, misleading displays, the full
  malformed GEP matrix, and retained store/load coverage.
- `backend_lir_to_bir_interface` passed 1/1 with direct structured adaptation,
  misleading immediate display, raw compatibility, and authoritative-SSA
  no-display-alias coverage.
- Focused `--codegen llvm` retained `%t0 = getelementptr [1 x i32], ptr
  @lir_identity_array, i64 0, i64 0` with no preceding cast. All four focused
  `--dump-bir` probes retained their Step 3 importer boundaries: store/load/GEP
  are
  `UnsupportedOrdinaryInstruction`; scalar return is `InvalidVoidReturn`.
- Exact full proof `ctest --test-dir build -j --output-on-failure >
  test_after.log` passed 3033/3033, matching `test_before.log`. `git diff
  --check` passed.
