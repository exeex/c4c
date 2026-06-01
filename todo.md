Status: Active
Source Idea Path: ideas/open/77_aarch64_machine_status_printer_validation_probe.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Decide AArch64-Local Helper Boundary

# Current Packet

## Just Finished

Completed `plan.md` Step 3 helper-boundary decision for the paired AArch64
machine status/printer record families recorded in Steps 1-2.

Prior Step 1 paired-family inventory remains:

- Branch: status helper `branch_selection_status`; printer checks
  `print_branch` and fused path `print_fused_compare_branch`.
- Scalar: status helper `scalar_selection_status`; printer check
  `print_scalar` plus delegated ALU/cast print helpers.
- Address materialization: status helper
  `address_materialization_selection_status`; printer dispatch to
  `print_address_materialization_instruction`.
- Spill/reload: status helper `spill_reload_selection_status`; printer check
  `print_spill_reload`.
- Frame: status helper `frame_selection_status`; printer check `print_frame`.

Prior Step 1 unpaired-family summary remains:

- Printer-only dispatch families include `MemoryInstructionRecord`,
  `AtomicMemoryInstructionRecord`, call-boundary/call/return records,
  `AssemblerInstructionRecord`, i128/f128 helper records, and intrinsic-family
  records. These have printer validation but no matching local
  `*_selection_status` helper in `instruction.cpp`.
- No dedicated `*_selection_status` helper lacks a matching printer dispatch
  for its record family. Unpaired status assignments are the fixed
  `DeferredUnsupported` constructors for assembler/object records and
  caller-provided unsupported status; `ObjectInstructionRecord` is not printer
  dispatched.

Step 2 overlap classification:

- Branch: mixed repeated semantic validation and final printer validation.
  - Repeated semantic validation:
    `branch_selection_status` requires conditional branches to have
    `target_pair` plus `condition_record`; `print_branch` also requires
    `target_pair`, and either a materialized condition operand or fused compare
    facts before it can print a conditional branch. Status checks this so the
    machine node is not marked selected without branch decision facts; the
    printer checks it so it can choose and spell the actual branch sequence.
  - Repeated semantic validation:
    both status and printer require fused compare branches to have
    `condition.predicate` and `condition.compare_operands`. Status rejects a
    selected fused branch without compare facts; the printer needs the same
    facts to fold immediate compares or emit `cmp` plus `b.<cond>`.
  - Repeated semantic validation:
    status checks `operands.size() < 5U` as missing fused compare operands;
    `print_fused_compare_branch` repeats the same operand-count condition
    before reading `instruction.operands[3]` and `[4]`. Status verifies the
    prepared node has the operand payload; the printer verifies the payload is
    present before indexing and spelling it.
  - Final mnemonic/register/text validation:
    `print_branch` validates target label text, branch mnemonic spelling,
    materialized-bool condition operand kind/register payload, condition-code
    suffix, compare operand printability, and final `b`/`b.<cond>`/`cmp`
    assembly lines. These are printer-owned final spelling checks.
  - Non-overlap:
    status checks conditional `condition_type` identity and
    `can_fuse_with_branch`/candidate fusable flags; the printer does not repeat
    those exact status decisions except by relying on `validate_selected_machine_node`.

- Scalar: mixed repeated semantic validation and final printer validation.
  - Repeated semantic validation:
    `scalar_selection_status` selects scalar ALU only when
    `supported_integer_operation || supported_floating_operation` and
    `operation != ScalarAluOperationKind::Deferred`; ALU printer routes are also
    gated by `supported_integer_operation` or `supported_floating_operation`
    before spelling scalar ALU forms. Status uses these flags to classify the
    operation subset; the printer uses the same flags to enter printable ALU
    paths.
  - Repeated semantic validation:
    status selects scalar unary only when `supported_integer_operation` and
    `operation != ScalarUnaryOperationKind::Deferred`; the unary printer path is
    gated by `scalar_unary->supported_integer_operation` and later rejects an
    empty mnemonic for `Deferred`. Status classifies the selected subset; the
    printer revalidates that the selected unary operation has a mnemonic.
  - Repeated semantic validation:
    status selects scalar casts only when one of
    `supported_simple_integer_cast`, `supported_float_integer_conversion`, or
    `supported_float_width_conversion` is true and
    `operation != ScalarCastOperationKind::Deferred`. The cast printers repeat
    those support-flag gates and reject `Deferred` before emitting conversion
    or integer-cast text. Status decides selectedness; the printer ensures the
    supported cast category has a printable encoding.
  - Final mnemonic/register/address/text validation:
    scalar printers validate result register presence, input operand counts,
    operand kinds, integer/floating register views, scratch availability,
    memory-source address printability, stack-publication lines, immediate
    materialization, and exact mnemonics such as `neg`, `mvn`, `clz`, `fcvt`,
    `scvtf`, `ucvtf`, `mul`, `sdiv`, `udiv`, and shift/reduction spellings.
    These are printer-owned final register/address/text checks.
  - Non-overlap:
    status treats a missing scalar ALU/unary/cast record as
    `MissingRequiredFacts`; the printer generally fails later through missing
    destination metadata or an ALU/cast diagnostic rather than repeating that
    exact record-family presence check.

- Address materialization: mixed repeated semantic validation and final
  relocation/address/text validation.
  - Repeated semantic validation:
    status and printer both reject `GotPageLow12` unless
    `address_materialization_policy == GotRequired`. Status checks that the
    selected machine node carries the required GOT policy; the printer repeats
    it before emitting `:got:` and `:got_lo12:` relocation operands.
  - Repeated semantic validation:
    status and printer both reject TLS-relative materialization unless the TLS
    model/thread-pointer facts match local-exec AArch64 TPIDR_EL0 materializing
    facts, and both require `tls_high_relocation == Aarch64TprelHi12` plus
    `tls_low_relocation == Aarch64TprelLo12Nc`. Status verifies the semantic TLS
    lowering facts; the printer repeats them before spelling `mrs tpidr_el0`
    and the paired TLS relocation operands.
  - Final relocation/address/text validation:
    the printer checks `result_register`, concrete label strings
    (`symbol_label`, `text_label`, `target_label_name`), relocation-prefix
    spelling, frame-slot base choice, byte-offset assembly, and exact `adrp`,
    `add`, `ldr`, and `mrs` lines. These are final printer-owned spelling and
    address emission checks.
  - Non-overlap:
    status checks prepared provenance (`source_materialization`,
    `function_name`, `block_label`), result value identity/home,
    `text_name`, `target_label`, `frame_slot_id`, `symbol_name`, TLS address
    space flags, and `DeferredUnsupported` kind. The printer does not repeat
    those exact source/provenance checks; it validates printable labels and
    relocations instead.

- Spill/reload: mixed repeated semantic validation and final address/register
  validation.
  - Repeated semantic validation:
    both status and printer require `slot_id` and `stack_offset_bytes`.
    Status classifies a spill/reload node without slot/offset facts as missing
    required facts; the printer repeats the condition because it needs a
    prepared stack-slot address to spell the pseudo.
  - Repeated semantic validation:
    both require `scratch` to be present. Status treats missing scratch facts as
    an unselected node; the printer needs the scratch register to print the
    spill/reload register operand and to avoid unsafe address materialization.
  - Final address/register/text validation:
    the printer additionally requires `stack_offset_is_prepared_snapshot`,
    `slot.support == Prepared`, `slot.base_kind == FrameSlot`,
    `slot.can_use_base_plus_offset`, `printable_memory_address`, non-empty
    pseudo mnemonic, scratch register spelling, and final load/store line text.
    These are printer-owned final address/register/mnemonic checks.
  - Non-overlap:
    status checks `pseudo_kind == None` and rematerialize ops as
    `DeferredUnsupported`, and it requires
    `occupied_scratch_register_references` plus
    `scratch_register_authority`. The printer does not repeat those exact
    authority/selection-subset checks, except that it later rejects an empty
    mnemonic for unsupported pseudo spelling.

- Frame: mixed repeated semantic validation and final frame text validation.
  - Repeated semantic validation:
    status and printer both require valid `function_name`,
    nonzero `frame_alignment_bytes`, and either `source_frame` or
    `preserves_link_register`. Status uses these prepared-frame facts to mark
    the machine node selected; the printer repeats them before emitting frame
    setup/teardown text.
  - Repeated semantic validation:
    both require `link_register_save_offset_bytes` when
    `preserves_link_register` is true. Status checks selectedness of
    link-register preserving frame nodes; the printer needs the offset to spell
    the `x30` save/restore lines.
  - Final frame/register/address/text validation:
    the printer validates dynamic-stack printability, rejects `callee_save`
    frame nodes as outside the printable subset, restricts frame kinds to
    prologue/epilogue setup/teardown, validates frame-pointer save offsets and
    saved callee-register slot placements, checks non-empty frame mnemonic, and
    emits `sp`, `x29`, `x30`, `str`, `ldr`, `mov`, and frame-adjustment text.
    These are final printer-owned frame spelling/address checks.
  - Non-overlap:
    status accepts callee-save store/load when `callee_save` exists, while
    `print_frame` currently rejects any `callee_save` as outside the printable
    subset. That is intentional owner divergence, not duplicate validation.

Step 3 route decision:

No-code evidence conclusion. Do not implement an AArch64-local semantic
validation helper for this probe.

Reason:

- The repeated checks proven by Step 2 are real, but they do not form one
  coherent helper boundary that both owners can consume without blurring
  responsibilities. Branch, scalar, address materialization, spill/reload, and
  frame each repeat different family-specific facts for different immediate
  purposes.
- `validate_selected_machine_node` already blocks non-selected machine nodes
  before family printers run. That makes the status helpers the selectedness
  authority for the semantic facts they classify.
- The printer-side repeats are intentionally target-local print-safety guards:
  they protect immediate operand indexing, optional dereference, relocation
  selection, address rendering, register spelling, and final line emission at
  the point where `machine_printer.cpp` needs those facts.
- Centralizing the repeated checks would either duplicate the existing
  selection status contract under a new name, or require moving printer safety
  and final spelling decisions out of `machine_printer.cpp`, which the source
  idea and runbook explicitly reject.

Concrete duplicated semantic checks that remain separately target-local:

- Branch: conditional `target_pair`/`condition_record` presence; fused compare
  `predicate`/`compare_operands` presence; fused compare operand count before
  reading `instruction.operands[3]` and `[4]`.
- Scalar: scalar ALU/unary/cast supported-subset gates and non-`Deferred`
  operation checks before entering printable scalar forms.
- Address materialization: GOT-required policy for `GotPageLow12`; local-exec
  TLS model/thread-pointer facts and AArch64 TPREL relocation facts for
  `TlsRelative`.
- Spill/reload: `slot_id`, `stack_offset_bytes`, and `scratch` presence before
  rendering the prepared frame-slot pseudo.
- Frame: `function_name`, `frame_alignment_bytes`, `source_frame` or
  `preserves_link_register`, and link-register save offset presence for
  link-register-preserving frame nodes.

Final assembly text output and spelling checks stay in `machine_printer.cpp`.
That includes branch target labels and mnemonics, scalar result/source register
views and ALU/cast mnemonics, address labels and relocation operand spelling,
spill/reload printable memory addresses and scratch register spelling, and
frame mnemonics/register/address lines.

## Suggested Next

Proceed to `plan.md` Step 5 acceptance review and lifecycle decision. Step 4
implementation is not recommended for this probe because the selected route is
the no-code evidence conclusion above.

## Watchouts

- Do not change implementation files for this probe unless the supervisor or
  plan owner intentionally rewrites the route.
- Do not fold printer checks for the Step 2 families; the accepted Step 3 route
  treats them as intentionally separate printer-local safety/spelling guards.
- Keep final assembly spelling validation in the printer.
- `validate_selected_machine_node` already blocks non-selected machine nodes
  before family printers run; the repeated printer checks remain local guards
  before indexing operands or spelling text.
- `MemoryInstructionRecord` has prepared-record builders in `instruction.hpp`,
  but no dedicated `*_selection_status` helper in `instruction.cpp`; the printer
  has substantial load/store validation in `print_memory` and
  `print_symbol_memory`.
- `AtomicMemoryInstructionRecord`, call-boundary, call, return, intrinsic,
  i128/f128, and assembler families are printer-dispatched without a matching
  local status helper in `instruction.cpp`.

## Proof

No build required by delegated proof; evidence-only `todo.md` update for
`plan.md` Step 3. No `test_after.log` was produced.
