# Current Packet

Status: Active
Source Idea Path: ideas/open/415_prepared_value_materialization_contracts.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Producer-Side Verification

## Just Finished

Completed Step 3 by adding producer-side verifier classification and reports
for `PreparedCallArgumentBinaryProducerMaterializationFact`.

Missing binary producer materialization facts now report
`producer_missing`; cross-family producer kind, stale instruction linkage,
contradictory destination/opcode/operand payloads, unsupported opcode/type, and
other malformed non-null facts report `producer_incoherent`. Coherent reports
use `value_materialization_fact`, preserve destination value identity, and omit
diagnostic detail under the existing prepared contract verifier style.

Focused verifier coverage was added for coherent, missing, cross-family, stale,
unsupported, contradictory opcode, contradictory operand, and missing scheduling
authority cases. The value materialization contract notes now document the Step
3 binary-producer status taxonomy.

## Suggested Next

Begin Step 4 by migrating the final AArch64 scalar call-argument binary
producer materialization consumer to the typed fact plus producer-side verifier
contract, without adding target-local expression recovery.

## Watchouts

- Do not add target-local evaluators for arbitrary BIR expressions.
- Do not weaken runtime comparison, gcc_torture, or supported-path tests.
- `PreparedCallArgumentBinaryProducerMaterializationFact` remains a typed
  compatibility view over existing prepared same-block source-producer records;
  Step 4 should consume the verifier rather than reopen producer lookup shape.
- Route 6-only duplicate/ambiguous facts remain target-side evidence and should
  not be treated as prepared fact coherence without prepared fallback.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prealloc_prepared_contract_verifier|backend_aarch64_instruction_dispatch)$' > test_after.log 2>&1`

Result: passed. Proof log: `test_after.log`.
