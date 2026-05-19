Status: Active
Source Idea Path: ideas/open/299_aarch64_scalar_immediate_materialize_or_encoding_fallback.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce and localize the focused printer failures

# Current Packet

## Just Finished

Step 1 reproduced and localized the focused AArch64 scalar immediate
machine-printer failures. The build was current, then the delegated focused
CTest subset failed all 8/8 cases with the shared diagnostic:
`cannot print AArch64 machine node family=scalar opcode=<add|xor|and>: scalar
add/sub/bitwise immediate operand is outside the plain #imm encoding range
0..4095`.

Current focused case modes:

- `00031`: fails in the machine-node printer as scalar `add`, function 0 block
  4 instruction 4. Prepared BIR shows pre/post decrement forms such as
  `bir.add i32 %t12, -1` and `bir.add i32 %t34, -1`; representative operand is
  register plus signed `-1`.
- `00104`: fails as scalar `xor`, function 0 block 0 instruction 4. Prepared
  BIR shows bitwise complement forms `bir.xor i32 %t1, -1` and
  `bir.xor i64 %t7, -1`; representative operand is register xor signed `-1`
  for `~x` / `~l`.
- `00143`: fails as scalar `add`, function 0 block 19 instruction 0. Prepared
  BIR includes Duff-device loop decrement `bir.add i32 %t33, -1` plus small
  positive pointer increments; representative failed form is register plus
  signed `-1`.
- `00207`: fails as scalar `add`, function 0 block 3 instruction 1. Prepared
  BIR shows `argc--` as `bir.add i32 %t5, -1`; representative operand is
  register plus signed `-1`.
- `00213`: fails as scalar `add`, function 0 block 1 instruction 7. Prepared
  BIR shows repeated loop/decrement forms including `bir.add i64 %t4, -1` and
  `bir.add i32 %t3, -1`; representative operand is register plus signed `-1`.
- `00214`: fails as scalar `xor`, function 0 block 4 instruction 0. Prepared
  BIR shows mask complement `bir.xor i64 %t17, -1` feeding
  `bir.and i64 %t16, %t18`; nearby call materializes `4096`, but the reproduced
  first printer failure is the signed `-1` xor mask.
- `00215`: fails as scalar `add`, function 0 block 7 instruction 1. Prepared
  BIR shows repeated timeout decrements such as `bir.add i64 %t7, -1`;
  representative operand is register plus signed `-1`.
- `00218`: fails as scalar `and`, function 0 block 0 instruction 2. Prepared
  BIR shows bitfield clear mask `bir.and i32 %t2.bf.old, -256` and value mask
  `bir.and i32 152, 255`; representative failed form is register and signed
  `-256`.

First implementation surface for Step 2: inspect scalar add/sub/bitwise
immediate acceptance, materialization, and printable fallback in
`src/backend/mir/aarch64/codegen/alu.cpp`, especially
`scalar_alu_operation_accepts_immediate`, the immediate rejection before
printing, and `materialize_source` / add-sub-bitwise emission. Selection inputs
come through `make_scalar_immediate_operand` and prepared scalar operand
conversion in the same file, with prepared BIR confirming the failing operands
are semantic immediates rather than moved failure modes.

## Suggested Next

Step 2 should implement a semantic AArch64 fallback for scalar add/sub/bitwise
immediates that are not directly printable as plain `#imm`, starting with
negative decrement/mask forms (`-1`, `-256`) and non-encodable positive forms
such as `4096` without adding filename-specific handling or weakening test
contracts.

## Watchouts

- Progress must repair scalar add/sub/bitwise immediate materialization or
  encoding fallback semantics, not match filenames or emitted instruction
  strings.
- The existing printer test `selected_scalar_add_sub_reject_nonencodable_immediates`
  currently asserts fail-closed behavior for `add #4096` and `sub #-1`; Step 2
  will likely need to update capability tests along with the implementation,
  but this Step 1 packet did not change expectations.
- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, or CTest registration.
- Do not fold scalar cast, mul/div/rem, call-boundary, memory store/symbol,
  runtime, or timeout residuals into this owner without new generated-code or
  diagnostic evidence.
- Do not reopen closed owners 285 through 298 from residual counts alone.

## Proof

Ran the delegated proof command exactly:

`cmake --build --preset default && ctest --test-dir build -j8 -R '^c_testsuite_aarch64_backend_src_(00031|00104|00143|00207|00213|00214|00215|00218)_c$' --output-on-failure > test_after.log 2>&1`

Build completed with `ninja: no work to do`; focused CTest returned failure
status with 0/8 tests passing and preserved the reproduction log at
`test_after.log`. Additional read-only diagnostics used `--dump-prepared-bir`,
`--dump-mir`, `--trace-mir`, and direct `--codegen asm` on representative
inputs; `--dump-mir` / `--trace-mir` currently expose only a legacy route
summary for these AArch64 inputs, while `--dump-prepared-bir` exposed the
representative operand forms above.
