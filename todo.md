Status: Active
Source Idea Path: ideas/open/334_aarch64_scalar_machine_node_operand_fact_printing.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Scalar Operand Fact Divergence

# Current Packet

## Just Finished

Step 1 localized the first owner for the current scalar machine-node printer
failures. Both representatives enter the same broad owner class: prepared
scalar operand conversion does not publish stack-slot scalar values as coherent
prepared frame-slot operands, so selection falls back to producer/load-source
memory facts. The two representatives then fail in different printer contract
branches.

`00164.c` representative:

- CTest first bad fact from `test_after.log`:
  `c_testsuite_aarch64_backend_src_00164_c` reaches function 0 block 40
  instruction 6 and fails printing scalar opcode `mul` with
  `scalar mul/div/rem node has incomplete printable rhs facts`.
- Prepared source operation in `main`/`logic.end.146`:
  `%t159 = bir.mul i32 %t157, %t158`.
- Source operands and homes:
  `%t157` is `%lv.b`, prepared home/register storage `x21`, width 1;
  `%t158` is `%lv.c`, prepared home stack slot `slot#58+stack232`, width 1;
  result `%t159` has prepared home stack slot `slot#59+stack236`, width 1.
- Prepared moves before block 40 instruction 6 include `%t157 -> %t159`
  as `consumer_register_to_stack` and `%t158 -> %t159` as
  `consumer_stack_to_stack`; prepared frame plan is coherent at
  `frame_size=256`, `frame_alignment=4`.
- Selected operand facts diverge before printing: `make_prepared_scalar_operand`
  accepts immediates and register-backed named values but returns
  `UnsupportedOperandStorage` for stack-slot `%t158`, so the prepared scalar
  ALU record is abandoned and `try_make_prepared_scalar_instruction_record`
  falls back through `make_scalar_fallback_operand`. The fallback can only use
  emitted registers or `make_prepared_scalar_load_source` producer memory facts,
  not the value's prepared stack home as a complete scalar operand fact.
- The printer then reaches the mul/div/rem materializer with a RHS memory-like
  operand for `%t158` that is not materializable under the required
  register/memory/immediate contract, producing the exact missing fact
  `scalar mul/div/rem node has incomplete printable rhs facts`.

`00214.c` representative:

- CTest first bad fact from `test_after.log`:
  `c_testsuite_aarch64_backend_src_00214_c` reaches function 0 block 4
  instruction 2 and fails printing scalar opcode `add` with
  `scalar add/sub/bitwise memory operands require prepared frame-slot sources`.
- Prepared source operation in `extend_brk`/`block_3`:
  `%t16 = bir.add i64 %t14, %t15`.
- Source operands and homes:
  `%t14 = bir.load_global i64 @_brk_end`, prepared home stack slot
  `slot#11+stack40`, width 1; `%t15 = bir.load_local i64 %lv.mask`, prepared
  home stack slot `slot#12+stack48`, width 1; result `%t16` has prepared
  register home/storage `x13`, width 1.
- Prepared moves before block 4 instruction 2 include `%t14 -> %t16` and
  `%t15 -> %t16` as `consumer_stack_to_register`; prepared frame plan is
  coherent at `frame_size=96`, `frame_alignment=8`, with slot offsets 40 and
  48 present.
- Selected operand facts diverge at the same conversion boundary:
  stack-slot `%t14`/`%t15` cannot be represented by
  `make_prepared_scalar_operand`, so fallback chooses producer memory facts.
  For `%t14`, the producer access is `base=global_symbol result=%t14
  symbol=_brk_end size=8`; for `%t15`, the producer access is
  `base=frame_slot result=%t15 frame_slot=#0 size=8`. The add/sub/bitwise
  printer rejects the non-frame-slot memory source before materialization,
  producing the exact missing fact
  `scalar add/sub/bitwise memory operands require prepared frame-slot sources`.

Owner classification: scalar prepared-operand publication / selected
machine-node operand fact support. This is not a frame-layout/prologue owner:
current prepared frame sizes, alignments, and slot offsets cover the involved
homes. The partial generated assembly prologue evidence remains downstream and
should not be treated as the first owner until the scalar operand-fact printer
failure is removed. This is also separate from fixed-formal, byval, stdarg,
HFA/floating, MOVI, large-offset spelling, local/value publication, and the
previous OPI scalar ALU result-home/cast/immediate/shift owners.

## Suggested Next

Execute Step 2 by repairing the prepared scalar operand conversion so
stack-slot scalar values can reach selected AArch64 scalar ALU records as
complete prepared frame-slot operand facts, while preserving existing register
and immediate behavior. The repair should be general for scalar ALU operands;
do not key on `00164.c`, `00214.c`, instruction indices, symbols, stack offsets,
or diagnostic strings.

## Watchouts

Do not reopen parked idea 316 from partial frame evidence unless localization
proves frame allocation is the current first owner. Do not implement a
filename-specific, instruction-index-specific, stack-offset-specific, or
diagnostic-string-only workaround. Keep the unrelated transient
`review/326_stdarg_byval_route_review.md` untouched.

## Proof

Lifecycle split/activation only. No build, CTest, or regression guard was run
for activation.
