# AArch64 String/Global Address External Call Lowering Todo

Status: Active
Source Idea Path: ideas/open/287_aarch64_string_global_address_external_call_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish the Minimal Address Argument Failure

# Current Packet

## Just Finished

Step 1 (`Establish the Minimal Address Argument Failure`) reproduced and
localized `tests/c/external/c-testsuite/src/00125.c`. The AArch64 backend
binary runs but prints nothing; the expected output is `hello world\n`.

The generated assembly at
`build-aarch64-scan/c_testsuite_aarch64_backend/src/00125.c.s` lowers the
call as:

```asm
mov x0, x13
bl printf
```

There is no preceding `adrp/add` or equivalent materialization of the
string-literal address into `x13`, and the assembly contains no emitted
`.rodata`/`.asciz` string object for `@.str0`. The backend evidence is:

- LLVM route still has the correct literal and address use:
  `/tmp/c4c_aarch64_00125_step1.ll` contains `@.str0 = ... c"hello world\0A\00"`
  and `%t0 = getelementptr ... @.str0`, then `printf(ptr %t0)`.
- Semantic BIR has already collapsed the call argument to
  `bir.call i32 printf(ptr @.str0)` in `/tmp/c4c_aarch64_00125_step1.bir`.
- Prepared BIR records the intended ABI move:
  `arg0 bank=gpr from=symbol_address:@.str0 to=x0`, with storage/home for
  `@.str0` assigned to `x13`, in
  `/tmp/c4c_aarch64_00125_step1.prepared-bir`.
- The same prepared dump has an empty `--- prepared-addressing ---` section,
  so no address-materialization record is available for the final AArch64
  route to print before the call.

## Suggested Next

Implement the first repair packet for Step 2 by making the AArch64 prepared
backend materialize address-valued direct call arguments before their ABI
register move. The narrow owner is the prepared/address-materialization plus
call-boundary path, not parser/sema or testcase metadata:

- Create or reuse a semantic prepared-addressing record for pointer-valued
  `SymbolAddress` call arguments such as `@.str0`.
- Ensure AArch64 lowering emits `adrp/add` for the string/global address into
  the assigned source register before `mov x0, <source>` and `bl printf`.
- Ensure the AArch64 assembly route emits the referenced string constant data
  object, since the current `.s` has no `@.str0` payload to address.

Recommended first proof command:

```bash
ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_src_00125_c$'
```

## Watchouts

- Do not change runner behavior, allowlists, expectations, unsupported
  classifications, timeout settings, CTest registration, parser, or sema.
- Reject named-case shortcuts, literal-spelling shortcuts, and `printf`-only
  special cases.
- The prepared call plan already recognizes `source_encoding=symbol_address`
  and destination `x0`; a fix that only changes the printed `mov x0, x13`
  without materializing the source address leaves the root failure intact.
- `src/backend/prealloc/stack_layout/coordinator.cpp` currently records string
  address materializations for load-local/load-global address forms, but this
  testcase reaches the call as a direct symbol-address operand rather than a
  load instruction.
- `src/backend/mir/aarch64/codegen/globals.cpp` can already print selected
  string/global address materializations as `adrp`/`add` when prepared facts
  exist. `src/backend/mir/aarch64/codegen/calls.cpp` currently treats the
  symbol-address argument like an ordinary register-to-register call-boundary
  move after regalloc assigns `@.str0` to `x13`.
- Keep `src/00132.c` out of first proof; it is timeout-sensitive and
  loop/local-store-compounded.
- Keep broad aggregate ABI/HFA returns, broad variadic ABI completeness,
  `_Generic`, wide chars, and function-pointer casts out of this route.

## Proof

Ran the supervisor-selected narrow proof:

```bash
ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_src_00125_c$' > /tmp/c4c_aarch64_00125_step1.log 2>&1
```

Exit code was `8`, expected for this baseline packet. Log:
`/tmp/c4c_aarch64_00125_step1.log`.

Additional temporary evidence:

- `/tmp/c4c_aarch64_00125_step1.bir`
- `/tmp/c4c_aarch64_00125_step1.prepared-bir`
- `/tmp/c4c_aarch64_00125_step1.mir`
- `/tmp/c4c_aarch64_00125_step1.ll`
