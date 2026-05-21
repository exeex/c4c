# AArch64 String Literal Pointer Value Publication

Status: Closed
Created: 2026-05-21
Closed: 2026-05-21
Split From: ideas/open/356_semantic_bir_pointer_derived_string_loads.md
Completed By: 79ea7bceb `Publish string literal pointer values`

## Goal

Publish string literal addresses as pointer values before local storage,
prepared-addressing, or generated AArch64 consumers follow those pointers.

## Closure Summary

Commit 79ea7bceb completed the pointer-publication goal. The old first bad
fact is gone: generated AArch64 no longer stores a stack address such as
`sp+72` into `%lv.a` for `char *a = "hello"`. The repaired path now publishes
the real `.str0` address into the local pointer slot before dynamic consumers
follow it.

The representative `00173` path still segfaults, but the remaining failure is
a separate AArch64/prepared scalar-memory issue where a loaded byte value is
later reused as an address. That residual is tracked separately in
`ideas/open/366_aarch64_loaded_byte_value_reused_as_address.md`.

## Completion Evidence

- Focused backend coverage for the string-pool literal pointer publication
  path passed after the repair.
- The stability contracts covered by the delegated proof,
  `backend_lir_to_bir_notes`, `backend_aarch64_instruction_dispatch`, and
  `backend_aarch64_memory_operand_contract`, passed.
- The old string literal pointer publication failure mode is no longer the
  first bad fact for `c_testsuite_aarch64_backend_src_00173_c`.

## Residual Out Of Scope

The later `00173` runtime segfault is not part of this closed idea. After
`%t7 = load i8, ptr %t6`, generated AArch64 starts with a valid pointer load
and byte load, then later moves the loaded byte value into an address register
and emits another byte load through that value. That byte-value-as-address
problem belongs to the residual scalar-memory initiative linked above.

