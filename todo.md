Status: Active
Source Idea Path: ideas/open/564_bir_scalar_local_memory_semantic_admission_followup.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Decide Unordered Floating Compare Representation

# Current Packet

## Just Finished

Step 5 - Decide Unordered Floating Compare Representation completed as a
representation decision packet. Existing BIR comparison representation is the
single `bir::BinaryOpcode` enum plus `BinaryInst`/`CompareExpr`, and its compare
members are `Eq`, `Ne`, signed integer relations, and unsigned integer
relations only. Consumers treat those opcodes as directly evaluable or
emittable comparison predicates:

- `bir::is_compare_opcode()` / `is_comparison_binary_opcode()` classify only
  `Eq`, `Ne`, `Slt`, `Sle`, `Sgt`, `Sge`, `Ult`, `Ule`, `Ugt`, and `Uge`.
- `route7_comparison_instruction_record()` records a compare by copying
  `BinaryInst::opcode` into `Route7ComparisonInstructionRecord::predicate`.
- `evaluate_comparison_integer_constant()` evaluates comparison opcodes by
  integer truth tables.
- RV64 object emission lowers BIR compare predicates to simple integer compare
  instruction sequences.

Decision: `fcmp uno` should remain an explicit fail-closed scalar/local-memory
owner-boundary rejection for now, not a new `BinaryOpcode` and not a lossy
mapping onto `Ne` or another existing ordered/integer-style opcode. The current
`lower_scalar_or_local_memory_inst()` guard for float `LirCmpPredicate::Uno`
already encodes that decision by stopping before generic scalar compare lowering
and reporting `unordered-float-compare scalar/local-memory semantic family`.
The focused `unordered_float_uno_compare` admission test already documents this
predicate-family behavior, while nearby `admitted_float_une_compare` confirms
that admitted unordered-family predicates are not hidden behind the generic
scalar/local-memory diagnostic.

No Step 6 code or test change was necessary in this packet: Step 6's current
behavior is already the selected explicit fail-closed rejection. A future change
to actually lower `fcmp uno` should introduce a structured unordered floating
compare representation/lowering path rather than extending the existing
integer-style `BinaryOpcode` contract with an opcode that current consumers
cannot evaluate or emit correctly.

## Suggested Next

Next executable packet can treat Step 6 - Lower Or Reject `fcmp uno` At The
Owner Boundary as already satisfied by the current narrow fail-closed rejection,
then move to Step 7 proof of the IEEE representatives unless the supervisor
wants an additional no-code Step 6 confirmation packet.

## Watchouts

- This packet intentionally made no source, test, expectation, unsupported
  marker, allowlist, or lowering-behavior changes.
- Do not add a raw `Uno` `BinaryOpcode` without updating all comparison
  classifiers, route records, constant evaluators, and target emitters that
  currently assume compare opcodes are simple integer-style predicates.
- Do not map `fcmp uno` to `Ne`; that would erase NaN/unordered semantics and
  would be a semantic overfit.
- `ord` and `ueq` remain nearby predicate-family questions; keep any future
  decision predicate-family based and covered by focused tests.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: pass. CTest reported `100% tests passed, 0 tests failed out of 346`.

Proof log: `test_after.log`.
