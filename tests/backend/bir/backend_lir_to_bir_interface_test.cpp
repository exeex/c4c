#include "src/backend/bir/bir.hpp"
#include "src/codegen/lir/ir.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace bir = c4c::backend::bir;
namespace lir = c4c::codegen::lir;

namespace {

[[noreturn]] void fail(const std::string& message) {
  std::cerr << "FAIL: " << message << '\n';
  std::exit(1);
}

void expect(bool condition, const std::string& message) {
  if (!condition) fail(message);
}

lir::LirFunction void_declaration(std::string name) {
  lir::LirFunction function;
  function.name = std::move(name);
  function.is_declaration = true;
  function.signature_return_type_ref = lir::LirTypeRef("void");
  return function;
}

lir::LirFunction void_definition(std::string name,
                                 std::vector<lir::LirBlock> blocks) {
  lir::LirFunction function;
  function.name = std::move(name);
  function.signature_return_type_ref = lir::LirTypeRef("void");
  function.blocks = std::move(blocks);
  function.entry = function.blocks.front().id;
  return function;
}

lir::LirBlock return_block(std::uint32_t id, std::string label) {
  lir::LirBlock block;
  block.id = lir::LirBlockId{id};
  block.label = std::move(label);
  block.terminator = lir::LirRet{std::nullopt, "void"};
  return block;
}

lir::LirInlineAsmOp void_inline_asm(std::string asm_text,
                                    std::string constraints) {
  lir::LirInlineAsmOp op;
  op.ret_type = lir::LirTypeRef("void");
  op.original_asm_text = asm_text;
  op.original_constraint_text = constraints;
  op.asm_text = std::move(asm_text);
  op.constraints = std::move(constraints);
  op.side_effects = true;
  op.clobbers = {"memory", "cc"};
  return op;
}

void test_supported_import_and_views() {
  lir::LirModule module;
  module.functions.push_back(void_declaration("decl"));
  module.functions.push_back(
      void_definition("returns", {return_block(0, "entry")}));

  lir::LirBlock branch_entry;
  branch_entry.id = lir::LirBlockId{0};
  branch_entry.label = "entry";
  branch_entry.terminator = lir::LirBr{"exit"};
  module.functions.push_back(void_definition(
      "branches", {std::move(branch_entry), return_block(1, "exit")}));

  std::string asm_bytes = "add %0, %1";
  asm_bytes.push_back('\0');
  asm_bytes += "#opaque";
  std::string constraint_bytes = "=r,r,VR,VRM2,~{memory}";
  constraint_bytes.push_back('\0');
  constraint_bytes += "tail";
  auto asm_block = return_block(0, "entry");
  auto opaque_asm = void_inline_asm(asm_bytes, constraint_bytes);
  opaque_asm.asm_text = "llvm compatibility template";
  opaque_asm.constraints = "~{compatibility-only}";
  asm_block.insts.push_back(std::move(opaque_asm));
  module.functions.push_back(
      void_definition("inline_asm", {std::move(asm_block)}));

  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(), "supported LIR subset should publish RawBir");

  const bir::ModuleView module_view = imported.value().view();
  const auto functions = module_view.functions();
  expect(functions.size() == 4, "module view should preserve all functions");

  auto declaration = module_view.function(functions[0]);
  expect(declaration.has_value() && declaration.value().is_declaration(),
         "declaration should remain a declaration");
  expect(declaration.value().link_name() == "decl",
         "declaration should preserve its link name");
  expect(declaration.value().blocks().empty(),
         "declaration should not acquire a body");

  auto returns = module_view.function(functions[1]);
  expect(returns.has_value(), "returning definition should resolve");
  const auto return_blocks = returns.value().blocks();
  expect(return_blocks.size() == 1, "returning definition should have one block");
  auto return_term = returns.value().terminator(return_blocks[0]);
  expect(return_term.has_value() &&
             std::holds_alternative<bir::ReturnTerm>(return_term.value()),
         "void LIR return should become a BIR ReturnTerm");

  auto branches = module_view.function(functions[2]);
  expect(branches.has_value(), "branching definition should resolve");
  const auto branch_blocks = branches.value().blocks();
  expect(branch_blocks.size() == 2, "branching definition should preserve block order");
  auto successors = branches.value().successors(branch_blocks[0]);
  expect(successors.has_value() && successors.value().size() == 1 &&
             successors.value()[0] == branch_blocks[1],
         "CFG successor view should resolve the imported branch target");

  auto inline_asm_function = module_view.function(functions[3]);
  expect(inline_asm_function.has_value(), "inline-asm function should resolve");
  const auto asm_blocks = inline_asm_function.value().blocks();
  auto asm_insts = inline_asm_function.value().instructions(asm_blocks[0]);
  expect(asm_insts.has_value() && asm_insts.value().size() == 1,
         "inline asm should publish one stable instruction identity");
  auto asm_inst = inline_asm_function.value().instruction(asm_insts.value()[0]);
  expect(asm_inst.has_value() &&
             asm_inst.value().opcode() == bir::Opcode::InlineAsm,
         "instruction view should expose the closed InlineAsm opcode");
  const auto* payload =
      std::get_if<bir::InlineAsmNode>(&asm_inst.value().payload());
  expect(payload != nullptr, "InlineAsm opcode should carry InlineAsmNode");
  expect(payload->asm_text == asm_bytes,
         "inline asm template bytes must remain byte-exact and opaque");
  expect(payload->constraint_text == constraint_bytes,
         "aggregate constraint bytes must remain byte-exact and opaque");
  expect(payload->side_effects &&
             payload->clobbers == std::vector<std::string>({"memory", "cc"}),
         "side-effect and ordered clobber fields must remain separate");
  expect(asm_inst.value().operands().empty() &&
             asm_inst.value().results().empty(),
         "void/no-argument LIR asm must not acquire invented value identities");

  auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(),
         "the same target-independent storage should pass canonical verification");
  expect(canonical.value().view().functions().size() == functions.size(),
         "canonical publication should preserve the verified graph");
}

void test_generic_inline_asm_ssa_edges() {
  bir::ModuleBuilder builder;
  bir::FunctionSignature signature;
  signature.return_type = bir::Type{bir::TypeKind::I64};
  signature.parameter_types = {bir::Type{bir::TypeKind::I64}};
  auto function = builder.create_function(std::move(signature), "ssa_asm", false);
  expect(function.has_value(), "SSA asm test function should be constructible");

  bir::BlockId block_id{};
  bir::InstId instruction_id{};
  bir::ValueId parameter_id{};
  bir::ValueId result_id{};
  auto edited = builder.with_function(
      function.value(), [&](bir::FunctionBuilder& function_builder) {
        auto block = function_builder.create_block("entry");
        if (!block) return bir::Result<void, bir::BuildError>::failure(block.error());
        block_id = block.value();
        auto parameter = function_builder.parameter(0);
        if (!parameter)
          return bir::Result<void, bir::BuildError>::failure(parameter.error());
        parameter_id = parameter.value();

        bir::InlineAsmSpec malformed;
        malformed.asm_text = "opaque";
        malformed.constraint_text = "+r";
        malformed.inputs = {
            bir::ValueId{function.value(), bir::ValueKind::Parameter, 999, 1}};
        auto rejected = function_builder.append(block_id, std::move(malformed));
        expect(!rejected.has_value() &&
                   rejected.error() == bir::BuildError::InvalidValue,
               "missing SSA input must be rejected before instruction publication");

        bir::InlineAsmSpec spec;
        spec.asm_text = "# uses abstract SSA only";
        spec.constraint_text = "+r";
        spec.clobbers = {"memory"};
        spec.side_effects = true;
        spec.inputs = {parameter_id};
        spec.result_types = {bir::Type{bir::TypeKind::I64}};
        auto appended = function_builder.append(block_id, std::move(spec));
        if (!appended)
          return bir::Result<void, bir::BuildError>::failure(appended.error());
        instruction_id = appended.value().instruction;
        result_id = appended.value().results[0];
        return function_builder.set_terminator(
            block_id, bir::ReturnTerm{result_id});
      });
  expect(edited.has_value(), "generic SSA asm construction should succeed");

  auto published = std::move(builder).publish();
  expect(published.has_value(),
         "generic inline-asm use/def edges should pass Raw BIR verification");
  auto function_view = published.value().view().function(function.value());
  expect(function_view.has_value(), "published SSA asm function should resolve");
  auto instructions = function_view.value().instructions(block_id);
  expect(instructions.has_value() && instructions.value().size() == 1 &&
             instructions.value()[0] == instruction_id,
         "failed append must leave no partial instruction in block order");
  auto instruction = function_view.value().instruction(instruction_id);
  expect(instruction.has_value() &&
             instruction.value().operands() ==
                 std::vector<bir::ValueId>({parameter_id}) &&
             instruction.value().results() ==
                 std::vector<bir::ValueId>({result_id}),
         "InlineAsm must expose generic ordered SSA input and output edges");
  auto result = function_view.value().value(result_id);
  const auto* definition = result.has_value()
                               ? std::get_if<bir::InstResultDef>(
                                     &result.value().definition)
                               : nullptr;
  expect(definition && definition->instruction == instruction_id &&
             definition->result_index == 0,
         "inline-asm output must own a coherent InstResultDef coordinate");
  expect(parameter_id != result_id,
         "read/write asm must keep incoming use and produced result distinct");
}

void test_structured_lir_import_ssa_chain() {
  const auto binding = [](std::string value, std::string type,
                          lir::LirInlineAsmValueRole role,
                          std::size_t constraint_index) {
    return lir::LirInlineAsmValueBinding{
        lir::LirOperand(std::move(value)), lir::LirTypeRef(std::move(type)),
        role, constraint_index};
  };

  std::string semantic_asm = "semantic read/write";
  semantic_asm.push_back('\0');
  semantic_asm += "opaque tail";
  std::string semantic_constraints = "+r";
  semantic_constraints.push_back('\0');
  semantic_constraints += "opaque tail";

  auto producer = void_inline_asm("semantic producer", "=r");
  producer.asm_text = "llvm producer";
  producer.constraints = "=compat";
  producer.args_str = "compatibility producer args";
  producer.ordinary_results = {
      binding("%seed", "i64", lir::LirInlineAsmValueRole::Output, 0)};

  auto read_write = void_inline_asm(semantic_asm, semantic_constraints);
  read_write.asm_text = "llvm read/write";
  read_write.constraints = "+compat";
  read_write.args_str = "compatibility read/write args";
  read_write.clobbers = {"cc", "memory", "x7"};
  read_write.ordinary_inputs = {
      binding("%seed", "i64", lir::LirInlineAsmValueRole::ReadWrite, 0)};
  read_write.ordinary_results = {
      binding("%next", "i64", lir::LirInlineAsmValueRole::ReadWrite, 0)};

  auto consumer = void_inline_asm("semantic consumer", "r");
  consumer.asm_text = "llvm consumer";
  consumer.constraints = "compat";
  consumer.args_str = "compatibility consumer args";
  consumer.ordinary_inputs = {
      binding("%next", "i64", lir::LirInlineAsmValueRole::Input, 0)};

  lir::LirModule module;
  auto block = return_block(0, "entry");
  block.insts = {std::move(producer), std::move(read_write),
                 std::move(consumer)};
  module.functions.push_back(
      void_definition("structured_chain", {std::move(block)}));

  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(),
         "structured LIR inline-asm chain should publish RawBir");
  const auto module_view = imported.value().view();
  const auto function_ids = module_view.functions();
  expect(function_ids.size() == 1,
         "structured chain should preserve one function identity");
  auto function = module_view.function(function_ids[0]);
  expect(function.has_value(), "structured chain function should resolve");
  const auto blocks = function.value().blocks();
  auto instructions = function.value().instructions(blocks[0]);
  expect(instructions.has_value() && instructions.value().size() == 3,
         "structured chain should preserve stable instruction order");

  auto first = function.value().instruction(instructions.value()[0]);
  auto second = function.value().instruction(instructions.value()[1]);
  auto third = function.value().instruction(instructions.value()[2]);
  expect(first.has_value() && second.has_value() && third.has_value() &&
             first.value().results().size() == 1 &&
             second.value().operands() == first.value().results() &&
             second.value().results().size() == 1 &&
             third.value().operands() == second.value().results(),
         "ordinary result map should connect producer, read/write, and consumer");
  expect(second.value().operands()[0] != second.value().results()[0],
         "read/write import must retain distinct old and new SSA identities");
  const auto* payload =
      std::get_if<bir::InlineAsmNode>(&second.value().payload());
  expect(payload && payload->asm_text == semantic_asm &&
             payload->constraint_text == semantic_constraints &&
             payload->clobbers ==
                 std::vector<std::string>({"cc", "memory", "x7"}) &&
             payload->side_effects,
         "BIR payload must preserve only original semantic authority");
  auto result = function.value().value(second.value().results()[0]);
  expect(result.has_value() && result.value().type == bir::Type{bir::TypeKind::I64},
         "structured result type should survive through the stable value view");

  auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(),
         "the same structured chain should publish CanonicalBir transactionally");
}

void test_structured_lir_import_rejections() {
  const auto binding = [](std::string value, std::string type,
                          lir::LirInlineAsmValueRole role,
                          std::size_t constraint_index) {
    return lir::LirInlineAsmValueBinding{
        lir::LirOperand(std::move(value)), lir::LirTypeRef(std::move(type)),
        role, constraint_index};
  };
  const auto expect_rejected = [](lir::LirModule module,
                                  const std::string& message) {
    auto raw = bir::lower_lir_to_raw_bir(module);
    expect(!raw.has_value() &&
               raw.error().code == bir::ImportErrorCode::UnsupportedInlineAsmShape,
           message + " (RawBir)");
    auto canonical = bir::lower_lir_to_canonical_bir(module);
    expect(!canonical.has_value() &&
               canonical.error().code ==
                   bir::ImportErrorCode::UnsupportedInlineAsmShape,
           message + " (CanonicalBir)");
  };
  const auto one_function_module = [](std::string name,
                                      std::vector<lir::LirInst> instructions) {
    lir::LirModule module;
    auto block = return_block(0, "entry");
    block.insts = std::move(instructions);
    module.functions.push_back(
        void_definition(std::move(name), {std::move(block)}));
    return module;
  };

  auto missing = void_inline_asm("missing", "r");
  missing.ordinary_inputs = {
      binding("%missing", "i64", lir::LirInlineAsmValueRole::Input, 0)};
  expect_rejected(one_function_module("missing", {missing}),
                  "missing structured input must fail without publication");

  auto first = void_inline_asm("first", "=r");
  first.ordinary_results = {
      binding("%dup", "i64", lir::LirInlineAsmValueRole::Output, 0)};
  auto duplicate = void_inline_asm("duplicate", "=r");
  duplicate.ordinary_results = {
      binding("%dup", "i64", lir::LirInlineAsmValueRole::Output, 0)};
  expect_rejected(one_function_module("duplicate", {first, duplicate}),
                  "duplicate structured result must fail without publication");

  auto mistyped = void_inline_asm("mistyped", "r");
  mistyped.ordinary_inputs = {
      binding("%dup", "i32", lir::LirInlineAsmValueRole::Input, 0)};
  expect_rejected(one_function_module("mistyped", {first, mistyped}),
                  "mistyped structured input must fail without publication");

  auto inconsistent = void_inline_asm("inconsistent", "+r");
  inconsistent.ordinary_inputs = {
      binding("%dup", "i64", lir::LirInlineAsmValueRole::ReadWrite, 0)};
  expect_rejected(one_function_module("inconsistent", {first, inconsistent}),
                  "unpaired read/write input must fail without publication");

  lir::LirModule foreign;
  auto foreign_def = return_block(0, "entry");
  foreign_def.insts = {first};
  foreign.functions.push_back(
      void_definition("foreign_def", {std::move(foreign_def)}));
  auto foreign_use = return_block(0, "entry");
  auto use = void_inline_asm("foreign use", "r");
  use.ordinary_inputs = {
      binding("%dup", "i64", lir::LirInlineAsmValueRole::Input, 0)};
  foreign_use.insts = {std::move(use)};
  foreign.functions.push_back(
      void_definition("foreign_use", {std::move(foreign_use)}));
  expect_rejected(std::move(foreign),
                  "foreign-function identity must fail without publication");
}

void test_lir_inline_asm_structured_value_contract() {
  static_assert(std::is_same_v<
                decltype(lir::LirInlineAsmValueBinding::value),
                lir::LirOperand>);
  const auto binding = [](std::string value, std::string type,
                          lir::LirInlineAsmValueRole role,
                          std::size_t constraint_index) {
    return lir::LirInlineAsmValueBinding{
        lir::LirOperand(std::move(value)), lir::LirTypeRef(std::move(type)), role,
        constraint_index};
  };
  const auto verify_op = [](lir::LirInlineAsmOp op) {
    lir::LirModule module;
    auto block = return_block(0, "entry");
    block.insts.push_back(std::move(op));
    auto function =
        void_definition("structured_lir_asm", {std::move(block)});
    function.signature_text = "define void @structured_lir_asm()";
    module.functions.push_back(std::move(function));
    lir::verify_module(module);
  };
  const auto expect_rejected = [&](lir::LirInlineAsmOp op,
                                   const std::string& message) {
    try {
      verify_op(std::move(op));
    } catch (const lir::LirVerifyError&) {
      return;
    }
    fail(message);
  };

  auto input = void_inline_asm("llvm input rendering", "r");
  input.original_asm_text = "opaque input %0";
  input.original_constraint_text = "r";
  input.ordinary_inputs = {
      binding("%input", "i64", lir::LirInlineAsmValueRole::Input, 0)};
  verify_op(input);
  expect(input.original_asm_text == "opaque input %0" &&
             input.asm_text == "llvm input rendering" &&
             input.side_effects &&
             input.clobbers == std::vector<std::string>({"memory", "cc"}),
         "semantic text, ordered clobbers, and side effects must remain "
         "distinct from LLVM compatibility rendering");

  auto output = void_inline_asm("llvm output rendering", "=r");
  output.original_asm_text = "opaque output %0";
  output.original_constraint_text = "=r";
  output.ordinary_results = {
      binding("%output", "i64", lir::LirInlineAsmValueRole::Output, 0)};
  verify_op(output);

  auto read_write = void_inline_asm("llvm read/write rendering", "+r,r");
  read_write.original_asm_text = "opaque read/write %0, %1";
  read_write.original_constraint_text = "+r,r";
  read_write.ordinary_inputs = {
      binding("%old", "i64", lir::LirInlineAsmValueRole::ReadWrite, 0),
      binding("%input", "i64", lir::LirInlineAsmValueRole::Input, 1)};
  read_write.ordinary_results = {
      binding("%new", "i64", lir::LirInlineAsmValueRole::ReadWrite, 0)};
  verify_op(read_write);
  expect(read_write.ordinary_inputs[0].value !=
             read_write.ordinary_results[0].value,
         "read/write LIR asm must use distinct ordinary input/result IDs");

  auto invalid_identity = input;
  invalid_identity.ordinary_inputs[0].value = lir::LirOperand();
  expect_rejected(std::move(invalid_identity),
                  "invalid ordinary input identity must be rejected");

  auto invalid_type = output;
  invalid_type.ordinary_results[0].type = lir::LirTypeRef("void");
  expect_rejected(std::move(invalid_type),
                  "void structured result type must be rejected");

  auto invalid_role = input;
  invalid_role.ordinary_inputs[0].role = lir::LirInlineAsmValueRole::Output;
  expect_rejected(std::move(invalid_role),
                  "output role in the ordered input list must be rejected");

  auto invalid_order = read_write;
  invalid_order.ordinary_inputs[0].constraint_index = 1;
  invalid_order.ordinary_inputs[1].constraint_index = 0;
  expect_rejected(std::move(invalid_order),
                  "structured bindings outside constraint order must be rejected");

  auto reused_identity = read_write;
  reused_identity.ordinary_results[0].value =
      reused_identity.ordinary_inputs[0].value;
  expect_rejected(std::move(reused_identity),
                  "read/write input/result identity reuse must be rejected");

  auto mismatched_read_write_type = read_write;
  mismatched_read_write_type.ordinary_results[0].type = lir::LirTypeRef("i32");
  expect_rejected(std::move(mismatched_read_write_type),
                  "read/write input/result type mismatch must be rejected");
}

void test_closed_typed_lir_type_receipt() {
  lir::LirModule module;
  auto texts = std::make_shared<c4c::TextTable>();
  module.link_name_texts = texts;
  module.struct_names.attach_text_table(texts.get());
  const c4c::StructNameId pair_id = module.struct_names.intern("%struct.Pair");

  const std::vector<lir::LirTypeRef> value_types = {
      lir::LirTypeRef::integer(257),
      lir::LirTypeRef("x86_fp80"),
      lir::LirTypeRef("ptr"),
      lir::LirTypeRef("<4 x i32>"),
      lir::LirTypeRef::vrm_register(8),
      lir::LirTypeRef("[3 x i16]"),
      lir::LirTypeRef::struct_type("%struct.Pair", pair_id),
      lir::LirTypeRef("{ i32, ptr }"),
      lir::LirTypeRef("i32 (i8)", lir::LirTypeKind::Function),
      lir::LirTypeRef("%opaque.Payload", lir::LirTypeKind::Opaque),
  };
  const std::vector<bir::TypeKind> expected_kinds = {
      bir::TypeKind::Integer,     bir::TypeKind::Floating,
      bir::TypeKind::Pointer,     bir::TypeKind::Vector,
      bir::TypeKind::VrmRegister, bir::TypeKind::Array,
      bir::TypeKind::Struct,      bir::TypeKind::Struct,
      bir::TypeKind::Function,    bir::TypeKind::Opaque,
  };
  auto producer = void_inline_asm(
      "typed producers", "=r,=r,=r,=r,=r,=r,=r,=r,=r,=r");
  for (std::size_t index = 0; index < value_types.size(); ++index)
    producer.ordinary_results.push_back(lir::LirInlineAsmValueBinding{
        lir::LirOperand("%typed." + std::to_string(index)), value_types[index],
        lir::LirInlineAsmValueRole::Output, index});
  auto block = return_block(0, "entry");
  block.insts.push_back(std::move(producer));
  module.functions.push_back(void_definition("typed_asm", {std::move(block)}));

  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(), "all value-capable LirTypeKind alternatives and "
                               "the void signature should publish typed RawBir");
  const auto view = imported.value().view();
  const auto functions = view.functions();
  expect(functions.size() == 1, "typed asm definition should survive");
  auto function = view.function(functions.front());
  expect(function.has_value() &&
             function.value().signature().return_type.kind == bir::TypeKind::Void,
         "function signature receipt should use the shared conversion for void");
  const auto typed_block = function.value().blocks().front();
  const auto typed_inst = function.value().instructions(typed_block).value().front();
  const auto results = function.value().instruction(typed_inst).value().results();
  expect(results.size() == value_types.size(),
         "each closed value type should retain an ordinary result identity");
  for (std::size_t index = 0; index < results.size(); ++index) {
    const bir::Type type = function.value().value(results[index]).value().type;
    expect(type.kind == expected_kinds[index] && bir::is_well_formed(type),
           "Raw value view should expose a well-formed closed type kind");
  }
  expect(function.value().value(results[0]).value().type.bit_width == 257,
         "integer widths beyond the legacy fixed enum must remain exact");
  const auto fp80 = function.value().value(results[1]).value().type;
  expect(fp80.bit_width == 80 && fp80.spelling == "x86_fp80",
         "floating spelling and typed width must remain exact");
  expect(function.value().value(results[6]).value().type.struct_name_id == pair_id,
         "struct receipt must preserve StructNameId identity");
}

void test_structured_type_spec_signature_receipt() {
  lir::LirModule module;
  module.functions.push_back(void_declaration("mirrored_void"));
  auto structured_only = void_declaration("structured_only_void");
  structured_only.signature_return_type_ref.reset();
  module.functions.push_back(std::move(structured_only));

  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(),
         "valid structured void TypeSpec should reconcile with or without a mirror");
  const auto functions = imported.value().view().functions();
  for (const auto function_id : functions) {
    const auto function = imported.value().view().function(function_id);
    const auto type = function.value().signature().return_type;
    expect(type.kind == bir::TypeKind::Void && type.structured_spec.has_value(),
           "Raw signature view must expose structured TypeSpec authority");
    const auto& facts = *type.structured_spec;
    expect(facts.base == bir::StructuredTypeBase::Void &&
               facts.pointer_level == 0 && !facts.is_lvalue_reference &&
               !facts.is_rvalue_reference && facts.array_rank == 0 &&
               !facts.is_pointer_to_array && facts.inner_array_rank == 0 &&
               !facts.is_function_pointer,
           "valid structured void facts must survive without invented shape");
  }
}

void test_typed_lir_type_rejections() {
  const auto expect_value_rejected = [](lir::LirTypeRef type,
                                        const std::string& message) {
    lir::LirModule module;
    auto producer = void_inline_asm("bad type", "=r");
    producer.ordinary_results.push_back(lir::LirInlineAsmValueBinding{
        lir::LirOperand("%bad"), std::move(type),
        lir::LirInlineAsmValueRole::Output, 0});
    auto block = return_block(0, "entry");
    block.insts.push_back(std::move(producer));
    module.functions.push_back(void_definition("bad_type", {std::move(block)}));
    auto imported = bir::lower_lir_to_raw_bir(module);
    expect(!imported.has_value() &&
               imported.error().code ==
                   bir::ImportErrorCode::UnsupportedInlineAsmShape,
           message);
  };
  expect_value_rejected(
      lir::LirTypeRef("not-an-integer", lir::LirTypeKind::Integer),
      "integer receipt must reject a missing typed width");
  expect_value_rejected(
      lir::LirTypeRef("c4c.vrm", lir::LirTypeKind::VrmRegister),
      "VRM receipt must reject a missing typed width");
  expect_value_rejected(lir::LirTypeRef("semantic raw text"),
                        "semantic RawText must be rejected");
  expect_value_rejected(
      lir::LirTypeRef("<4 x i32", lir::LirTypeKind::Vector),
      "malformed vector shape must be rejected");
  expect_value_rejected(
      lir::LirTypeRef("i32 (", lir::LirTypeKind::Function),
      "malformed function shape must be rejected");

  lir::LirModule conflicting_type_spec;
  auto conflicting_void = void_declaration("conflicting_type_spec");
  conflicting_void.return_type.base = c4c::TB_INT;
  conflicting_type_spec.functions.push_back(std::move(conflicting_void));
  auto type_spec_conflict = bir::lower_lir_to_raw_bir(conflicting_type_spec);
  expect(!type_spec_conflict.has_value() &&
             type_spec_conflict.error().code ==
                 bir::ImportErrorCode::UnsupportedReturnType,
         "structured TypeSpec must not be overridden by a void type mirror");

  const auto expect_signature_shape_rejected = [](auto mutate,
                                                   const std::string& message) {
    lir::LirModule module;
    auto declaration = void_declaration("bad_signature_shape");
    mutate(declaration);
    module.functions.push_back(std::move(declaration));
    auto imported = bir::lower_lir_to_raw_bir(module);
    expect(!imported.has_value() &&
               imported.error().code == bir::ImportErrorCode::UnsupportedReturnType,
           message);
  };
  expect_signature_shape_rejected(
      [](lir::LirFunction& function) { function.return_type.ptr_level = 1; },
      "structured pointer shape must conflict with void receipt");
  expect_signature_shape_rejected(
      [](lir::LirFunction& function) {
        function.return_type.is_lvalue_ref = true;
      },
      "structured lvalue-reference shape must conflict with void receipt");
  expect_signature_shape_rejected(
      [](lir::LirFunction& function) {
        function.return_type.is_rvalue_ref = true;
      },
      "structured rvalue-reference shape must conflict with void receipt");
  expect_signature_shape_rejected(
      [](lir::LirFunction& function) { function.return_type.array_rank = 1; },
      "structured array shape must conflict with void receipt");
  expect_signature_shape_rejected(
      [](lir::LirFunction& function) { function.return_type.is_fn_ptr = true; },
      "structured function-pointer shape must conflict with void receipt");
  expect_signature_shape_rejected(
      [](lir::LirFunction& function) {
        function.signature_return_type_ref = lir::LirTypeRef::integer(32);
      },
      "non-void mirror must conflict with structured void TypeSpec");

  lir::LirModule invalid_struct;
  auto texts = std::make_shared<c4c::TextTable>();
  invalid_struct.link_name_texts = texts;
  invalid_struct.struct_names.attach_text_table(texts.get());
  const c4c::StructNameId pair_id =
      invalid_struct.struct_names.intern("%struct.Pair");
  auto bad_struct_producer = void_inline_asm("bad struct", "=r");
  bad_struct_producer.ordinary_results.push_back(lir::LirInlineAsmValueBinding{
      lir::LirOperand("%bad"),
      lir::LirTypeRef::struct_type("%struct.Other", pair_id),
      lir::LirInlineAsmValueRole::Output, 0});
  auto bad_struct_block = return_block(0, "entry");
  bad_struct_block.insts.push_back(std::move(bad_struct_producer));
  invalid_struct.functions.push_back(
      void_definition("bad_struct", {std::move(bad_struct_block)}));
  auto bad_struct = bir::lower_lir_to_raw_bir(invalid_struct);
  expect(!bad_struct.has_value() &&
             bad_struct.error().code ==
                 bir::ImportErrorCode::UnsupportedInlineAsmShape,
         "conflicting StructNameId/spelling identity must be rejected");
}

void test_verifier_rejects_malformed_raw_type() {
  bir::ModuleBuilder builder;
  bir::FunctionSignature malformed;
  malformed.return_type = bir::Type{bir::TypeKind::Void, 0, "void"};
  bir::StructuredTypeSpecFacts malformed_facts;
  malformed_facts.pointer_level = 1;
  malformed.return_type.structured_spec = malformed_facts;
  auto function = builder.create_function(std::move(malformed),
                                          "malformed_raw_type", true);
  expect(function.has_value(),
         "builder should retain malformed staged state for verifier diagnosis");
  auto published = std::move(builder).publish();
  expect(!published.has_value() &&
             published.error().reason == bir::PublishError::VerificationFailed &&
             !published.error().verification.errors.empty(),
         "Raw publication verifier must reject malformed typed state");
}

void test_module_name_and_struct_declaration_receipt() {
  lir::LirModule module;
  auto texts = std::make_shared<c4c::TextTable>();
  module.link_name_texts = texts;
  module.link_names.attach_text_table(texts.get());
  module.struct_names.attach_text_table(texts.get());

  const c4c::LinkNameId exported = module.link_names.intern("exported");
  const c4c::LinkNameId helper = module.link_names.intern("helper");
  const c4c::StructNameId outer = module.struct_names.intern("%struct.Outer");
  const c4c::StructNameId inner = module.struct_names.intern("%struct.Inner");

  lir::LirStructDecl outer_decl;
  outer_decl.name_id = outer;
  outer_decl.is_packed = true;
  outer_decl.fields = {
      {lir::LirTypeRef::struct_type("%struct.Inner", inner)},
      {lir::LirTypeRef("ptr")}};
  module.record_struct_decl(std::move(outer_decl));
  lir::LirStructDecl inner_decl;
  inner_decl.name_id = inner;
  inner_decl.fields = {{lir::LirTypeRef::integer(32)},
                       {lir::LirTypeRef::struct_type("%struct.Outer", outer)}};
  module.record_struct_decl(std::move(inner_decl));
  module.type_decls = {"deliberately non-authoritative legacy shadow"};

  auto declaration = void_declaration("legacy-fallback-must-not-win");
  declaration.link_name_id = exported;
  module.functions.push_back(std::move(declaration));

  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(),
         "structured declarations and semantic name tables should publish");
  const auto view = imported.value().view();
  const auto link_names = view.link_names();
  expect(link_names.size() == 2 &&
             view.spelling(link_names[0]).value() == "exported" &&
             view.spelling(link_names[1]).value() == "helper" &&
             view.source_id(link_names[0]).value() == exported,
         "link-name receipt must preserve source order, spelling, and source ID");
  const auto struct_names = view.struct_names();
  expect(struct_names.size() == 2 &&
             view.spelling(struct_names[0]).value() == "%struct.Outer" &&
             view.spelling(struct_names[1]).value() == "%struct.Inner" &&
             view.source_id(struct_names[1]).value() == inner,
         "struct-name receipt must preserve typed identity and spelling");
  const auto declarations = view.struct_declarations();
  expect(declarations.size() == 2,
         "struct declarations must remain in deterministic source order");
  const auto raw_outer = view.struct_declaration(declarations[0]).value();
  const auto raw_inner = view.struct_declaration(declarations[1]).value();
  expect(raw_outer.name == struct_names[0] && raw_outer.is_packed &&
             raw_outer.fields.size() == 2 &&
             raw_outer.fields[0].type.struct_name_id == inner &&
             raw_outer.fields[0].referenced_name == struct_names[1] &&
             raw_inner.name == struct_names[1] &&
             raw_inner.fields[1].type.struct_name_id == outer &&
             raw_inner.fields[1].referenced_name == struct_names[0],
         "shared and forward struct field types must retain structured references");
  expect(view.function(view.functions().front()).value().link_name() == "exported",
         "function identity must resolve from the semantic link-name table");
}

void test_module_name_and_struct_declaration_rejections() {
  const auto make_module = [] {
    lir::LirModule module;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    module.struct_names.attach_text_table(module.link_name_texts.get());
    return module;
  };
  const auto rejected = [](lir::LirModule module, const std::string& message) {
    const auto imported = bir::lower_lir_to_raw_bir(module);
    expect(!imported.has_value() &&
               imported.error().code ==
                   bir::ImportErrorCode::UnsupportedTypeDeclarations,
           message);
  };

  auto missing_name = make_module();
  missing_name.struct_decls.push_back(lir::LirStructDecl{});
  missing_name.struct_decl_index.emplace(c4c::kInvalidStructName, 0);
  rejected(std::move(missing_name),
           "invalid declaration IDs must fail transactionally");

  auto duplicate = make_module();
  const auto duplicate_id = duplicate.struct_names.intern("%struct.Duplicate");
  duplicate.struct_decls = {{duplicate_id}, {duplicate_id}};
  duplicate.struct_decl_index.emplace(duplicate_id, 0);
  rejected(std::move(duplicate),
           "duplicate/conflicting declarations must fail transactionally");

  auto malformed = make_module();
  const auto malformed_id = malformed.struct_names.intern("%struct.Malformed");
  lir::LirStructDecl malformed_decl;
  malformed_decl.name_id = malformed_id;
  malformed_decl.fields = {{lir::LirTypeRef("semantic raw field")}};
  malformed.record_struct_decl(std::move(malformed_decl));
  rejected(std::move(malformed),
           "malformed structured field types must fail transactionally");

  auto bad_index = make_module();
  const auto indexed_id = bad_index.struct_names.intern("%struct.Indexed");
  bad_index.record_struct_decl(lir::LirStructDecl{indexed_id});
  bad_index.struct_decl_index[indexed_id] = 1;
  rejected(std::move(bad_index),
           "struct declaration cache mismatches must fail transactionally");

  auto bad_name_cache = make_module();
  bad_name_cache.struct_names.intern("%struct.Cache");
  bad_name_cache.struct_names.ids_.id_by_key_.clear();
  rejected(std::move(bad_name_cache),
           "semantic name-table cache mismatches must fail transactionally");

  bir::ModuleBuilder builder;
  expect(builder.add_struct_name(1, "%struct.BadRaw").has_value(),
         "builder should own staged struct-name state");
  expect(builder.add_struct_declaration(
                    1, {bir::StructField{bir::Type{bir::TypeKind::Void}}},
                    false, false)
             .has_value(),
         "builder should retain malformed staged declarations for diagnosis");
  const auto published = std::move(builder).publish();
  expect(!published.has_value() &&
             published.error().reason == bir::PublishError::VerificationFailed,
         "publication verifier must reject malformed staged declarations");
}

void test_structured_rejection() {
  lir::LirModule module;
  module.globals.push_back(lir::LirGlobal{});

  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(!imported.has_value(), "unsupported globals must not produce RawBir");
  expect(imported.error().code == bir::ImportErrorCode::UnsupportedGlobals,
         "unsupported globals should return their structured import error code");
  expect(!imported.error().detail.empty(),
         "structured import rejection should include diagnostic detail");
}

void test_inline_asm_shape_rejection() {
  lir::LirModule module;
  auto block = return_block(0, "entry");
  auto unsupported = void_inline_asm("opaque", "=r,r");
  unsupported.args_str = "i64 %arg";
  block.insts.push_back(std::move(unsupported));
  module.functions.push_back(void_definition("bad_asm", {std::move(block)}));

  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(!imported.has_value(),
         "unstructured inline-asm arguments must publish no RawBir");
  expect(imported.error().code == bir::ImportErrorCode::UnsupportedInlineAsmShape,
         "unstructured argument rejection should remain structured");
  auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(!canonical.has_value() &&
             canonical.error().code ==
                 bir::ImportErrorCode::UnsupportedInlineAsmShape,
         "textual-only inline asm must publish no partial CanonicalBir");
}

}  // namespace

int main() {
  test_supported_import_and_views();
  test_generic_inline_asm_ssa_edges();
  test_structured_lir_import_ssa_chain();
  test_structured_lir_import_rejections();
  test_lir_inline_asm_structured_value_contract();
  test_closed_typed_lir_type_receipt();
  test_structured_type_spec_signature_receipt();
  test_typed_lir_type_rejections();
  test_verifier_rejects_malformed_raw_type();
  test_module_name_and_struct_declaration_receipt();
  test_module_name_and_struct_declaration_rejections();
  test_structured_rejection();
  test_inline_asm_shape_rejection();
  return 0;
}
