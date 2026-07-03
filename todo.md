Status: Active
Source Idea Path: ideas/open/570_rv64_unsupported_instruction_fragment_owner_diagnostics.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Review And Close Readiness

# Current Packet

## Just Finished

Step 4 converted the Step 3 enriched diagnostics into durable focused
follow-up ideas under `ideas/open/`:

- `ideas/open/571_rv64_inline_asm_carrier_lowering.md`
- `ideas/open/572_rv64_same_module_call_result_lowering.md`
- `ideas/open/573_rv64_select_phi_select_lowering.md`
- `ideas/open/574_rv64_floating_point_binary_lowering.md`
- `ideas/open/575_rv64_pointer_arithmetic_lowering.md`

Each idea records goal/intent, why it exists, in-scope work, out-of-scope work,
acceptance criteria, and concrete `## Reviewer Reject Signals`. The split keeps
inline asm carriers, ordinary call/result lowering, select/phi-select
materialization, FP binary lowering, and pointer arithmetic lowering as separate
initiatives. No idea mixes unrelated RV64 lowering, prepared authority, BIR
producer, inline-asm, call-boundary, or runtime work.

No implementation files, tests, `plan.md`, runtime comparison files,
unsupported markers, allowlists, or the 570 source idea were changed.

## Suggested Next

Proceed to Step 5 close-readiness review for the active diagnostic plan.
Confirm the source idea's acceptance criteria are satisfied by the diagnostic
patch, Step 3 evidence artifact, and Step 4 follow-up split, then decide
whether this plan is ready for close-gate validation.

## Watchouts

- This packet is evidence-only; the nine object-route failures remain expected
  unsupported lowering diagnostics.
- The five new follow-up ideas are implementation candidates, not work folded
  into the active diagnostic plan.
- `src/pr78438.c` previously looked like an arithmetic/shift family from source
  shape, but the first enriched object fallback is the entry `llvm.inline_asm`
  call, so routing it to shift lowering would be premature without bypassing or
  implementing inline asm handling first.
- `src/20000605-1.c` points at `BinaryInst; owner=double %t5`, specifically a
  double FP division in prepared BIR, before later FP casts.
- `src/20000819-1.c` points at `BinaryInst; owner=ptr %t4`, a pointer add of a
  loaded base and scaled byte offset.
- Do not add named-case matching, expectation downgrades, unsupported marker
  changes, allowlist changes, or runtime comparison changes.

## Proof

Lifecycle-only proof command:
`git diff --check -- ideas/open/571_rv64_inline_asm_carrier_lowering.md ideas/open/572_rv64_same_module_call_result_lowering.md ideas/open/573_rv64_select_phi_select_lowering.md ideas/open/574_rv64_floating_point_binary_lowering.md ideas/open/575_rv64_pointer_arithmetic_lowering.md todo.md`

Result: passed.

Step 3 evidence remains in:
`build/agent_state/570_unsupported_instruction_fragment_diagnostics/`.
