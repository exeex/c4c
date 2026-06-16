Status: Active
Source Idea Path: ideas/open/286_aarch64_00204_stdarg_semantic_bir_local_memory_admission.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Capture the Failing AArch64 Local-Memory Shape

# Current Packet

## Just Finished

Step 1 completed the failure capture for the AArch64-target `00204.c`
`myprintf` route. The delegated proof reproduced the expected failure in both
target tests, with the first public diagnostic still:
`semantic lir_to_bir function 'myprintf' failed in load local-memory semantic
family`.

The first concrete bad shape is in AArch64 LLVM/LIR block `block_72` at the
first aggregate `va_arg(ap)` for `%struct.s7`:

```llvm
%t9 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3
%t10 = load i32, ptr %t9
...
%t18 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 1
%t19 = load ptr, ptr %t18
%t20 = getelementptr i8, ptr %t19, i32 %t10
...
%t21 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 0
%t22 = load ptr, ptr %t21
%t23 = getelementptr i8, ptr %t22, i64 8
store ptr %t23, ptr %t21
...
%t24 = phi ptr [ %t20, %vaarg.reg.13 ], [ %t22, %vaarg.stack.11 ]
%t25 = alloca %struct.s7
call void @llvm.memcpy.p0.p0.i64(ptr %t25, ptr %t24, i64 7, i1 false)
%t26 = load %struct.s7, ptr %t25
store %struct.s7 %t26, ptr %lv.t7
```

Facts captured:

- First failing function/block/family: `myprintf`, `block_72` /
  `vaarg.join.14`, load local-memory semantic family.
- First rejected instruction family: aggregate local load from the local
  aggregate scratch created for the selected `va_arg` payload,
  `%t26 = load %struct.s7, ptr %t25`, after `memcpy(ptr %t25, ptr %t24, i64 7)`.
- Local/address sources: `%lv.ap` is AArch64 `__va_list_tag_ = { ptr, ptr, ptr,
  i32, i32 }`; field 3 (`gr_offs`) chooses register vs stack for the first
  non-HFA aggregate, field 1 (`gr_top`) feeds `%t20`, field 0
  (`overflow_arg_area`) feeds `%t22`, and `%t24` is the phi-selected payload
  pointer copied into `%t25`.
- Nearby AArch64 HFA shape uses the same semantic family later: e.g.
  `block_78` allocates `%t59 = alloca %struct.hfa11`, reads field 4
  (`vr_offs`) and field 2 (`vr_top`) for register-save lanes, then joins with
  `%t82 = load %struct.hfa11, ptr %t59`; this confirms the route must handle
  aggregate local scratch loads generally, not just `%struct.s7`.
- Passing x86 route lowers the same first aggregate shape into expanded BIR:
  register and stack arms copy lane bytes from pointer-value addresses
  (`addr %t20`, `addr %t20+1`, ... or `addr %t24`, ...), then the join copies
  local scalar leaves into `%lv.t7.*`. Prepared x86 records those as structured
  `base=pointer_value` loads followed by `base=frame_slot` stores.
- AST-backed owner scan selected `src/backend/bir/lir_to_bir/memory/local_slots.cpp`
  as Step 2's semantic owner, specifically the dynamic local aggregate load
  surface around `try_lower_dynamic_local_aggregate_load()` and
  `load_dynamic_local_aggregate_array_value()`. The supporting producer for
  aggregate `va_arg` calls is `src/backend/bir/lir_to_bir/calling.cpp`
  `lower_va_arg_call`, while prepared-only planning in
  `src/backend/prealloc/variadic_entry_plans.cpp` is comparison/proof surface,
  not the first repair owner.

## Suggested Next

Execute Step 2 by admitting this semantic shape in
`src/backend/bir/lir_to_bir/memory/local_slots.cpp`: aggregate local scratch
loads after `va_arg` payload `memcpy`, including scalar-leaf expansion from the
declared local aggregate slots, without keying on `00204.c`, `myprintf`, or
specific struct names.

## Watchouts

- Do not fix this by changing expected dump text, CTest labels, or support
  classification.
- Do not add shortcuts for `00204.c`, `myprintf`, `movi`, or specific HFA
  struct names.
- Do not treat the AArch64 prepared variadic plans as the source of the Step 2
  fix; semantic BIR currently fails before prepared publication can run.
- The first non-HFA aggregate is `%struct.s7` size 7/align 1, but the adjacent
  HFA local scratch loads show the repair should cover aggregate local scratch
  loads by layout/leaf slots rather than by named type.
- The x86 route proves the expected semantic shape is pointer-source payload
  copy into local aggregate leaves, then frame-slot leaf loads/stores.

## Proof

Ran:

```sh
cmake --build --preset default && ctest --test-dir build -R '^(backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$' --output-on-failure
```

Result: build was up to date; both delegated AArch64 tests failed with the
expected `myprintf` load local-memory semantic family rejection. Proof log:
`test_after.log`.

Use the same narrow proof command for the first Step 2 implementation packet.
