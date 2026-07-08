Status: Active
Source Idea Path: ideas/open/612_rv64_instruction_fragment_consumers.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement First Supported Instruction-Fragment Family

# Current Packet

## Just Finished

Step 2 from `plan.md` implemented the first supported RV64/MIR
instruction-fragment family: prepared GPR-backed i8/i16 integer bitfield
`BinaryInst` lowering for `lshr`/`shl` immediate shifts and adjacent
`and`/`or` clear/recombine forms.
The object route now zero-extends the narrow GPR source before emitting
semantic RV64 `srli`/`slli`/`andi`/`ori`/`and`/`or` fragments, gated by
prepared i8/i16 operand facts and supported GPR or stack result homes.
`LShr`/`Shl` accept immediate shift counts without requiring the RHS value type
to equal the result type; `And`/`Or` remain fail-closed on mixed-type RHS
values.

Movement covered by the implemented semantic shape: the selected Step 1 rows
with prepared i8/i16 bitfield `lshr` facts (`src/20030714-1.c`,
`src/20040709-2.c`, `src/20040709-3.c`, `src/960608-1.c`) and same-rule
`and`/clear/recombine facts (`src/pr37882.c`, `src/931110-1.c`). Focused
object-emission tests now cover i8/i16 `lshr`, `shl`, clear masks, recombine
`or`, stack-result publication, pointer-result rejection, and malformed
mixed-type RHS rejection for value-combining forms without expectation or
unsupported-marker changes.

Direct object probes after the patch:

- `src/20030714-1.c`: moved past owner `i8 %t2.bf.shr`; residual stop is
  `CastInst` owner `i32 %t3` in `RenderBox_setStyle`.
- `src/20040709-2.c`: moved past owner `i16 %t2.bf.shr` and the adjacent
  shifted/recombined bitfield fragments; residual stop is
  `unsupported_floating_cast`.
- `src/20040709-3.c`: moved past owner `i16 %t2.bf.shr` and the adjacent
  shifted/recombined bitfield fragments; residual stop is
  `unsupported_floating_cast`.
- `src/960608-1.c`: moved past owner `i8 %t10.bf.shr` and the adjacent
  shifted/recombined bitfield fragments; residual stop is prepared move-bundle
  fan-in to one stack destination at `main`, `entry`, `instruction_index=33`.
- `src/pr37882.c`: moved past owner `i8 %t2.bf.vm` and recombine; direct object
  probe now succeeds.
- `src/931110-1.c`: moved past owner `i16 %t12.bf.clr` and recombine; residual
  stop is `BinaryInst` owner `i16 %t13.bf.sext` in `main`, `block_1`.

## Suggested Next

Refresh the RV64 gcc_torture backend-object diagnostics and decide whether the
next packet should take the pointer `BinaryInst` add/address arithmetic family
or split the remaining unsupported instruction fragments by current first
owner after this Step 2 movement.

## Watchouts

- This slice intentionally does not touch pointer arithmetic, casts, select
  publication, call/result policy, inline asm, floating/f128, ABI/runtime,
  expectations, unsupported markers, allowlists, timeout, or accounting.
- The new lowering is deliberately fail-closed for non-i8/i16 types, missing
  prepared storage facts, unsupported homes, pointer results, invalid shift
  immediates, named/register shift counts, and mixed-type RHS values for
  `And`/`Or`.
- The delegated proof did not rerun the full gcc_torture backend-object refresh;
  row-count movement should be measured by the suggested next diagnostics
  packet before closing the active route.

## Proof

Ran delegated proof command exactly and preserved the output in
`test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed, 346/346 backend tests passed. Focused pre-proof check also
passed: `ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'`.
