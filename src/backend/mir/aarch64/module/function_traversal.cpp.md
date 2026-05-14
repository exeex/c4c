# `src/backend/mir/aarch64/module/function_traversal.cpp` Replacement Draft

## Purpose

`function_traversal.cpp` owns prepared function and block traversal for the
AArch64 module replacement. It consumes validated `prepare::PreparedBirModule`
state and constructs canonical `MachineFunction` and `MachineBlock` carriers
for downstream instruction, branch/control, call, and operand lowering.

This draft is Stage 3 replacement design. It does not describe an existing
legacy file and does not preserve the legacy catch-all record assembler under a
new name.

## Classification

Core traversal.

This file is the per-function traversal and lowering-context seam. It is not
module dispatch, operation-specific instruction lowering, public assembly
bridging, or compatibility projection.

## Traversal Contract

The traversal entry accepts the validated prepared module, the resolved AArch64
target profile, and module-dispatch services needed to report errors. It
returns completed canonical functions:

```cpp
std::vector<MachineFunction> lower_prepared_functions(
    const prepare::PreparedBirModule& prepared,
    const c4c::TargetProfile& target_profile,
    ModuleLoweringDiagnostics& diagnostics);
```

The spelling above is illustrative. The required contract is that traversal
returns `MachineFunction` objects whose `MachineBlock` lists contain ordered
target `MachineInstruction` nodes and successor metadata. Any compatibility
records are side products or later projections, not the traversal output
authority.

## Prepared Function Order

Traversal order comes from prepared control-flow authority, not source-text
recovery:

1. Iterate `prepared.control_flow.functions` in prepared order.
2. Resolve function identity from stable prepared IDs and labels.
3. Create a `MachineFunction` with a `LabelSymbol`, ordered `MachineBlock`
   container, and function-level `Provenance`.
4. Build a per-function lowering context before visiting blocks.
5. Traverse prepared blocks in the prepared block order for that function.

Source BIR functions may contribute optional debug/source provenance only when
there is a durable prepared identity link. They must not drive function order
or replace missing prepared control-flow identity by spelling.

## Prepared Block Order

Block traversal follows prepared block identity and successor facts:

1. For each prepared block, create a `MachineBlock` with `BlockLabelId`,
   `LabelSymbol`, successor storage, instruction storage, and block
   `Provenance`.
2. Lower non-control operations into the current block through instruction
   lowering.
3. Lower calls through call lowering when the prepared operation is governed by
   a prepared call plan.
4. Lower terminators through branch/control lowering, including successor
   metadata.
5. Seal the block only after control nodes and successor facts agree.

Traversal does not print labels and does not decide assembly section or file
structure. The shared `mir_printer` later owns traversal for emission.

## Lowering Context Construction

The per-function context gathers typed access to prepared authorities without
turning them into broad public records:

- prepared function identity and block identity maps
- frame, stack-layout, and dynamic-stack facts
- storage-plan entries and value homes
- regalloc assignments and target register classes
- spill/reload plans and spill-slot identities
- parallel-copy bundles and resolved move-step authorities
- prepared call plans and ABI call-site facts
- durable source-location references for optional provenance
- diagnostic sink and target profile

The context exposes narrow query services to owned lowerers. For example,
operand resolution receives typed prepared value-location facts; call lowering
receives prepared call-plan facts; branch/control lowering receives prepared
terminators and successor identities. Lowerers should not reach around the
context into broad prepared tables.

## MIR Function And Block Creation

Traversal creates the hierarchy before operation lowering emits nodes:

```cpp
MachineFunction function {
  .function_name = prepared_function.function_name,
  .label = make_function_label(prepared_function),
  .blocks = {},
  .provenance = provenance_for_prepared_function(prepared_function),
};

MachineBlock block {
  .block_label = prepared_block.block_label,
  .label = make_block_label(prepared_block),
  .instructions = {},
  .successors = {},
  .provenance = provenance_for_prepared_block(prepared_block),
};
```

Instruction, branch/control, and call lowerers append `MachineInstruction`
nodes to the current `MachineBlock`. Traversal owns block insertion order and
the active insertion point; operation lowerers own the target nodes they append.

## Optional Debug Provenance

Traversal may attach optional `Provenance` metadata to functions, blocks, and
diagnostic sideband records. Allowed provenance includes stable IDs, prepared
indices, labels, durable source locations, and reason tags such as
`PreparedControlFlowFunction`, `PreparedBlockOrder`, or
`PreparedSourceLocation`.

Optional provenance must remain metadata. It cannot:

- recover missing prepared function or block identity by source spelling
- retain raw prepared/source pointers as lowering authority
- make cached display strings authoritative for labels, operands, or
  instructions
- require the public `Module` product to depend on caller-owned prepared
  lifetime

## Owned Inputs

- validated `prepare::PreparedBirModule`
- resolved AArch64 `TargetProfile`
- prepared control-flow function and block order
- prepared frame, storage, value-home, regalloc, spill/reload,
  parallel-copy, and call-plan facts
- prepared terminators, operation lists, successor identities, and labels
- durable source-location references when already linked by prepared identity
- operand, instruction, branch/control, and call lowering seams

## Owned Outputs

- ordered `MachineFunction` carriers
- ordered `MachineBlock` carriers
- block successor metadata
- function/block `Provenance`
- per-function typed context passed to subordinate lowerers
- diagnostics for missing or inconsistent prepared authority

Traversal may also produce private sideband facts that compatibility projection
uses after canonical lowering. Such sideband facts must be derived from the same
typed traversal and cannot replace canonical MIR output.

## Indirect Queries

Traversal may indirectly query:

- operand resolution for typed `MachineOperand` and `MachineRegister` values
- instruction lowering for non-control target `MachineInstruction` nodes
- branch/control lowering for terminators, branch nodes, return control, and
  successor metadata
- call lowering for prepared call-plan machine sequences

Traversal must not query the public assembly bridge, compatibility projection,
or flat `FunctionRecord::machine_nodes` while deciding what to lower.

## Forbidden Knowledge

`function_traversal.cpp` must not know:

- AArch64 assembly text syntax, punctuation, spacing, or file structure
- target render method internals beyond storing target-owned nodes
- operation-specific scalar, memory, move, spill/reload, branch, return, or
  call instruction-selection algorithms
- register-string fallback policy except through operand-resolution diagnostics
- public broad inspection records as semantic inputs
- source function/block spelling recovery as the primary identity model
- compatibility projection layout beyond optional sideband data that is derived
  after canonical traversal

## Replacement Rejection Signals

Reject an implementation of this draft if it:

- walks source BIR functions instead of prepared control-flow functions as the
  authoritative order
- creates flat machine-node vectors before canonical `MachineBlock`
  instructions
- lets `FunctionRecord::machine_nodes` drive instruction, branch, or call
  lowering
- expands narrow returned-`add`/`sub` or other testcase-shaped paths inside
  traversal instead of delegating semantic families to lowerers
- recovers missing block labels, function names, registers, or operands from
  strings when structured prepared facts should exist
- duplicates the module-wide legacy record assembler inside a per-function
  context object
