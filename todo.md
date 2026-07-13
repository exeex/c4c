# Current Packet

Status: Active
Source Idea Path: ideas/open/741_lir_structured_operand_and_terminator_identity_decomposition.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement the narrowest generic carrier

## Just Finished

- Plan Step 5 packet 2 implements CC-STORE-1 for ordinary scalar integer stores
  to selected globals.
- `AssignableLValue.ptr` now carries `LirOperand`; `emit_lval_operand` attaches
  the selected global's `LinkNameId` before string loss. `emit_rval_operand`
  attaches native non-complex HIR `IntLiteral` authority, and the focused
  set/coerce/store route rebuilds authority from the native payload when the
  structured source/destination value representations match.
- Unowned lvalue/rvalue routes use explicit raw monostate compatibility.
  Load/memset consumers take only `.str()`, so this packet does not populate
  load/GEP/return authority or claim their rows.
- The verifier now resolves direct-global store IDs to exactly one `LirGlobal`
  and validates authoritative integer-immediate representability. Malformed
  IDs, missing/wrong alternatives, ambiguity, and narrow overflow reject
  without interpreting display.

## Suggested Next

- Execute Plan Step 5 packet 3, CC-LOAD-1: allocate the focused load result with
  `fresh_value`, retain selected-global pointer authority through the
  coordinator, and add its owned verifier and neighboring coverage.

## Watchouts

- Integer authority survives set assignment only when structured
  source/destination LLVM value representations match. Post-coercion text
  supplies presentation only; changed representations fall back to raw
  monostate.
- A valid global ID paired with misleading display denotes the ID-owned global.
  Only invalid/unresolved, non-global/ownerless, or ambiguously owned IDs
  reject. Float, special-token, local-pointer, and other unowned store shapes
  retain phased compatibility.
- `fresh_value` remains unused by producers. CC-LOAD-1 must use the exact
  function shell allocator and must not infer result identity from `tmp_idx`.

## Proof

- `cmake --build --preset default` passed.
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$'
  --output-on-failure` passed 1/1 with normal producer inspection, zero/second
  global coverage, misleading integer/global displays, and the full malformed
  store matrix.
- Focused `--codegen llvm` retained `store i32 7, ptr
  @lir_identity_scalar`. All four focused `--dump-bir` probes retained their
  Step 3 importer boundaries: store/load/GEP are
  `UnsupportedOrdinaryInstruction`; scalar return is `InvalidVoidReturn`.
- Canonical `test_before.log` and fresh `test_after.log` both report 3033/3033
  passing. `git diff --check` passed.
