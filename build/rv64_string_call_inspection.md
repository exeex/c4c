# RV64 String Call Inspection

## Scope

Narrow representative: `tests/c/external/c-testsuite/src/00025.c`.

The source declares `int strlen(char *);`, stores the string literal `"hello"`
through a local pointer, then returns `strlen(p) - 5`.

This packet is inspection-only. No implementation source or tests were changed.

## Prepared-BIR Facts

Generated artifact:
`build/rv64_string_call_inspection/src_00025_prepared_bir.txt`.

Important facts from the prepared BIR:

- BIR contains the imported declaration `bir.func @strlen(ptr %p._anon_param_) -> i32`.
- `main` materializes the string literal as a pointer value named `@.str0`:
  `@.str0 = bir.load_local ptr %lv.p, addr .str0`.
- The local pointer assignment is represented as
  `bir.store_local %lv.p, ptr @.str0`.
- The extern call is already present as `%t2 = bir.call i32 strlen(ptr %t1)`.
- The prepared call plan is already `wrapper_kind=direct_extern_fixed_arity`,
  `callee=strlen`, one GPR argument from `t0` to `a0`, and a GPR result from
  `a0` to `s2`.
- Prepared addressing records
  `address_materialization block=entry inst_index=1 kind=string_constant result=@.str0 text=.str0`.
- Stack-layout notes currently report that address materialization for `@.str0`
  is missing a structured symbol `LinkNameId`; this is a diagnostic fact, not
  the immediate RV64 storage-emitter failure.

## Current RV64 Output Facts

Generated artifacts:

- `build/rv64_string_call_inspection/src_00025.emit.out`
- `build/rv64_string_call_inspection/src_00025.s`

Current result:

- RV64 asm emission fails with
  `error: riscv prepared module emitter does not support this prepared global storage layout`.
- No `src_00025.s` file is produced by the current failing command.
- The failure happens after the output-contract repair as a proper diagnostic,
  not as a silent missing output.

## Owner Surface: String Storage Emission

Smallest BIR owner surface:

- `src/backend/bir/lir_to_bir/module.cpp:1329` lowers each LIR string-pool
  entry through `lower_string_constant_global()` and pushes it into
  `module.globals`.
- `src/backend/bir/lir_to_bir/globals.cpp:519` owns
  `lower_string_constant_global()`.
- That helper sets string data to `type=I8`, `is_extern=true`,
  `is_constant=true`, `size_bytes=byte_length`, `align_bytes=1`, and records
  `GlobalInfo::is_string_constant=true`.

Smallest RV64 owner surface:

- `src/backend/mir/riscv/codegen/prepared_module_emit.cpp:13` calls
  `append_prepared_global_storage_asm()` before emitting `.text`.
- `src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp:166` owns
  `append_prepared_global_storage_asm()`.
- The current RV64 storage filter accepts only no-storage extern globals or
  simple defined i32 globals. String globals are `extern` and `constant`, so
  `is_no_storage_extern_global()` rejects them, and they are not simple i32
  storage.

Suggested smallest implementation packet:

- Teach `append_prepared_global_storage_asm()` a semantic string/byte-data
  storage case for prepared `I8` constant globals with byte-sized storage,
  instead of weakening the string global into a no-storage extern.

## Owner Surface: String Address Materialization

Prepared owner surface:

- `src/backend/prealloc/stack_layout/coordinator.cpp:1436` records
  `PreparedAddressMaterializationKind::StringConstant` for string-backed
  pointer values.

RV64 owner surface:

- `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp:210` owns
  pointer `store_local` emission.
- That branch currently handles only `FrameSlot` and `DirectGlobal`
  materialization. It rejects `StringConstant`, so even after storage emission
  is repaired, `p = "hello"` still needs RV64 emission for string-constant
  address materialization.
- `src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp:231` owns
  direct global address materialization, but currently requires
  `DirectGlobal` and a simple defined i32 global. It is not the string-constant
  path as-is.

Suggested smallest implementation packet:

- Add RV64 handling for `PreparedAddressMaterializationKind::StringConstant`
  in the pointer local-store/address materialization path, using the prepared
  text identity and string data label. Avoid hard-coding `.str0`, `"hello"`, or
  `strlen`.

## Owner Surface: Direct Fixed-Arity Extern Calls

Prepared facts say the direct extern call plan already exists for this case:

- `wrapper_kind=direct_extern_fixed_arity`
- `callee=strlen`
- `args=1`
- arg move `t0 -> a0`
- result move `a0 -> s2`

RV64 owner surface:

- `src/backend/mir/riscv/codegen/prepared_function_emit.cpp:197` dispatches
  `bir::CallInst` to `emit_riscv_simple_call()`.
- `src/backend/mir/riscv/codegen/prepared_call_emit.cpp:14` owns
  `emit_riscv_simple_call()`.
- That emitter already accepts `PreparedCallWrapperKind::DirectExternFixedArity`
  together with `SameModule`, supports up to eight GPR arguments, emits
  `call <callee>`, and handles GPR result movement.

Conclusion:

- Direct fixed-arity extern-call lowering is not the first blocking gap for
  `00025.c`; the representative is blocked earlier by string storage emission,
  and then by string-address materialization into the pointer local.

## Watchouts

- Do not special-case `.str0`, `"hello"`, `strlen`, or this testcase path.
- Do not convert string globals into no-storage externs; the string bytes must
  be emitted as addressable storage.
- Do not weaken expectations or mark this supported-path family unsupported.
- Keep the direct extern call path general: the existing plan is already
  fixed-arity and should remain driven by prepared call metadata.
