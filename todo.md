# AArch64 String/Global Address External Call Lowering Todo

Status: Active
Source Idea Path: ideas/open/287_aarch64_string_global_address_external_call_lowering.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement General Address Materialization for Call Arguments

# Current Packet

## Just Finished

Step 2 (`Implement General Address Materialization for Call Arguments`)
completed the first general AArch64 repair for direct call arguments whose
source encoding is a string/global address. Stack-layout preparation now
publishes a prepared address-materialization record for pointer-valued direct
call arguments that name a retained string constant or carry a structured
global/function `LinkNameId`. AArch64 block dispatch lowers that prepared
address materialization before before-call ABI moves, so the existing call
move consumes an initialized source register instead of `mov x0, x13` from an
unmaterialized scratch.

The public AArch64 assembly route now also emits retained BIR string constants
into `.rodata` with an explicit trailing NUL byte. For
`tests/c/external/c-testsuite/src/00125.c`, the generated assembly now contains:

```asm
adrp x13, .str0
add x13, x13, :lo12:.str0
mov x0, x13
bl printf
```

and emits `.str0` in `.rodata` as byte data ending in `0`.

## Suggested Next

Keep the next packet narrow around nearby same-feature coverage: prove one
additional direct external call argument that uses a structured global/function
symbol address, or broaden AArch64 string/global call-argument coverage only
far enough to catch duplicate materialization and multi-argument ordering.

## Watchouts

- Do not change runner behavior, allowlists, expectations, unsupported
  classifications, timeout settings, CTest registration, parser, or sema.
- Reject named-case shortcuts, literal-spelling shortcuts, and `printf`-only
  special cases.
- The dispatch fix materializes at most the prepared address record associated
  with the call instruction index before call moves. If later calls need more
  than one address-valued direct argument at the same instruction index, the
  prepared-addressing lookup shape may need to grow beyond the current
  first-match helper.
- Keep `src/00132.c` out of first proof; it is timeout-sensitive and
  loop/local-store-compounded.
- Keep broad aggregate ABI/HFA returns, broad variadic ABI completeness,
  `_Generic`, wide chars, and function-pointer casts out of this route.

## Proof

Ran the supervisor-selected proof:

```bash
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_src_00125_c$'; } > test_after.log 2>&1
```

Exit code was `0`; `src/00125.c` passed. Canonical proof log:
`test_after.log`.

Focused local guard also passed:

```bash
cmake --build build --target backend_prepare_stack_layout_test
./build/tests/backend/bir/backend_prepare_stack_layout_test
```
