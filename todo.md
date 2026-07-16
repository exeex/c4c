Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.41
Current Step Title: Receive The One 829-Authorized Body-Parameter Argument-1 Row

# Current Packet

## Just Finished

Activated 734 after closed 829 supplied the exact next body-parameter
authority handoff. Accepted 734 Steps 1 through 7.40 remain historical work;
Step 7.40 receiver commit `6609d92d4` must not be repeated.

## Suggested Next

Implement Step 7.41: receive only closed 829's current-function
`DirectScalar` parameter 1 tuple, used unchanged as fixed direct-call
argument 1 at `LirCallOp.structured_args[1]`, into typed Raw BIR with importer
dispatch, reachable verifier coverage, and transactional malformed-authority
tests.

## Watchouts

- Do not edit LIR producer/schema/verifier publication for this packet.
- Do not recover identity from text, names, rendered operands, diagnostics,
  signature strings, or compatibility mirrors.
- Do not receive any other body-parameter form, call argument index, generic
  call, variadic/indirect/unspecified call, ABI conversion, memory/VA,
  aggregate/vector, module/type/global/metadata, residual
  instruction/terminator, or inline-assembly family.
- Closed 829 makes no Raw-BIR receipt claim; this packet owns that receipt.

## Proof

Pending. Use a fresh build plus focused backend receiver proof and matching
before/after regression guard before acceptance.
