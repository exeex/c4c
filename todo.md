# Current Packet

Status: Active
Source Idea Path: ideas/open/741_lir_structured_operand_and_terminator_identity_decomposition.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement the narrowest generic carrier

## Just Finished

- Plan Step 5 packet 1 implements the generic structured-identity foundation.
- The single existing `LirValueId` definition now lives in shared LIR
  `identity.hpp`. `LirOperand` carries a closed monostate/value/symbol/integer
  authority variant with explicit factories, accessors, and semantic equality;
  legacy text constructors remain monostate compatibility.
- Definition lowering now owns a local `LirFunction` shell before `FnCtx`
  emission. `fresh_value` allocates from that exact shell and obtains display
  separately from `fresh_tmp`; no producer uses it until its focused packet.
- The verifier collects authoritative modern result definitions per function,
  rejects invalid/duplicate IDs, and resolves modeled authoritative uses only
  within that function. Coverage proves misleading display is ignored and the
  same numeric ID is independent across functions.

## Suggested Next

- Execute Plan Step 5 packet 2, CC-STORE-1: carry selected-global and native
  integer authority through the focused lvalue/rvalue/set/store route, then add
  its owned verifier and neighboring coverage.

## Watchouts

- `fresh_value` is foundation only and currently has no producer caller. Packet
  2 must not infer value or symbol authority from `fresh_tmp` or display text.
- The ownership use walk covers the direct modern operand roles already
  modeled by `verify_inst`. Raw PHI incoming pairs, raw GEP indices, raw
  terminators/targets, and semantic inline-asm ordinary-result definitions are
  intentionally not guessed; migrate them only with their checked rows.
- Focused store/load/GEP/return fields still accept monostate compatibility and
  none is populated by this packet. Their Step 3 failure families must remain
  the capability boundary until their owned packets.

## Proof

- `cmake --build --preset default` passed.
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$'
  --output-on-failure` passed 1/1 with factory, misleading-display, malformed
  ownership, and per-function namespace coverage.
- All four focused `--dump-bir --target x86_64-linux-gnu` probes retained their
  Step 3 first failures: store/load/GEP are `UnsupportedOrdinaryInstruction`;
  scalar return is `InvalidVoidReturn`.
- Canonical `test_before.log` and fresh `test_after.log` both report 3033/3033
  passing. `git diff --check` passed.
