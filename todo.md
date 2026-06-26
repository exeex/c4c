Status: Active
Source Idea Path: ideas/open/389_rv64_va_start_destination_va_list_address_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture The Prepared `va_start` Destination Address Boundary

# Current Packet

## Just Finished

Lifecycle activation created the active runbook for Step 1 of idea 389.

## Suggested Next

Execute Step 1: capture the prepared/BIR/object facts for the RV64 `va_start`
destination `va_list` address runtime boundary in `va-arg-13.c`, then identify
the likely owner for making `dst_va_list_addr` valid before store.

## Watchouts

- Keep idea 386 frame-slot-address call arguments and idea 388 `va_end` lowering closed.
- Keep idea 387 same-module sret calls out of scope.
- Do not hard-code `va-arg-13.c`, function `test`, register `s1`, or literal object offsets.
- Do not broaden this packet into aggregate `va_arg`, `va_copy`, or unrelated variadic helper rewrites.

## Proof

Lifecycle-only activation. No build or test proof required.
