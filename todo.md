Status: Active
Source Idea Path: ideas/open/341_rv64_vector_register_inline_asm_constraints_stage2.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Group Allocation And Overlap Rules

# Current Packet

## Just Finished

Step 3: Implement Group Allocation And Overlap Rules completed.

Fixed grouped-register eviction so a newly assigned value receives a legal span
for its own required width/class after the evicted assignment is removed, rather
than inheriting the evicted value's span shape. This preserves existing grouped
linear-scan behavior while preventing width-mismatched reuse during eviction.

Added RV64 inline-asm allocator coverage using the Stage 2 `VR`, `VRM2`, and
`VRM4` operand metadata. Tests now prove legal width/alignment for `VR`,
`VRM2`, and `VRM4`, full occupied-register reservation, no overlap between
untied same-instruction operands, grouped homes for `+VRM2` read-write and
numeric tied `=VRM4,0` reuse, and explicit impossible-allocation diagnostics
when 33 simultaneous `VR` operands exhaust the disjoint vector register space.

The existing RV64 vector span split remains intentional and sufficient for this
step: non-crossing allocation first uses `v0..v15` and then the disjoint
callee-saved fallback spans `v16..v31`, so the combined allocator space covers
`v0..v31` without overlapping the allocator's separate active caller/callee
assignment sets.

## Suggested Next

Delegate Step 4 to implement base-register substitution for RV64 inline asm
vector operands. The next packet should print the allocated base register for
`VR`, `VRM2`, and `VRM4` placeholders while preserving scalar `.insn` behavior.

## Watchouts

- Do not reactivate the parked Stage 1 child unless lifecycle rules explicitly
  require it.
- Keep Stage 2 focused on `VR`, `VRM2`, and `VRM4`; EV `.insn.d`,
  named-operands, masks, and consteval asm strings are later children.
- Treat testcase-overfit, expectation weakening, and raw-string constraint
  acceptance as route failures.
- The pre-regalloc override hook is intentionally in
  `src/backend/prealloc/prealloc.cpp` so regalloc sees inline-asm vector
  class/width facts before assignment; keep later packets from moving this back
  to carrier-only publication.
- `+VRM2` is now covered by allocator-focused read-write tests; substitution
  still needs to print only the selected base register.
- Current RV64 object helper
  `rv64_register_number_for_inline_asm_operand` hard-codes `=r`, `+r`, `r`,
  and numeric tied GPR operands. Vector substitution should not weaken these
  scalar fail-closed checks.
- The existing RV64 vector span helper splits caller/callee pools into
  `v0..v15` and `v16..v31`; Step 3 proved this as the intended disjoint
  combined `v0..v31` allocator space. Do not collapse it into one overlapping
  caller pool without also unifying active caller/callee overlap tracking.

## Proof

Ran exact delegated proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`

Result: build succeeded; backend CTest completed with only the known unrelated
baseline failure `backend_codegen_route_riscv64_pointer_typed_select_publication`.
No new backend failures were present. Proof log: `test_after.log`.
