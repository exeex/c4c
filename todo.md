Status: Active
Source Idea Path: ideas/open/335_aarch64_uninitialized_local_slot_runtime_residual.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Uninitialized Local-Slot First Bad Fact

# Current Packet

## Just Finished

Step 1 localized the first bad fact for
`c_testsuite_aarch64_backend_src_00164_c` to AArch64 local-load value
publication, not local-slot initialization or frame-slot assignment. Source
line 28 computes `a | b ^ c & d`, expecting `12 | (34 ^ (56 & 78)) == 46`.
Semantic and prepared BIR still load the right source locals:
`%t109 = load_local %lv.a`, `%t110 = load_local %lv.b`,
`%t111 = load_local %lv.c`, `%t112 = load_local %lv.d`; prepared addressing
maps those loads to local frame slots `%lv.a/#0+0`, `%lv.b/#1+4`,
`%lv.c/#2+8`, and `%lv.d/#3+12`, and assembly initializes those homes at
function entry with `12`, `34`, `56`, and `78`.

The first missing fact appears after those loads are assigned result homes:
prepared storage assigns `%t110` to `slot#37+stack148`, `%t111` to
`slot#38+stack152`, and `%t112` to `slot#39+stack156` while `%t109` is assigned
register `x13`. Before the line-28 scalar operations, generated assembly emits
`ldr w9, [sp, #152]`, `ldr w10, [sp, #156]`, and `ldr w10, [sp, #148]`, then
uses `orr w9, w13, w9`; there are no preceding stores publishing `34`, `56`,
or `78` to `sp+148/152/156`, and no fresh post-`printf` load publishing `a=12`
into caller-saved `w13`. Expected code must load from the initialized local
homes (`sp+4`, `sp+8`, `sp+12`, and `sp+0`) and then either use registers
directly or store the values into their assigned result homes before consumers
read them.

Classification: first owner is local-slot load result publication/materialized
value availability in the AArch64 prepared-memory/scalar-consumer path. The
local frame slots are initialized, the prepared frame-slot homes are assigned,
there are no relevant spill/reload records for these values, and this is not
primarily load/store ordering after a valid store. The visible bad emission is
that consumers read the result homes as initialized even though the load-local
results were never published there.

## Suggested Next

Execute Step 2 by repairing the general AArch64 load-local publication path:
when a `bir.load_local` result is assigned a stack home, materialize the source
local load from the prepared addressing frame slot and publish it to the
assigned result home before scalar consumers; when the result is assigned a
caller-saved register, ensure the load is emitted after intervening calls before
that register is consumed.

## Watchouts

Do not reopen scalar operand-fact printing unless the old printer diagnostics
return. Do not special-case `00164.c`, stack offsets around `sp+#148`,
`sp+#152`, or `sp+#156`, one instruction index, or one printed value. Keep
parked frame-layout idea 316 out of scope unless current localization proves
frame allocation or slot layout is the first owner.

AST-backed symbol checks point at the AArch64 lowering/publication boundary:
`src/backend/mir/aarch64/codegen/memory.cpp` has `lower_memory_instruction` for
`LoadLocalInst`, and `src/backend/mir/aarch64/codegen/alu.cpp` has scalar
consumer publication fallback using prepared homes. Treat this as a semantic
publication fix, not a named-offset patch.

## Proof

Ran the delegated proof:
`cmake --build build --target c4cll -j 2 && ctest --test-dir build -j --output-on-failure -R 'c_testsuite_aarch64_backend_src_00164_c' > test_after.log 2>&1`.
Build was up to date and CTest still fails with `[RUNTIME_MISMATCH]`: expected
line-28 output `46`, actual `-276893768`; later arithmetic output also remains
wrong (`1916` expected, actual `-276902993` / `10`). `test_after.log` contains
no old scalar operand-printer diagnostics; the observable failure is the
runtime mismatch.
