# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.28
Current Step Title: Publish direct scalar floating call-result authority

## Just Finished

- Stopped Plan Step 7.27 without code changes and classified `.insn r` input
  publication as an exact separately owned blocker.
- Native R operand indices identify rd/rs1/rs2 positions, but do not say whether
  rs1/rs2 must carry IDs. Legitimate raw-compatible local/body-parameter inputs
  have the same native metadata, roles, types, counts, and positions as the
  focused authoritative-source route after those IDs are erased.
- Requiring IDs from shape would overreject legitimate compatibility; allowing
  absence cannot reject the focused missing-authority mutations. Truthful
  publication requires a native input-authority expectation/source-capability
  carrier owned by a separate future semantic-schema initiative.
- Step 7 remains active: a fixed direct scalar floating result call is an
  unclaimed production disposition whose direct callee, exact signature/return
  type, result definition, and later floating use are already native and
  carrier-ready.
- Previously recorded parity, inline-asm, and producer-specific coercion
  blockers remain exact and are not reopened.

## Suggested Next

- Executor: complete Plan Step 7.28 for only one zero-argument fixed
  nonvariadic direct call returning `double`. Allocate its `LirCallOp.result`
  through `fresh_value`, preserve the exact direct callee/signature/return
  authority, and carry that exact result ID into one later ordinary double
  FAdd use.

## Watchouts

- Own only PC's zero-argument direct `double` result route. Require one native
  result definition, module-owned direct-callee `LinkNameId`, exact nonvariadic
  zero-parameter signature, and matching double call/return type refs.
- Preserve the exact call-result-to-FAdd ID/type edge. Reuse the accepted direct
  scalar call-result allocator and Step-7.5 floating BinOp ownership/type seam;
  do not create a floating-call-specific value model.
- Rendered callee/call/result/use spelling, raw signature/type mirrors, and
  instruction order may observe native facts but never create, repair, select,
  or validate identity or type authority.
- Reachable verification must reject invalid/duplicate/missing results;
  unknown or cross-function uses; unresolved/conflicting direct callee IDs;
  variadic or wrong parameter counts; wrong signature/call return types; and
  nonfloating opcode, operand, or FAdd type conflicts. Misleading displays with
  unchanged native facts must pass.
- Exclude accepted direct integer result calls, arguments, intrinsic/builtin
  calls, indirect/variadic/ABI-expanded calls, float-to-double conversions,
  pointer/vector/complex/aggregate/object returns, other floating operations,
  inline assembly, CFG/parameters, BIR receipt, and idea-741 changes.
- This packet is carrier-ready only on the existing direct callee/signature/
  result carrier, `fresh_value`, exact double type refs, floating BinOp seam,
  and generic ownership. If the route needs a new return-purpose carrier or
  text-derived bridge, stop and return that exact blocker rather than widening
  Step 7.28.

## Proof

- Fresh `cmake --build --preset default`.
- Focused `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure`
  with a zero-argument direct-double-call-to-FAdd chain,
  misleading-display positives, and malformed result/use, callee, signature,
  count/variadic, return-type, opcode, operand, and use-type cases.
- Preserve Step-3 direct integer calls, Step-7.5 floating arithmetic, nearby
  call signature/verifier coverage, and accepted generic ownership unchanged;
  the supervisor owns matched regression logs and broader/full proof.
- Run `git diff --check` before handoff.
