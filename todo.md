Status: Active
Source Idea Path: ideas/open/209_aarch64_scalar_alu_cast_first_instruction_slice.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Document Scalar Record Contract And Close Readiness

# Current Packet

## Just Finished

Completed Step 6 from `plan.md`: documented the scalar ALU/cast target-record
contract and added close-readiness contract proof.

Concrete work completed:
- Extended `src/backend/mir/aarch64/codegen/records.md` with scalar-specific
  guardrails for supported ALU `Add`/`Sub`/`And`/`Or`/`Xor` records and
  supported cast `SExt`/`ZExt`/`Trunc` records.
- Documented deferred and fail-closed scalar boundaries for unsupported ALU
  opcodes, compare predicates, floating/pointer/broad cast forms, `i128`,
  `f128`, and incomplete prepared facts.
- Added `backend_aarch64_scalar_record_contract` proving scalar records stay
  `RecordOnly`, preserve prepared/BIR ids/types/opcodes, expose explicit
  supported/deferred vocabulary, and do not own memory/call/return/assembler/
  object payload behavior.

## Suggested Next

Route to the plan owner for lifecycle close decision on
`ideas/open/209_aarch64_scalar_alu_cast_first_instruction_slice.md`.

## Watchouts

- The source idea appears ready for close review: scalar ALU and cast record
  vocabulary, prepared conversions, documentation, and contract proof are in
  place.
- No assembler, object, memory, call, return, `module.cpp`, or `emit.hpp`
  ownership was added by this step.

## Proof

Proof passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') 2>&1 | tee test_after.log`

Result: backend subset passed with `backend_aarch64_scalar_record_contract`
included and green: 128 tests passed, 0 failed; 12 disabled MIR trace tests
were not run. Proof log path: `test_after.log`.
