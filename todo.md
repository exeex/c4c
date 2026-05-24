Status: Active
Source Idea Path: ideas/open/value-home-storage-interpretation-prealloc.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Adapt AArch64 To Consume Prealloc Interpretation

# Current Packet

## Just Finished

Completed `Step 3: Adapt AArch64 To Consume Prealloc Interpretation`.

Updated AArch64 value-operand resolution to consume
`prepare::decode_prepared_home_storage()` instead of independently
interpreting raw regalloc, storage-plan, and value-home records.

The AArch64 adapter now:

- maps `PreparedDecodedHomeStorage` facts to `ResolvedOperand`;
- keeps `abi::convert_prepared_register()`, MIR physical-register, memory,
  immediate, and symbol operand construction in AArch64 codegen;
- preserves AArch64-local diagnostic selection and messages for missing typed
  register authority, unsupported storage plans, unsupported value homes, and
  missing value authority;
- preserves regalloc, storage-plan, value-home precedence, including the rule
  that a present higher-precedence decoded authority blocks lower-precedence
  fallback.

Added focused `backend_aarch64_operand_resolution` coverage for present empty
regalloc authority blocking storage-plan fallback and storage-plan `None`
authority blocking value-home fallback.

## Suggested Next

Execute the next coherent packet by adapting x86 prepared operand/lowering
reuse to consume or expose the prealloc decoded home/storage API where it
removes duplicated target-neutral interpretation, while keeping x86-specific
text/encoding and legality decisions in x86 code.

## Watchouts

- The AArch64 adapter intentionally maps decoded facts to MIR operands locally;
  prealloc still does not construct MIR operands or target registers.
- Present-but-empty regalloc or storage-plan authority now stops fallback in
  AArch64 through the decoded API; future target adapters should preserve the
  same precedence contract.
- Diagnostics are preserved as closely as possible, but regalloc register
  assignments that are present without typed placement now diagnose as missing
  typed register authority instead of falling through to lower authority.

## Proof

Ran exactly:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed. `test_after.log` reports `100% tests passed, 0 tests failed out
of 151`; `backend_aarch64_operand_resolution` passed with the new precedence
coverage.
