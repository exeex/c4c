Status: Active
Source Idea Path: ideas/open/270_aarch64_prologue_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish The Current Prologue Boundary

# Current Packet

## Just Finished

Completed plan Step 1, `Establish The Current Prologue Boundary`, as a
todo-only audit. The live compiled frame-entry boundary today is the simple
fixed-frame node insertion in
`src/backend/mir/aarch64/codegen/traversal.cpp`: it accepts only a prepared
frame plan with nonzero frame size/alignment, no dynamic stack plan, and no
saved callee registers; it inserts a `FrameInstructionKind::PrologueSetup`
node at the entry block start and an `EpilogueTeardown` node before each return
record. The target node payload already preserves prepared frame facts through
`FrameInstructionRecord`: function identity, frame size/alignment snapshots,
frame slot order, saved-callee-register vector, dynamic-stack flag,
frame-pointer-for-fixed-slots flag, optional callee-save payload, and
`source_frame` provenance. `instruction.cpp` currently owns
`make_frame_instruction`, frame opcode/side-effect mapping, and
`frame_selection_status`; `machine_printer.cpp` prints only the behavior that
is currently live: plain `sub/add sp, sp, #frame_size` for simple fixed frames,
rejecting dynamic stack, callee saves, unsupported frame kinds, missing
provenance, missing alignment, and large immediates.

Broad legacy prologue material should be rejected or deferred for this route:
do not revive `used_callee_saved`, `callee_save_offset`, parameter ingress,
variadic function-entry register-save areas, old frame-pointer setup/teardown,
stack probing, dynamic-stack restore, outgoing-call-area layout, or
legacy text-first save/restore selection from `prologue.md`. The current
contracts say those are prepared-carrier or later machine-node work, not
authority to recompute frame layout from markdown.

Current broad owners worth preserving are: `traversal.cpp` for the temporary
simple frame setup/teardown insertion point; `instruction.{hpp,cpp}` for the
generic frame record types, status, opcode, side effects, and public factory;
`machine_printer.cpp` for the narrow printable subset; `calls.cpp` for call
boundary, preserved-value, clobber, memory-return, and variadic call facts;
`memory.cpp` for frame-slot memory operands and prepared frame-slot loads;
`returns.cpp` for return payload/control records; and `module/module.hpp` plus
prepared lookup helpers for frame, dynamic-stack, call, move, ABI-binding,
spill/reload, and value-location snapshots.

Step 2 API direction: add `prologue.hpp` / `prologue.cpp` as the compiled
frame-entry owner with a narrow prepared-frame API, not a legacy ABI
reimplementation. The first useful surface should expose a behavior-preserving
helper such as
`insert_prepared_frame_boundary_nodes(const module::FunctionLoweringContext&,
const prepare::PreparedControlFlowFunction&, module::MachineFunction&)`, plus
an internal predicate equivalent to the current simple-fixed-frame gate and an
internal builder for `FrameInstructionRecord`-backed machine instructions.
Prefer moving the current frame instruction record construction/status
direction into the prologue owner where practical, while keeping the published
`FrameInstructionRecord` data shape compatible with existing tests and generic
instruction consumers. The integration answer is behavior-preserving movement
of the existing simple fixed-frame boundary; callee-save, dynamic-stack,
frame-pointer, variadic-entry, outgoing-area, and concrete save/restore
selection remain explicit deferred boundaries.

## Suggested Next

Execute Step 2 from `plan.md`: create `prologue.hpp` / `prologue.cpp`, add the
new translation unit to the backend build if needed, and move only the
behavior-preserving simple fixed-frame boundary out of `traversal.cpp` behind
the prepared-frame API described above.

## Watchouts

Keep `traversal.cpp` as the caller/coordinator, not the hidden frame owner.
Preserve emitted output exactly for the current printable subset. Do not add
callee-save store/load nodes, dynamic-stack handling, frame-pointer setup, or
variadic function-entry support in Step 2 unless a separate packet explicitly
authorizes the needed carriers and proof. Public tests currently call
`make_frame_instruction` from `instruction.hpp`, so moving the factory/status
surface may need a compatibility wrapper or a test update in a code-changing
packet; this audit packet made no code changes.

## Proof

No build required per the supervisor packet. Audit-only todo update; no
`test_after.log` was written because the packet explicitly listed
`test_after.log` under Do Not Touch.
