Status: Active
Source Idea Path: ideas/open/179_bir_return_chain_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Existing Consumers

# Current Packet

## Just Finished

Step 1 - Map Existing Consumers completed the AArch64 prepared return-chain
consumer map.

Mapped call sites:

- `src/backend/mir/aarch64/codegen/alu.cpp:1369` calls
  `prepare::find_prepared_return_chain_terminal_value` inside
  `find_return_chain_register`. Classification: terminal recovery. Direct
  caller: `lower_scalar_instruction` only. Current prepared key is
  `(context.block_index, instruction_index, result_home.value_name)`. Route 8
  needs `context.function.bir_function`, `*context.bir_block`,
  `instruction_index`, and the current chain `bir::Value` plus
  `result_home.value_name` for `bir::route8_return_chain_value_key`. The block
  and function context are available, but `find_return_chain_register` does not
  currently receive the chain `bir::Value`; the caller has it as
  `binary->result`. Migration should either form/query the route 8 key in
  `lower_scalar_instruction` or thread `binary->result` into
  `find_return_chain_register`. The returned route 8 terminal identity should
  feed the existing AArch64 value-home and return ABI register path.
- `src/backend/mir/aarch64/codegen/alu.cpp:4207` calls
  `prepare::find_prepared_return_chain_next_operand_value` inside
  `lower_scalar_instruction`. Classification: next-operand alias/scratch
  recovery. Current prepared key is
  `(context.block_index, instruction_index, result_home.value_name)` after
  `find_return_chain_register` has recovered a result register for a
  rematerializable-immediate result. Route 8 key context is already local:
  `context.function.bir_function`, `*context.bir_block`, `instruction_index`,
  `binary->result`, and `result_home.value_name`. The route 8 next-operand
  identity should replace only the source value-name lookup; AArch64 should
  keep the existing value-home lookup, register parsing, alias comparison,
  scalar view, scratch selection, and result-register retargeting policy.

No AArch64 diagnostic-only or unrelated prepared return-chain helper call sites
were found. `rg` found the two AArch64 helper reads above; clang direct-caller
queries confirmed `find_return_chain_register` is only consumed by
`lower_scalar_instruction`.

## Suggested Next

Execute Step 2 by adding the minimal AArch64 route 8 projection/query adapter:
build or reuse a `bir::Route8ReturnChainIndex`, form
`bir::route8_return_chain_value_key(context.function.bir_function,
*context.bir_block, instruction_index, binary->result,
result_home.value_name)`, and fail closed when the route 8 record is missing,
invalid, duplicate, or lacks the requested identity.

## Watchouts

- Keep AArch64 target policy out of BIR.
- Do not contract prepared return-chain APIs in this plan.
- Treat prepared helper reads as diagnostics or bounded fallback only after the
  matching consumer migrates.
- Preserve fail-closed behavior for missing, invalid, or conflicting BIR route
  answers.
- Route 8 has function-wide and block-local index builders plus terminal and
  next-operand identity query helpers in `src/backend/bir/bir.hpp`.
- `find_return_chain_register` currently lacks the actual chain `bir::Value`;
  avoid reconstructing it from prepared names. Use `binary->result` from the
  caller or explicitly thread that value.
- Focused later-packet proof commands identified:
  `cmake --build build --target backend_aarch64_return_lowering_test backend_prepared_lookup_helper_test`,
  `ctest --test-dir build -R '^(backend_aarch64_return_lowering|backend_prepared_lookup_helper)$' --output-on-failure`.

## Proof

Metadata/inspection-only packet; no build required and no `test_after.log`
produced by contract. Inspection used `rg`, `c4c-clang-tool-ccdb list-symbols`,
`c4c-clang-tool-ccdb function-signatures`,
`c4c-clang-tool-ccdb function-callers find_return_chain_register`,
`c4c-clang-tool-ccdb function-callees lower_scalar_instruction`, targeted file
reads, and `ctest --test-dir build -N` to identify focused later proof names.
