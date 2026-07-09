Status: Active
Source Idea Path: ideas/open/630_string_constant_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Narrow RV64 String-Constant Consumer Admission

# Current Packet

## Just Finished

Completed Step 4: RV64 object-route local-memory diagnostics and fragments now
admit the narrow prepared string-constant pointer-load consumer path only when
the prepared access is a `LoadLocalInst` result of pointer type, default address
space, nonvolatile, exact 8-byte size/alignment, base-plus-offset
`PreparedAddressBaseKind::StringConstant`, nonempty prepared label,
signed-12-bit offset, and
`prepared_string_constant_local_memory_has_authority(...)`.

Implementation details:

- Added a string-specific local-memory diagnostic admission check in RV64 object
  emission without widening generic frame-slot, pointer-value, or global
  admission.
- Added the matching encoded fragment path for prepared string-constant pointer
  loads, using PC-relative object fixups to the prepared string label and
  preserving optional stack-home publication.
- Focused `backend_riscv_object_emission` coverage exercises positive
  admission plus fail-closed missing/unknown authority, proven out-of-bounds
  authority, non-default address space, stores, and unrelated global/frame/
  pointer base shapes.

## Suggested Next

Execute the smallest Step 5 row-reclassification packet: reclassify only the
validated prepared string-constant local-memory row family that now carries
`StringConstantBytes` authority and is admitted by the RV64 object consumer.
Keep the packet limited to row/category movement and focused proof; do not
rewrite GCC torture expectations, unsupported markers, allowlists, runtime
policy, direct-global policy, aggregate-home policy, ABI, or generic pointer
policy.

## Watchouts

Do not treat string constants as ordinary globals, frame slots, or pointer
values during Step 5. The accepted consumer path is authority-gated by prepared
facts, not by literal spelling or final assembly shape, and stores remain
unsupported. If row reclassification exposes a separate missing lowering
capability, split it rather than broadening this string-constant route.

## Proof

Ran the delegated proof exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$' > test_after.log 2>&1`

Result: build succeeded and `backend_riscv_object_emission` passed. Proof log:
`test_after.log`.
