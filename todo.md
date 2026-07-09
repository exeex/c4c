Status: Active
Source Idea Path: ideas/open/630_string_constant_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Separate String-Label Pointer Authority

# Current Packet

## Just Finished

Completed Step 6: traced the first producer/carrier boundary that marks the
remaining idea-630 string-constant pointer materializations
`range_verdict=proven_out_of_bounds`.

Exact boundary: `append_direct_frame_slot_accesses` calls
`build_direct_symbol_backed_access` for `bir.load_local ptr ..., addr .strN`;
the `LoadLocalInst` overload derives `size_bytes=8` from the pointer result
type, then calls `build_direct_symbol_backed_address`. For string constants,
`build_direct_symbol_backed_address` builds a `PreparedAddress` with
`base_kind=StringConstant`, `byte_offset=0`, `size_bytes=8`, and
`provenance=prepared_memory_provenance(...)`. When
`publish_string_constant_local_memory_authority` runs, it sets
`object_extent.size_bytes=string_constant.bytes.size()` and immediately calls
`prove_memory_access_requested_range`. If the string byte payload is shorter
than 8 bytes, `prove_memory_access_requested_range` sets
`ProvenOutOfBounds`, so `publish_string_constant_local_memory_authority`
returns before setting `layout_authority=StringConstantBytes`.

Representative code facts:

- `prepared_memory_provenance` fills the requested range and calls
  `prove_memory_access_requested_range` at
  `src/backend/prealloc/stack_layout/coordinator.cpp:80`.
- `build_direct_symbol_backed_address` constructs the string-constant
  prepared address and calls the string authority publisher at
  `src/backend/prealloc/stack_layout/coordinator.cpp:822`.
- `publish_string_constant_local_memory_authority` uses
  `string_constant.bytes.size()` as the complete extent, calls
  `prove_memory_access_requested_range`, and refuses authority unless the
  verdict is `ProvenInBounds` at
  `src/backend/prealloc/stack_layout/coordinator.cpp:488`.
- The range prover compares the requested end against
  `object_extent.size_bytes` at
  `src/backend/bir/bir_memory_provenance.hpp:150`.
- RV64 later requires `prepared_string_constant_local_memory_has_authority`,
  so the unknown authority plus out-of-bounds verdict is observed as
  `unsupported_local_memory_access` at
  `src/backend/mir/riscv/codegen/object_emission.cpp:12104` and in the
  prepared emitter at
  `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp:1900`.

This is not primarily missing string bytes. The prepared BIR operation is a
pointer materialization, for example
`@.str0 = bir.load_local ptr %lv.c1, addr .str0`, but the current prepared
memory-access path treats it as an 8-byte load from the string object's byte
payload. Short string literals therefore fail the string-byte range proof even
though no eight string bytes are needed to materialize the string label
address. There is also a real extent-size inconsistency to preserve as a
follow-up check: `collect_lowered_string_constants` stores decoded raw bytes in
`bir::StringConstant::bytes` without the trailing NUL, while
`lower_string_constant_global` uses LIR `byte_length`, which includes the NUL.
That inconsistency can change byte-load bounds near the terminator, but it
does not explain the 8-byte pointer materialization failures by itself.

Affected Step 5 rows and representative prepared facts:

- `src/20000722-1.c`: literal `"hi"`; `bir.load_local ptr ..., addr .str0`
  at `build/agent_state/630_step1_20000722-1.prepared.txt:41`; prepared
  access `offset=0 size=8` is `proven_out_of_bounds` at `:318`.
- `src/20011109-2.c`: literal `"foo"`; two pointer materializations at
  `build/agent_state/630_step1_20011109-2.prepared.txt:22` and `:25`;
  prepared accesses are `proven_out_of_bounds` at `:409` and `:412`.
- `src/20021204-1.c`: literal `"test"`; pointer materialization at
  `build/agent_state/630_step1_20021204-1.prepared.txt:35`; prepared access
  is `proven_out_of_bounds` at `:371`.
- `src/20030920-1.c`: literal `"\x7f\xff"`; pointer materialization at
  `build/agent_state/630_step1_20030920-1.prepared.txt:21`; prepared access
  is `proven_out_of_bounds` at `:229`.
- `src/920429-1.c`: literal `"ab"`; pointer materialization at
  `build/agent_state/630_step1_920429-1.prepared.txt:56`; prepared access is
  `proven_out_of_bounds` at `:393`.
- `src/930429-1.c`: literal `""`; pointer materialization at
  `build/agent_state/630_step1_930429-1.prepared.txt:38`; prepared access is
  `proven_out_of_bounds` at `:298`.
- `src/pr34415.c`: literal `"Bbb:"`; pointer materialization at
  `build/agent_state/630_step1_pr34415.prepared.txt:133`; prepared access is
  `proven_out_of_bounds` at `:606`.
- `src/pr35800.c`: mixed string lengths. Short literals such as `"int"`,
  `"char"`, `"short"`, `"long"`, `"void"`, `"float"`, `"double"`, `"wchar"`,
  and `"logical"` have `offset=0 size=8` pointer materializations marked
  `proven_out_of_bounds` at
  `build/agent_state/630_step1_pr35800.prepared.txt:1880`, `:1883`, `:1886`,
  `:1889`, `:1910`, `:1913`, `:1916`, `:1931`, and `:1967`. Longer literals
  in the same file, such as `"unsigned char"`, are `proven_in_bounds`, which
  confirms the current verdict is driven by string byte extent versus the
  pointer-width access size.

Focused evidence extract:
`build/agent_state/630_step6_string_range_extract.txt`.

## Suggested Next

Execute Step 7: implement one narrow idea-630 code packet that separates
string-label pointer materialization authority from real string-byte memory
access range authority. The smallest candidate is in the prepared
access/address construction path: for `bir.load_local ptr` /
`bir.load_global ptr` whose address base is `StringConstant`, publish a
string-label pointer materialization/access authority that RV64 can consume
without requiring an 8-byte in-bounds range against the string byte payload.
Keep real byte/char loads from string storage on the existing string-byte
extent proof, and separately add or update focused coverage for the
trailing-NUL extent mismatch.

## Watchouts

Fail closed when the string identity/text id is missing or ambiguous, the
address space is non-default, the access is volatile, the requested pointer
size/alignment is not the target pointer width, or RV64 lacks a label to
relocate/materialize. Do not make all 8-byte string accesses automatically
valid; only pointer-result string-label materializations should bypass the
string-byte extent proof. The out-of-scope owners from Step 5 remain split:
direct-global, aggregate/block-entry stack-home publication, pointer-value byte
access, select-carrier publication, and prepared move-bundle fan-in.

## Proof

Evidence-only trace; no build or test run required by this packet. No root logs
created or modified. Diagnostic artifact:
`build/agent_state/630_step6_string_range_extract.txt`.
