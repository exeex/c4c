# Current Packet

Status: Active
Source Idea Path: ideas/open/741_lir_structured_operand_and_terminator_identity_decomposition.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement the narrowest generic carrier

## Just Finished

- Plan Step 5 packet 3 implements CC-LOAD-1 for ordinary direct scalar loads
  from selected globals.
- `emit_rval_operand` intercepts the exact selected `GlobalVar` before string
  loss, allocates the load result through `fresh_value` on the current function
  shell, and attaches the selected global's `LinkNameId` to the load pointer.
  The same authoritative result operand returns to structured callers.
- Aggregate, array-adjacent, unary-dereference, assignable read-modify-write,
  and ABI carrier loads remain explicit raw monostate compatibility. No GEP or
  return authority is populated by this packet.
- The verifier shares one display-independent global-owner resolver between
  store and load. Direct-global loads require a uniquely owned `LinkNameId`
  pointer and function-owned `LirValueId` result; foundation ownership checks
  reject invalid or duplicate result IDs.

## Suggested Next

- Execute Plan Step 5 packet 4, CC-GEP-1: populate only the bounded array-decay
  GEP result/base/index identities and add its owned verifier coverage.

## Watchouts

- `fresh_value` allocates load identity independently of the shared display and
  label counter. Numeric IDs remain function-local and may repeat across
  functions.
- A valid load pointer ID paired with misleading display denotes the ID-owned
  global. Invalid, unresolved, function-only, ownerless, and ambiguously owned
  IDs reject without rendered-name inspection.
- Global array decay remains in the CC-GEP packet. Its GEP result, base, and
  indices are still unpopulated; do not infer them from `%tN`, `@name`, or raw
  index strings. Return authority and new-BIR receipt also remain untouched.

## Proof

- `cmake --build --preset default` passed.
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$'
  --output-on-failure` passed 1/1 with normal producer inspection, two-load and
  cross-function allocation coverage, misleading displays, and the full
  malformed load matrix while retaining store coverage.
- Focused `--codegen llvm` retained `%t0 = load i32, ptr @g_counter`. All four
  focused `--dump-bir` probes retained their Step 3 importer boundaries:
  store/load/GEP are
  `UnsupportedOrdinaryInstruction`; scalar return is `InvalidVoidReturn`.
- Exact full proof `ctest --test-dir build -j --output-on-failure >
  test_after.log` passed 3033/3033, matching `test_before.log`. `git diff
  --check` passed.
