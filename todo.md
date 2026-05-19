Status: Active
Source Idea Path: ideas/open/300_aarch64_scalar_cast_machine_printer_forms.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce and classify focused cast diagnostics

# Current Packet

## Just Finished

Step 1 reproduced and classified the focused AArch64 scalar-cast
machine-printer diagnostics for idea 300.

Focused cases:

- `00035`: zero-extension width/form. CTest reports `opcode=zero_extend` at
  function 0 block 0 instruction 5 with `scalar cast node requires a supported
  integer source/result width`. Semantic BIR has logical-not truth casts
  `bir.zext i1 ... to i32`; prepared BIR promotes them to `bir.zext i32 ... to
  i32` (`%t3`, `%t10`, `%t13`), so the printer rejects the non-widening simple
  integer cast. Likely Step 2 surface: AArch64 scalar cast lowering/record
  publication around `make_prepared_scalar_cast_record` and
  `print_scalar_cast_instruction`, with attention to legalizer i1 promotion.
- `00105`: zero-extension width/form. CTest reports `opcode=zero_extend` at
  function 0 block 3 instruction 3 with the same supported-width diagnostic.
  Semantic BIR has `!i` lowered as `i1 -> i32`; prepared BIR presents
  `bir.zext i32 %t8 to i32`. Likely Step 2 surface: preserve original boolean
  source width for printable zero-extension or eliminate/lower promoted
  identity-width zext before machine printing.
- `00126`: zero-extension width/form. CTest reports `opcode=zero_extend` at
  function 0 block 0 instruction 5 with the same supported-width diagnostic.
  Semantic BIR has two `bir.zext i1 ... to i32` truth casts; prepared BIR
  presents `bir.zext i32 %t2 to i32` and `bir.zext i32 %t6 to i32`. Likely
  Step 2 surface: shared truth-value zero-extension handling after i1
  promotion.
- `00134`: sign-extension source structure. CTest reports `opcode=sign_extend`
  at function 0 block 0 instruction 2 with `scalar cast node requires a
  structured register source operand`. Prepared BIR includes `bir.sext i32 %t1
  to i64`, where `%t1` is rematerializable immediate `-1`; the cast record
  source is therefore not a structured register when the printer checks it.
  Likely Step 3 surface: scalar cast operand publication/materialization for
  immediate or rematerializable sign-extension sources, not the printer
  spelling itself.
- `00135`: sign-extension source structure. Same diagnostic at function 0 block
  0 instruction 2. Prepared BIR starts with `bir.sext i32 %t1 to i64`, and
  `%t1` is rematerializable immediate `-1`, matching `00134`. Likely Step 3
  surface: the same scalar cast source materialization path.
- `00151`: zero-extension width/form. CTest reports `opcode=zero_extend` at
  function 0 block 0 instruction 4 with the supported-width diagnostic.
  Semantic BIR has array equality result inverted then `bir.zext i1 ... to
  i32`; prepared BIR presents `bir.zext i32 %t17 to i32`. Likely Step 2
  surface: shared boolean result zero-extension after legalizer promotion.
- `00208`: zero-extension width/form. CTest reports `opcode=zero_extend` at
  function 0 block 6 instruction 3 with the supported-width diagnostic.
  Semantic BIR has `!s[0]` paths as `bir.zext i1 ... to i32`; prepared BIR
  presents promoted `bir.zext i32 ... to i32` forms (`%t55` in helpers and
  `%t66` in `main`). Likely Step 2 surface: same i1-to-i32 truth-value
  zero-extension handling, with additional pressure from stack/register value
  placement in the larger stdio-including translation unit.

## Suggested Next

Execute `plan.md` Step 2: repair zero-extension supported-width publication for
the five promoted truth-value cases (`00035`, `00105`, `00126`, `00151`,
`00208`) by making the machine route see a printable widening cast or by
lowering/eliding the promoted `i32 -> i32` zext before printer admission.

## Watchouts

- Keep the owner limited to scalar-cast machine-printer forms.
- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, or CTest registration.
- `print_scalar_cast_instruction` rejects simple integer casts when source and
  result widths are equal/non-widening, so a Step 2 fix should not merely
  weaken that guard without preserving valid AArch64 semantics.
- The sign-extension cases (`00134`, `00135`) are probably Step 3: prepared
  value locations show rematerializable immediate sources, while the printer
  requires a structured register source operand.
- The relevant code surfaces observed while reading are
  `src/backend/mir/aarch64/codegen/cast_ops.cpp` for cast record publication
  and printer admission, plus `make_prepared_scalar_operand` in
  `src/backend/mir/aarch64/codegen/alu.cpp` for immediate/rematerializable
  operand publication.

## Proof

Ran:

```bash
cmake --build --preset default && ctest --test-dir build -j7 -R '^c_testsuite_aarch64_backend_src_(00035|00105|00126|00134|00135|00151|00208)_c$' --output-on-failure > test_after.log 2>&1
```

Result: build was up to date; CTest failed 7/7 with the expected focused
machine-printer diagnostics. Proof log: `test_after.log`.
