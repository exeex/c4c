Status: Active
Source Idea Path: ideas/open/95_aarch64_calls_immediate_scalar_argument_publication_owner.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce The Immediate Scalar Publication Owner

# Current Packet

## Just Finished

Completed plan Step 2: `Introduce The Immediate Scalar Publication Owner`.

Introduced local `ImmediateScalarCallArgumentPublicationOwner` in
`src/backend/mir/aarch64/codegen/calls.cpp` and routed the existing same-block
immediate cast call-argument publication instruction through
`ImmediateScalarCallArgumentPublicationOwner::instruction`.

The owner now owns only the immediate cast publication boundary: same-block
cast lookup consumption, immediate integer bit extraction, GP/FP destination
view rendering for immediate publication, FP immediate scratch materialization,
inline-asm line construction, and the assembler `MachineInstruction` record
used for immediate cast publication. Existing instruction fields, effects,
inline-asm spelling, selection status, and call-site predicates were preserved.

## Suggested Next

Execute Step 3 by confirming every in-scope immediate scalar publication use is
now routed through `ImmediateScalarCallArgumentPublicationOwner`, and document
any remaining immediate-looking code that intentionally stays outside the owner.

## Watchouts

- `lower_before_call_immediate_binding` remains the prepared immediate
  consumer and still owns ordinary before-call immediate register moves, stack
  immediate stores, prepared argument lookup, destination register authority,
  synthetic move construction, diagnostics, and ordinary `CallBoundaryMove`
  construction.
- `materialize_integer_constant_lines` remains generic constant materialization
  outside the owner; the owner only consumes it for immediate cast publication.
- Aggregate/byval transport, f128 carriers, ordinary before-call moves, stack
  immediate stores, and generic record construction remain outside this owner.
- The same-block cast lookup stayed local to the owner and still searches only
  before the current instruction in the current BIR block for the already
  prepared source-home value name.

## Proof

Ran exact delegated proof:

```bash
cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_target_instruction_records|backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|backend_codegen_route_aarch64_scalar_fp_literal_add_publishes_fpr_result)$' --output-on-failure > test_after.log
```

Result: build passed and 7/7 focused tests passed.

Proof log path: `test_after.log`.

Additional check: `git diff --check` passed.
