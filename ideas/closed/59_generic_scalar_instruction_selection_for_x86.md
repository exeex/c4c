# Generic Scalar Instruction Selection For Prepared X86

Status: Closed
Created: 2026-04-18
Last-Updated: 2026-04-20
Closed: 2026-04-20
Disposition: Completed; prepared x86 scalar lowering now routes through ordinary per-operation and per-terminator dispatch over authoritative prepared inputs instead of growing bounded whole-function matcher families for the covered scalar route.
Depends-On:
- idea 58 shared CFG, join, and loop materialization
- idea 60 prepared value-location consumption
- idea 61 stack-frame and addressing consumption

## Why This Was Closed

Idea 59 was about replacing matcher-shaped prepared x86 scalar lowering with a
selector-driven dispatch surface over prepared operands, value homes,
addressing facts, and terminator semantics. That route is now complete on its
stated scope: the covered scalar instruction, call, and branch families dispatch
through explicit helpers and prepared context instead of depending on new
whole-function or named-testcase matcher growth.

## What Landed Before Closure

- prepared x86 lowering now exposes shared function and block dispatch context
  surfaces instead of repeatedly unpacking raw whole-function helper arguments
- covered scalar operand, compare, call-lane, and local-memory legality
  questions are answered through selector helpers framed around prepared inputs
  and x86 legality
- covered scalar instruction families lower through explicit per-operation
  dispatch surfaces rather than inline matcher-shaped control flow
- covered compare/boolean branch and bounded scalar call families route through
  prepared per-terminator or per-call selection instead of whole-function x86
  shape recognition
- remaining x86-only decisions on the covered route are machine spelling and
  legality, not semantic recovery from raw function shape

## Validation At Closure

Closure used a backend-scoped regression guard:

- `cmake --build --preset default`
- `ctest --test-dir build -j --output-on-failure -R '^backend_'`
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result:

- regression guard passed
- before and after both reported `68` passed / `4` failed / `72` total
- the same four pre-existing backend route failures remained:
  - `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`
  - `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`
  - `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`
  - `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`

## Follow-On Context

- `ideas/open/57_x86_backend_c_testsuite_capability_families.md` remains the
  umbrella route for broader backend capability debt that still fails before the
  canonical prepared-module handoff
- idea 59 closure does not claim to resolve unresolved variadic or dynamic
  member-array semantic lowering outside the covered scalar instruction-selection
  route

## Intent

This idea defines the per-instruction and per-terminator lowering layer that
should sit between prepared BIR handoff data and final x86 asm text emission.

It rejects the current route where x86 succeeds mainly by recognizing bounded
whole-function shapes.

## What "Instruction Selection" Means Here

Instruction selection here does not mean:

`BIR_ADD -> add`

It means:

- inspect one prepared operation
- inspect its prepared operands, value homes, addressing facts, and branch
  semantics
- choose a legal x86 sequence

Sometimes that sequence is one instruction.
Sometimes it is several.
Sometimes it fuses with a branch.
Sometimes it requires a scratch register.

## Concrete Functions To Exist

The x86 backend should have a dispatch surface like this:

```cpp
struct X86PreparedFunctionContext;
struct X86PreparedBlockContext;

std::string emit_prepared_function(const prepare::PreparedBirModule& module,
                                   const bir::Function& function);

void emit_prepared_block(const bir::Block& block,
                         const X86PreparedBlockContext& ctx,
                         std::vector<std::string>* out);

void emit_prepared_inst(const bir::Inst& inst,
                        const X86PreparedBlockContext& ctx,
                        std::vector<std::string>* out);

void emit_prepared_terminator(const bir::Terminator& term,
                              const X86PreparedBlockContext& ctx,
                              std::vector<std::string>* out);
```

The operation-specific selectors should then be small and ordinary:

```cpp
void emit_binop(const bir::BinaryInst& inst,
                const X86PreparedBlockContext& ctx,
                std::vector<std::string>* out);

void emit_load_local(const bir::LoadLocalInst& inst,
                     const X86PreparedBlockContext& ctx,
                     std::vector<std::string>* out);

void emit_store_local(const bir::StoreLocalInst& inst,
                      const X86PreparedBlockContext& ctx,
                      std::vector<std::string>* out);

void emit_select(const bir::SelectInst& inst,
                 const X86PreparedBlockContext& ctx,
                 std::vector<std::string>* out);

void emit_call(const bir::CallInst& inst,
               const X86PreparedBlockContext& ctx,
               std::vector<std::string>* out);

void emit_cond_branch(const bir::Terminator& term,
                      const X86PreparedBlockContext& ctx,
                      std::vector<std::string>* out);
```

Helper selectors should answer legality questions, not whole-shape questions:

```cpp
X86ValueOperand select_value_operand(const bir::Value& value,
                                     const X86PreparedBlockContext& ctx);

X86MemoryOperand select_memory_operand(const bir::MemoryAddress& address,
                                       const X86PreparedBlockContext& ctx);

X86ComparePlan select_compare_plan(const prepare::PreparedBranchCondition& cond,
                                   const X86PreparedBlockContext& ctx);

X86CallPlan select_call_plan(const bir::CallInst& inst,
                             const X86PreparedBlockContext& ctx);
```

## Prepared Inputs Required

This idea depends on upstream data being present.
No new BIR opcodes are required, but the selector must be able to query:

- branch-condition semantics from idea 58
- join-transfer semantics from idea 58
- value homes from idea 60
- call/return/join move bundles from idea 60
- frame slot and memory-address facts from idea 61

Suggested context sketch:

```cpp
struct X86PreparedBlockContext {
  const prepare::PreparedBirModule& module;
  const prepare::PreparedControlFlowFunction* control_flow = nullptr;
  const prepare::PreparedValueLocationFunction* value_locations = nullptr;
  const prepare::PreparedAddressingFunction* addressing = nullptr;
  const bir::Function* function = nullptr;
  const bir::Block* block = nullptr;
};
```

## Producer / Consumer Boundary

### Produced Upstream

- compare-vs-bool branch meaning
- assigned register or stack home for each value
- required moves at join, call, and return boundaries
- canonical frame-slot and address provenance

### Consumed Here

- legal operand forms
- scratch-based fallbacks when x86 forbids a direct form
- fused compare-and-branch vs materialized-bool branch emission
- concrete call sequence spelling

### Must Not Be Re-Derived Here

- join meaning from CFG shape
- stack offset from local-slot order
- value home from ad hoc `eax` conventions

## Code Examples

### Example 1: Add With Two Stack-Resident Inputs

Input prepared facts:

```cpp
// %a in stack slot 16
// %b in stack slot 24
// %sum result home in register eax
```

Selector result:

```cpp
mov eax, DWORD PTR [rsp + 16]
add eax, DWORD PTR [rsp + 24]
```

This is still one logical `BinaryInst::Add`, but not one x86 opcode mapping.

### Example 2: Compare Feeding Only A Branch

Prepared branch condition:

```cpp
PreparedBranchCondition{
  .kind = PreparedBranchConditionKind::FusedCompare,
  .predicate = bir::BinaryOpcode::Eq,
  .compare_type = bir::TypeKind::I32,
  .lhs = bir::Value::named(TypeKind::I32, "%x"),
  .rhs = bir::Value::immediate_i32(0),
  .true_label = "then",
  .false_label = "else",
};
```

Selector result:

```cpp
test eax, eax
je .Lthen
jmp .Lelse
```

No separate "bool materialization lane" is needed.

## Acceptance Shape

This idea is satisfied when x86 scalar lowering is organized as ordinary
per-operation and per-terminator dispatch, and the remaining code paths are
bounded by machine legality rather than by whole-function matcher families.

## Non-Goals

- introducing a separate machine IR in the same slice
- x86 peephole optimization
- target-independent SSA rewrites
