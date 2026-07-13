#include "src/backend/bir/bir.hpp"
#include "src/codegen/lir/ir.hpp"

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <limits>
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

c4c::TypeSpec scalar_type(c4c::TypeBase base) {
  c4c::TypeSpec type{};
  type.base = base;
  return type;
}

double double_from_bits(std::uint64_t bits) {
  double value = 0.0;
  static_assert(sizeof(value) == sizeof(bits));
  std::memcpy(&value, &bits, sizeof(value));
  return value;
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

  bir::ModuleBuilder array_builder;
  bir::Type malformed_array{bir::TypeKind::Array, 0, "[5 x i16]"};
  malformed_array.array_facts =
      bir::ArrayTypeFacts{bir::TypeKind::Integer, 16, 0, {5, 2}};
  expect(array_builder
             .add_global_object("malformed_array", std::move(malformed_array),
                                4, false, false, false, true)
             .has_value(),
         "builder should retain incoherent typed array facts for verifier diagnosis");
  auto rejected_array = std::move(array_builder).publish();
  expect(!rejected_array.has_value() &&
             rejected_array.error().reason ==
                 bir::PublishError::VerificationFailed &&
             !rejected_array.error().verification.errors.empty(),
         "Raw publication verifier must reject array dimensions that disagree with typed spelling");

  bir::ModuleBuilder empty_array_builder;
  bir::Type empty_array{bir::TypeKind::Array, 0, "[5 x i16]"};
  empty_array.array_facts =
      bir::ArrayTypeFacts{bir::TypeKind::Integer, 16, 0, {}};
  expect(empty_array_builder
             .add_global_object("empty_array", std::move(empty_array), 4,
                                false, false, false, true)
             .has_value(),
         "builder should retain empty typed array dimensions for verifier diagnosis");
  auto rejected_empty_array = std::move(empty_array_builder).publish();
  expect(!rejected_empty_array.has_value() &&
             rejected_empty_array.error().reason ==
                 bir::PublishError::VerificationFailed &&
             !rejected_empty_array.error().verification.errors.empty(),
         "Raw publication verifier must reject empty typed array dimensions");

  bir::ModuleBuilder zero_array_builder;
  bir::Type zero_array{bir::TypeKind::Array, 0, "[0 x i16]"};
  zero_array.array_facts =
      bir::ArrayTypeFacts{bir::TypeKind::Integer, 16, 0, {0}};
  expect(zero_array_builder
             .add_global_object("zero_array", std::move(zero_array), 4,
                                false, false, false, true)
             .has_value(),
         "builder should retain producer-valid zero-length array facts");
  const auto accepted_zero_array = std::move(zero_array_builder).publish();
  expect(accepted_zero_array.has_value(),
         "Raw publication verifier must accept exact zero-length array facts");

  const auto malformed_array_facts_reject = [](std::string name,
                                                bir::Type type) {
    bir::ModuleBuilder malformed_builder;
    if (!malformed_builder
             .add_global_object(std::move(name), std::move(type), 4, false,
                                false, false, true)
             .has_value())
      return false;
    const auto published = std::move(malformed_builder).publish();
    return !published.has_value() &&
           published.error().reason ==
               bir::PublishError::VerificationFailed &&
           !published.error().verification.errors.empty();
  };

  bir::Type negative_pointer_depth{bir::TypeKind::Array, 0, "[5 x ptr]"};
  negative_pointer_depth.array_facts =
      bir::ArrayTypeFacts{bir::TypeKind::Integer, 16, -1, {5}};
  expect(malformed_array_facts_reject("negative_pointer_depth",
                                      std::move(negative_pointer_depth)),
         "Raw publication verifier must reject negative array element pointer depth");

  bir::Type negative_outer_dimension{bir::TypeKind::Array, 0, "[-1 x i16]"};
  negative_outer_dimension.array_facts =
      bir::ArrayTypeFacts{bir::TypeKind::Integer, 16, 0, {-1}};
  expect(malformed_array_facts_reject("negative_outer_dimension",
                                      std::move(negative_outer_dimension)),
         "Raw publication verifier must reject negative outer array dimensions");

  bir::Type negative_inner_dimension{bir::TypeKind::Array, 0,
                                     "[5 x [-2 x i16]]"};
  negative_inner_dimension.array_facts =
      bir::ArrayTypeFacts{bir::TypeKind::Integer, 16, 0, {5, -2}};
  expect(malformed_array_facts_reject("negative_inner_dimension",
                                      std::move(negative_inner_dimension)),
         "Raw publication verifier must reject negative inner array dimensions");

  bir::Type nonscalar_pointer_base{bir::TypeKind::Array, 0, "[5 x ptr]"};
  nonscalar_pointer_base.array_facts =
      bir::ArrayTypeFacts{bir::TypeKind::Struct, 0, 1, {5}};
  expect(malformed_array_facts_reject("nonscalar_pointer_base",
                                      std::move(nonscalar_pointer_base)),
         "Raw publication verifier must reject nonscalar pointer-element base facts");

  bir::Type pointer_spelling_conflict{bir::TypeKind::Array, 0, "[5 x i16]"};
  pointer_spelling_conflict.array_facts =
      bir::ArrayTypeFacts{bir::TypeKind::Integer, 16, 1, {5}};
  expect(malformed_array_facts_reject("pointer_spelling_conflict",
                                      std::move(pointer_spelling_conflict)),
         "Raw publication verifier must reject pointer depth that disagrees with array spelling");

  bir::Type facts_on_scalar{bir::TypeKind::Integer, 16, "i16"};
  facts_on_scalar.array_facts =
      bir::ArrayTypeFacts{bir::TypeKind::Integer, 16, 0, {5}};
  expect(malformed_array_facts_reject("facts_on_scalar",
                                      std::move(facts_on_scalar)),
         "Raw publication verifier must reject typed array facts on non-array types");

  const auto malformed_pointer_facts_reject = [](std::string name,
                                                  bir::Type type) {
    bir::ModuleBuilder malformed_builder;
    if (!malformed_builder
             .add_global_object(std::move(name), std::move(type), 8, false,
                                false, false, true)
             .has_value())
      return false;
    const auto published = std::move(malformed_builder).publish();
    return !published.has_value() &&
           published.error().reason ==
               bir::PublishError::VerificationFailed &&
           !published.error().verification.errors.empty();
  };

  bir::Type pointer_facts_on_scalar{bir::TypeKind::Integer, 32, "i32"};
  pointer_facts_on_scalar.pointer_facts =
      bir::PointerTypeFacts{bir::TypeKind::Integer, 32, 1};
  expect(malformed_pointer_facts_reject("pointer_facts_on_scalar",
                                        std::move(pointer_facts_on_scalar)),
         "Raw publication verifier must reject pointer facts on non-pointer types");

  bir::Type zero_pointer_depth{bir::TypeKind::Pointer};
  zero_pointer_depth.pointer_facts =
      bir::PointerTypeFacts{bir::TypeKind::Integer, 32, 0};
  expect(malformed_pointer_facts_reject("zero_pointer_depth",
                                        std::move(zero_pointer_depth)),
         "Raw publication verifier must reject zero typed pointer depth");

  bir::Type negative_direct_pointer_depth{bir::TypeKind::Pointer};
  negative_direct_pointer_depth.pointer_facts =
      bir::PointerTypeFacts{bir::TypeKind::Floating, 64, -1};
  expect(malformed_pointer_facts_reject(
             "negative_direct_pointer_depth",
             std::move(negative_direct_pointer_depth)),
         "Raw publication verifier must reject negative typed pointer depth");

  bir::Type invalid_pointer_base{bir::TypeKind::Pointer};
  invalid_pointer_base.pointer_facts =
      bir::PointerTypeFacts{bir::TypeKind::Struct, 0, 1};
  expect(malformed_pointer_facts_reject("invalid_pointer_base",
                                        std::move(invalid_pointer_base)),
         "Raw publication verifier must reject nonscalar pointer pointee facts");

  bir::Type invalid_pointer_width{bir::TypeKind::Pointer};
  invalid_pointer_width.pointer_facts =
      bir::PointerTypeFacts{bir::TypeKind::Floating, 24, 1};
  expect(malformed_pointer_facts_reject("invalid_pointer_width",
                                        std::move(invalid_pointer_width)),
         "Raw publication verifier must reject invalid floating pointee widths");

  bir::Type invalid_pointer_storage_width{bir::TypeKind::Pointer};
  invalid_pointer_storage_width.bit_width = 64;
  invalid_pointer_storage_width.pointer_facts =
      bir::PointerTypeFacts{bir::TypeKind::Integer, 64, 1};
  expect(malformed_pointer_facts_reject(
             "invalid_pointer_storage_width",
             std::move(invalid_pointer_storage_width)),
         "Raw publication verifier must reject storage width on typed opaque pointers");

  bir::Type facts_on_noncomplex{bir::TypeKind::Integer, 32, "i32"};
  facts_on_noncomplex.complex_facts =
      bir::ComplexTypeFacts{bir::TypeKind::Integer, 32};
  expect(malformed_pointer_facts_reject("facts_on_noncomplex",
                                        std::move(facts_on_noncomplex)),
         "Raw publication verifier must reject complex facts on non-complex types");

  bir::Type missing_complex_facts{bir::TypeKind::Complex, 32,
                                  "{ i32, i32 }"};
  expect(malformed_pointer_facts_reject("missing_complex_facts",
                                        std::move(missing_complex_facts)),
         "Raw publication verifier must require typed complex component facts");

  bir::Type invalid_complex_kind{bir::TypeKind::Complex, 32,
                                 "{ i32, i32 }"};
  invalid_complex_kind.complex_facts =
      bir::ComplexTypeFacts{bir::TypeKind::Struct, 32};
  expect(malformed_pointer_facts_reject("invalid_complex_kind",
                                        std::move(invalid_complex_kind)),
         "Raw publication verifier must reject non-scalar complex component kinds");

  bir::Type invalid_complex_width{bir::TypeKind::Complex, 24,
                                  "{ fp24, fp24 }"};
  invalid_complex_width.complex_facts =
      bir::ComplexTypeFacts{bir::TypeKind::Floating, 24};
  expect(malformed_pointer_facts_reject("invalid_complex_width",
                                        std::move(invalid_complex_width)),
         "Raw publication verifier must reject invalid complex component widths");

  bir::Type complex_spelling_conflict{bir::TypeKind::Complex, 32,
                                      "{ double, double }"};
  complex_spelling_conflict.complex_facts =
      bir::ComplexTypeFacts{bir::TypeKind::Floating, 32};
  expect(malformed_pointer_facts_reject("complex_spelling_conflict",
                                        std::move(complex_spelling_conflict)),
         "Raw publication verifier must reject complex spelling conflicts");

  bir::Type malformed_complex_pointer{bir::TypeKind::Pointer};
  malformed_complex_pointer.pointer_facts = bir::PointerTypeFacts{
      bir::TypeKind::Complex, 64, 2,
      bir::ComplexTypeFacts{bir::TypeKind::Floating, 32}};
  expect(malformed_pointer_facts_reject("malformed_complex_pointer",
                                        std::move(malformed_complex_pointer)),
         "Raw publication verifier must reject mismatched complex pointer facts");

  const auto pointer_array_facts = [](std::vector<std::int64_t> dimensions,
                                      int inner_rank) {
    return bir::PointerTypeFacts{
        bir::TypeKind::Integer, 32, 1, std::nullopt,
        bir::PointerArrayTypeFacts{std::move(dimensions), inner_rank}};
  };
  bir::Type empty_pointer_array{bir::TypeKind::Pointer};
  empty_pointer_array.pointer_facts = pointer_array_facts({}, -1);
  expect(malformed_pointer_facts_reject("empty_pointer_array",
                                        std::move(empty_pointer_array)),
         "Raw publication verifier must reject empty pointer-array dimensions");

  bir::Type negative_pointer_array{bir::TypeKind::Pointer};
  negative_pointer_array.pointer_facts = pointer_array_facts({3, -1}, -1);
  expect(malformed_pointer_facts_reject("negative_pointer_array",
                                        std::move(negative_pointer_array)),
         "Raw publication verifier must reject negative pointer-array dimensions");

  bir::Type split_pointer_array{bir::TypeKind::Pointer};
  split_pointer_array.pointer_facts = pointer_array_facts({3, 5}, 1);
  expect(malformed_pointer_facts_reject("split_pointer_array",
                                        std::move(split_pointer_array)),
         "Raw publication verifier must reject pointer-array facts with an outer array rank");

  bir::Type untyped_pointer_array{bir::TypeKind::Pointer};
  untyped_pointer_array.pointer_facts = bir::PointerTypeFacts{
      bir::TypeKind::Void, 0, 1, std::nullopt,
      bir::PointerArrayTypeFacts{{3}, -1}};
  expect(malformed_pointer_facts_reject("untyped_pointer_array",
                                        std::move(untyped_pointer_array)),
         "Raw publication verifier must reject pointer-array facts without typed pointee authority");

  bir::Type pointer_array_facts_on_scalar{bir::TypeKind::Integer, 32, "i32"};
  pointer_array_facts_on_scalar.pointer_facts = pointer_array_facts({3}, 1);
  expect(malformed_pointer_facts_reject(
             "pointer_array_facts_on_scalar",
             std::move(pointer_array_facts_on_scalar)),
         "Raw publication verifier must reject pointer-array facts on non-pointer types");

  bir::Type oversized_pointer_array{bir::TypeKind::Pointer};
  oversized_pointer_array.pointer_facts =
      pointer_array_facts({1, 2, 3, 4, 5, 6, 7, 8, 9}, 9);
  expect(malformed_pointer_facts_reject("oversized_pointer_array",
                                        std::move(oversized_pointer_array)),
         "Raw publication verifier must bound typed pointer-array dimensions");

  bir::Type incompatible_complex_pointer_array{bir::TypeKind::Pointer};
  incompatible_complex_pointer_array.pointer_facts = bir::PointerTypeFacts{
      bir::TypeKind::Complex, 64, 1,
      bir::ComplexTypeFacts{bir::TypeKind::Floating, 32},
      bir::PointerArrayTypeFacts{{3}, 1}};
  expect(malformed_pointer_facts_reject(
             "incompatible_complex_pointer_array",
             std::move(incompatible_complex_pointer_array)),
         "Raw publication verifier must reject incompatible nested complex pointer-array facts");

  bir::Type malformed_complex_array{bir::TypeKind::Array, 0,
                                    "[2 x { i32, i32 }]"};
  malformed_complex_array.array_facts = bir::ArrayTypeFacts{
      bir::TypeKind::Complex, 32, 0, {2}, std::nullopt};
  expect(malformed_pointer_facts_reject("malformed_complex_array",
                                        std::move(malformed_complex_array)),
         "Raw publication verifier must reject missing complex array element facts");

  bir::Type invalid_vrm_width{bir::TypeKind::VrmRegister, 3, "c4c.vrm3"};
  expect(malformed_pointer_facts_reject("invalid_vrm_width",
                                        std::move(invalid_vrm_width)),
         "Raw publication verifier must reject unsupported direct VRM widths");

  bir::Type invalid_vrm_spelling{bir::TypeKind::VrmRegister, 4, "c4c.vrm2"};
  expect(malformed_pointer_facts_reject("invalid_vrm_spelling",
                                        std::move(invalid_vrm_spelling)),
         "Raw publication verifier must reject direct VRM spelling conflicts");

  bir::Type invalid_vrm_pointer_width{bir::TypeKind::Pointer};
  invalid_vrm_pointer_width.pointer_facts =
      bir::PointerTypeFacts{bir::TypeKind::VrmRegister, 16, 2};
  expect(malformed_pointer_facts_reject(
             "invalid_vrm_pointer_width",
             std::move(invalid_vrm_pointer_width)),
         "Raw publication verifier must reject unsupported VRM pointer widths");

  bir::Type nested_vrm_pointer_facts{bir::TypeKind::Pointer};
  nested_vrm_pointer_facts.pointer_facts = bir::PointerTypeFacts{
      bir::TypeKind::VrmRegister, 4, 2,
      bir::ComplexTypeFacts{bir::TypeKind::Integer, 4}};
  expect(malformed_pointer_facts_reject(
             "nested_vrm_pointer_facts",
             std::move(nested_vrm_pointer_facts)),
         "Raw publication verifier must reject complex facts nested in VRM pointers");

  bir::Type invalid_vrm_array_width{bir::TypeKind::Array, 0,
                                    "[2 x c4c.vrm16]"};
  invalid_vrm_array_width.array_facts =
      bir::ArrayTypeFacts{bir::TypeKind::VrmRegister, 16, 0, {2}};
  expect(malformed_array_facts_reject("invalid_vrm_array_width",
                                      std::move(invalid_vrm_array_width)),
         "Raw publication verifier must reject unsupported VRM array widths");

  bir::Type nested_vrm_array_facts{bir::TypeKind::Array, 0,
                                   "[2 x c4c.vrm4]"};
  nested_vrm_array_facts.array_facts = bir::ArrayTypeFacts{
      bir::TypeKind::VrmRegister, 4, 0, {2},
      bir::ComplexTypeFacts{bir::TypeKind::Integer, 4}};
  expect(malformed_array_facts_reject("nested_vrm_array_facts",
                                      std::move(nested_vrm_array_facts)),
         "Raw publication verifier must reject complex facts nested in VRM arrays");

  const auto malformed_vector_facts_reject = [](std::string name,
                                                 bir::Type type) {
    bir::ModuleBuilder malformed_builder;
    if (!malformed_builder
             .add_global_object(std::move(name), std::move(type), 16, false,
                                false, false, true)
             .has_value())
      return false;
    const auto published = std::move(malformed_builder).publish();
    return !published.has_value() &&
           published.error().reason ==
               bir::PublishError::VerificationFailed &&
           !published.error().verification.errors.empty();
  };

  bir::Type vector_facts_on_scalar{bir::TypeKind::Integer, 32, "i32"};
  vector_facts_on_scalar.vector_facts =
      bir::VectorTypeFacts{bir::TypeKind::Integer, 32, 4, 16};
  expect(malformed_vector_facts_reject("vector_facts_on_scalar",
                                       std::move(vector_facts_on_scalar)),
         "Raw publication verifier must reject vector facts on non-vector types");

  bir::Type zero_vector_lanes{bir::TypeKind::Vector, 0, "<4 x i32>"};
  zero_vector_lanes.vector_facts =
      bir::VectorTypeFacts{bir::TypeKind::Integer, 32, 0, 16};
  expect(malformed_vector_facts_reject("zero_vector_lanes",
                                       std::move(zero_vector_lanes)),
         "Raw publication verifier must reject nonpositive vector lane facts");

  bir::Type negative_vector_storage{bir::TypeKind::Vector, 0, "<4 x i32>"};
  negative_vector_storage.vector_facts =
      bir::VectorTypeFacts{bir::TypeKind::Integer, 32, 4, -16};
  expect(malformed_vector_facts_reject("negative_vector_storage",
                                       std::move(negative_vector_storage)),
         "Raw publication verifier must reject nonpositive vector storage facts");

  bir::Type nonscalar_vector_base{bir::TypeKind::Vector, 0,
                                  "<4 x %struct.Payload>"};
  nonscalar_vector_base.vector_facts =
      bir::VectorTypeFacts{bir::TypeKind::Struct, 0, 4, 16};
  expect(malformed_vector_facts_reject("nonscalar_vector_base",
                                       std::move(nonscalar_vector_base)),
         "Raw publication verifier must reject nonscalar vector element facts");

  bir::Type invalid_vector_width{bir::TypeKind::Vector, 0, "<4 x float>"};
  invalid_vector_width.vector_facts =
      bir::VectorTypeFacts{bir::TypeKind::Floating, 24, 4, 16};
  expect(malformed_vector_facts_reject("invalid_vector_width",
                                       std::move(invalid_vector_width)),
         "Raw publication verifier must reject invalid floating vector widths");

  bir::Type vector_spelling_conflict{bir::TypeKind::Vector, 0,
                                     "<8 x i32>"};
  vector_spelling_conflict.vector_facts =
      bir::VectorTypeFacts{bir::TypeKind::Integer, 32, 4, 16};
  expect(malformed_vector_facts_reject("vector_spelling_conflict",
                                       std::move(vector_spelling_conflict)),
         "Raw publication verifier must reject vector spelling conflicts");
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

void test_structured_constant_value_receipt() {
  constexpr std::uint64_t negative_zero = UINT64_C(0x8000000000000000);
  constexpr std::uint64_t quiet_nan = UINT64_C(0x7ff8000000000042);

  lir::LirModule module;
  auto block = return_block(0, "entry");
  block.insts = {
      lir::LirConstInt{lir::LirValueId{0}, scalar_type(c4c::TB_LONGLONG),
                       std::numeric_limits<long long>::min()},
      lir::LirConstInt{lir::LirValueId{17}, scalar_type(c4c::TB_LONGLONG),
                       std::numeric_limits<long long>::max()},
      lir::LirConstFloat{lir::LirValueId{8}, scalar_type(c4c::TB_DOUBLE),
                         double_from_bits(negative_zero)},
      lir::LirConstFloat{lir::LirValueId{9}, scalar_type(c4c::TB_DOUBLE),
                         double_from_bits(quiet_nan)},
  };
  module.functions.push_back(
      void_definition("structured_constants", {std::move(block)}));

  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(),
         "typed integer and floating constants should publish RawBir");
  expect(bir::FoundationVerifier::verify(imported.value()).ok(),
         "published constant state should be reachable by FoundationVerifier");
  const auto module_view = imported.value().view();
  const auto function_id = module_view.functions().front();
  const auto function = module_view.function(function_id).value();
  const auto values = function.values();
  expect(values.size() == 4,
         "constants must be ordinary values, not fabricated instructions");
  expect(function.instructions(function.blocks().front()).value().empty(),
         "constant definitions must not create runtime instruction nodes");

  const auto min_id =
      function.source_value(bir::SourceValueId{function_id, 0}).value();
  const auto max_id =
      function.source_value(bir::SourceValueId{function_id, 17}).value();
  const auto neg_zero_id =
      function.source_value(bir::SourceValueId{function_id, 8}).value();
  const auto nan_id =
      function.source_value(bir::SourceValueId{function_id, 9}).value();
  const auto min_def = function.value(min_id).value();
  const auto max_def = function.value(max_id).value();
  const auto neg_zero_def = function.value(neg_zero_id).value();
  const auto nan_def = function.value(nan_id).value();
  const auto min_constant = module_view
                                .constant(std::get<bir::ConstantDef>(
                                              min_def.definition)
                                              .constant)
                                .value();
  const auto max_constant = module_view
                                .constant(std::get<bir::ConstantDef>(
                                              max_def.definition)
                                              .constant)
                                .value();
  const auto neg_zero_constant =
      module_view
          .constant(std::get<bir::ConstantDef>(neg_zero_def.definition).constant)
          .value();
  const auto nan_constant =
      module_view.constant(std::get<bir::ConstantDef>(nan_def.definition).constant)
          .value();
  expect(std::get<bir::IntegerConstant>(min_constant.payload).value ==
             std::numeric_limits<std::int64_t>::min() &&
             std::get<bir::IntegerConstant>(max_constant.payload).value ==
                 std::numeric_limits<std::int64_t>::max(),
         "integer constants must preserve signed boundary values exactly");
  expect(std::get<bir::FloatingConstant>(neg_zero_constant.payload).bits ==
             negative_zero &&
             std::get<bir::FloatingConstant>(nan_constant.payload).bits == quiet_nan,
         "floating constants must preserve negative-zero and NaN payload bits");
  expect(min_def.type == bir::Type{bir::TypeKind::I64} &&
             neg_zero_def.type == bir::Type{bir::TypeKind::F64},
         "constant immutable views must expose their exact typed definitions");
}

void test_constant_value_rejections_and_forward_use() {
  const auto rejected = [](lir::LirModule module, const std::string& message) {
    auto imported = bir::lower_lir_to_raw_bir(module);
    expect(!imported.has_value() &&
               imported.error().code ==
                   bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           message);
  };
  const auto module_with = [](std::string name,
                              std::vector<lir::LirInst> instructions) {
    lir::LirModule module;
    module.functions.push_back(void_declaration("valid_before_" + name));
    auto block = return_block(0, "entry");
    block.insts = std::move(instructions);
    module.functions.push_back(
        void_definition(std::move(name), {std::move(block)}));
    return module;
  };

  rejected(module_with(
               "duplicate_constant",
               {lir::LirConstInt{lir::LirValueId{3}, scalar_type(c4c::TB_INT), 1},
                lir::LirConstFloat{lir::LirValueId{3}, scalar_type(c4c::TB_DOUBLE),
                                   1.0}}),
           "duplicate LirValueId definitions must roll back the whole module");
  rejected(module_with(
               "missing_constant",
               {lir::LirConstInt{lir::LirValueId::invalid(),
                                 scalar_type(c4c::TB_INT), 1}}),
           "invalid source result identities must be rejected");
  rejected(module_with(
               "mistyped_constant",
               {lir::LirConstInt{lir::LirValueId{1}, scalar_type(c4c::TB_DOUBLE), 1}}),
           "integer payloads with floating types must be rejected");

  bir::ModuleBuilder builder;
  bir::FunctionSignature signature;
  signature.return_type = bir::Type{bir::TypeKind::Void};
  auto function = builder.create_function(signature, "forward_constant", false);
  expect(function.has_value(), "forward-use function should be constructible");
  bir::ValueId reserved{};
  bir::InstId use{};
  auto edited = builder.with_function(
      function.value(), [&](bir::FunctionBuilder& function_builder) {
        auto block = function_builder.create_block("entry");
        if (!block) return bir::Result<void, bir::BuildError>::failure(block.error());
        auto value = function_builder.reserve_source_value(
            41, bir::Type{bir::TypeKind::I64});
        if (!value) return bir::Result<void, bir::BuildError>::failure(value.error());
        reserved = value.value();

        bir::InlineAsmSpec spec;
        spec.asm_text = "forward use";
        spec.constraint_text = "r";
        spec.inputs = {reserved};
        auto appended = function_builder.append(block.value(), std::move(spec));
        if (!appended)
          return bir::Result<void, bir::BuildError>::failure(appended.error());
        use = appended.value().instruction;
        auto defined = function_builder.define_int_constant(reserved, -7);
        if (!defined) return defined;
        expect(!function_builder.define_int_constant(reserved, -8).has_value(),
               "conflicting second definitions must be rejected");
        return function_builder.set_terminator(block.value(), bir::ReturnTerm{});
      });
  expect(edited.has_value(),
         "a reserved identity should permit a use before its constant definition");
  auto published = std::move(builder).publish();
  expect(published.has_value(),
         "a resolved forward use should pass foundation verification");
  const auto published_view = published.value().view();
  const auto view = published_view.function(function.value()).value();
  const auto reserved_definition = view.value(reserved).value();
  const auto reserved_constant =
      published_view
          .constant(std::get<bir::ConstantDef>(reserved_definition.definition)
                        .constant)
          .value();
  expect(view.instruction(use).value().operands() ==
             std::vector<bir::ValueId>{reserved} &&
             std::get<bir::IntegerConstant>(reserved_constant.payload).value == -7,
         "forward uses and the later typed definition must share one ValueId");

  bir::ModuleBuilder unresolved_builder;
  auto unresolved_function = unresolved_builder.create_function(
      signature, "unresolved_constant", false);
  auto unresolved_edit = unresolved_builder.with_function(
      unresolved_function.value(), [&](bir::FunctionBuilder& function_builder) {
        auto block = function_builder.create_block("entry");
        if (!block) return bir::Result<void, bir::BuildError>::failure(block.error());
        auto value = function_builder.reserve_value(bir::Type{bir::TypeKind::I64});
        if (!value) return bir::Result<void, bir::BuildError>::failure(value.error());
        expect(!function_builder.define_float_constant_bits(value.value(), 0)
                    .has_value(),
               "mistyped definitions must be rejected without consuming a reservation");
        return function_builder.set_terminator(block.value(), bir::ReturnTerm{});
      });
  expect(unresolved_edit.has_value(), "unresolved staged state should remain inspectable");
  auto unresolved = std::move(unresolved_builder).publish();
  expect(!unresolved.has_value() &&
             unresolved.error().reason == bir::PublishError::VerificationFailed,
         "FoundationVerifier must reject unresolved reservations");

  bir::ModuleBuilder foreign_builder;
  auto owner = foreign_builder.create_function(signature, "owner", false);
  auto foreign = foreign_builder.create_function(signature, "foreign", false);
  bir::ValueId owner_value{};
  expect(foreign_builder
             .with_function(owner.value(), [&](bir::FunctionBuilder& fb) {
               auto block = fb.create_block("entry");
               owner_value = fb.reserve_value(bir::Type{bir::TypeKind::I64}).value();
               fb.define_int_constant(owner_value, 1);
               return fb.set_terminator(block.value(), bir::ReturnTerm{});
             })
             .has_value(),
         "owner value setup should succeed");
  expect(foreign_builder
             .with_function(foreign.value(), [&](bir::FunctionBuilder& fb) {
               expect(fb.define_int_constant(owner_value, 2).error() ==
                          bir::BuildError::ForeignOwner,
                      "foreign definitions must be rejected by exact owner");
               auto block = fb.create_block("entry");
               return fb.set_terminator(block.value(), bir::ReturnTerm{});
             })
             .has_value(),
         "foreign rejection should not poison the surrounding edit");
}

void test_string_pool_receipt_and_views() {
  std::string embedded("A\0B", 3);
  embedded += "\\22\\5C";
  std::string unusual;
  unusual.push_back(static_cast<char>(0xff));
  unusual.push_back('\n');
  unusual += "opaque\\00tail";

  lir::LirModule module;
  module.string_pool = {
      lir::LirStringConst{"@.str0", embedded, 8},
      lir::LirStringConst{"@.str1", unusual, 3},
      lir::LirStringConst{"@.str2", "", 1},
      lir::LirStringConst{"@.str3", "[2 x i32] [i32 1, i32 0]", -1},
  };
  module.str_pool_map.emplace(std::string("source\0one", 10), "@.str0");
  module.str_pool_map.emplace("source-two", "@.str1");
  module.str_pool_map.emplace("", "@.str2");
  module.str_pool_idx = 4;

  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(),
         "ordered string rows with coherent cache evidence should publish");
  expect(bir::FoundationVerifier::verify(imported.value()).ok(),
         "published string data should be reachable by FoundationVerifier");
  const auto view = imported.value().view();
  const auto ids = view.string_data();
  expect(ids.size() == 4 && ids[0].slot == 0 && ids[1].slot == 1 &&
             ids[2].slot == 2 && ids[3].slot == 3,
         "string data IDs must preserve vector authority and order");
  const auto first = view.string_data(ids[0]).value();
  const auto second = view.string_data(ids[1]).value();
  const auto third = view.string_data(ids[2]).value();
  const auto wide = view.string_data(ids[3]).value();
  expect(first.pool_name == "@.str0" && first.raw_bytes == embedded &&
             first.byte_length == 8 && second.pool_name == "@.str1" &&
             second.raw_bytes == unusual && second.byte_length == 3 &&
             third.pool_name == "@.str2" && third.raw_bytes.empty() &&
             third.byte_length == 1 && wide.pool_name == "@.str3" &&
             wide.raw_bytes == "[2 x i32] [i32 1, i32 0]" &&
             wide.byte_length == -1,
         "immutable string views must preserve names, opaque bytes, and lengths exactly");
  expect(view.string_data("@.str1").value() == ids[1],
         "name lookup must resolve to the ordered typed identity");

  lir::LirModule empty;
  auto empty_import = bir::lower_lir_to_raw_bir(empty);
  expect(empty_import.has_value() &&
             empty_import.value().view().string_data().empty(),
         "an empty vector/cache with zero counter should publish an empty view");
}

void test_string_pool_rejections_and_transactionality() {
  const auto valid_module = [] {
    lir::LirModule module;
    module.string_pool = {
        lir::LirStringConst{"@.str0", "opaque\\00", 4},
        lir::LirStringConst{"@.str1", "payload", 8},
    };
    module.str_pool_map.emplace("source-zero", "@.str0");
    module.str_pool_map.emplace("source-one", "@.str1");
    module.str_pool_idx = 2;
    module.functions.push_back(void_declaration("valid_before_bad_pool"));
    return module;
  };
  const auto rejected = [&](auto mutate, const std::string& message) {
    auto module = valid_module();
    mutate(module);
    auto imported = bir::lower_lir_to_raw_bir(module);
    expect(!imported.has_value() &&
               imported.error().code ==
                   bir::ImportErrorCode::UnsupportedStringPool,
           message);
  };

  rejected(
      [](lir::LirModule& module) { module.string_pool[0].pool_name.clear(); },
      "empty string names must reject the whole module");
  rejected(
      [](lir::LirModule& module) {
        module.string_pool[1].pool_name = module.string_pool[0].pool_name;
      },
      "duplicate ordered string names must reject the whole module");
  rejected(
      [](lir::LirModule& module) { module.string_pool[0].byte_length = -2; },
      "lengths below the evidenced sentinel must reject without parsing payloads");
  rejected(
      [](lir::LirModule& module) { module.string_pool[0].byte_length = -1; },
      "cache entries for cacheless sentinel rows must reject the whole module");
  rejected(
      [](lir::LirModule& module) { module.str_pool_map.erase("source-one"); },
      "missing cache entries must reject the whole module");
  rejected(
      [](lir::LirModule& module) {
        module.str_pool_map.emplace("extra-source", "@.str2");
      },
      "extra cache entries must reject the whole module");
  rejected(
      [](lir::LirModule& module) {
        module.str_pool_map["source-one"] = "@.str0";
      },
      "conflicting cache values must reject the whole module");
  rejected(
      [](lir::LirModule& module) {
        module.str_pool_map["source-one"] = "@.missing";
      },
      "cache values missing from ordered rows must reject the whole module");
  rejected(
      [](lir::LirModule& module) { module.str_pool_idx = 1; },
      "a stale string counter must reject the whole module");

  lir::LirModule malformed_empty;
  malformed_empty.str_pool_map.emplace("ghost", "@.str0");
  expect(!bir::lower_lir_to_raw_bir(malformed_empty).has_value(),
         "an empty pool cannot retain cache state");
  malformed_empty.str_pool_map.clear();
  malformed_empty.str_pool_idx = 1;
  expect(!bir::lower_lir_to_raw_bir(malformed_empty).has_value(),
         "an empty pool cannot retain a stale counter");

  bir::ModuleBuilder builder;
  expect(builder.add_string_data("", "payload", 1).error() ==
             bir::BuildError::EmptyStringDataName,
         "builder must reject an empty string-data name");
  expect(builder.add_string_data("@.str0", "opaque", 6).has_value(),
         "builder should stage a valid ordered string row");
  expect(builder.add_string_data("@.str0", "other", 5).error() ==
             bir::BuildError::DuplicateStringDataName,
         "builder must reject duplicate staged names transactionally");
  expect(builder.add_string_data("@.str1", "malformed opaque", -2).has_value(),
         "builder should retain malformed staged length for verifier diagnosis");
  auto published = std::move(builder).publish();
  expect(!published.has_value() &&
             published.error().reason == bir::PublishError::VerificationFailed &&
             !published.error().verification.errors.empty(),
         "FoundationVerifier must reject malformed staged string storage");
}

void test_external_declaration_receipt_and_views() {
  lir::LirModule module;
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());
  const auto void_link = module.link_names.intern("linked_void");
  const auto struct_link = module.link_names.intern("linked_struct");
  const auto payload_name = module.struct_names.intern("%struct.Payload");
  module.record_struct_decl(lir::LirStructDecl{payload_name});

  const auto payload_type =
      lir::LirTypeRef::struct_type("%struct.Payload", payload_name);
  const lir::LirExternDecl fallback_sign{
      "fallback_sign", "i8", lir::LirTypeRef::integer(8),
      lir::LirExtAttr::SignExt, c4c::kInvalidLinkName};
  const lir::LirExternDecl linked_void{
      "linked_void", "void", lir::LirTypeRef("void"),
      lir::LirExtAttr::None, void_link};
  const lir::LirExternDecl linked_struct{
      "linked_struct", "%struct.Payload", payload_type,
      lir::LirExtAttr::None, struct_link};
  const lir::LirExternDecl fallback_zero{
      "fallback_zero", "i32", lir::LirTypeRef::integer(32),
      lir::LirExtAttr::ZeroExt, c4c::kInvalidLinkName};
  module.extern_decls = {fallback_sign, linked_void, linked_struct,
                         fallback_zero};
  module.extern_decl_link_name_map.emplace(
      void_link, lir::LirModule::ExternDeclInfo{
                     linked_void.name, linked_void.return_type_str,
                     linked_void.return_type, linked_void.return_ext_attr,
                     linked_void.link_name_id});
  module.extern_decl_link_name_map.emplace(
      struct_link, lir::LirModule::ExternDeclInfo{
                       linked_struct.name, linked_struct.return_type_str,
                       linked_struct.return_type,
                       linked_struct.return_ext_attr,
                       linked_struct.link_name_id});
  module.extern_decl_name_map.emplace(
      fallback_sign.name,
      lir::LirModule::ExternDeclInfo{
          fallback_sign.name, fallback_sign.return_type_str,
          fallback_sign.return_type, fallback_sign.return_ext_attr,
          fallback_sign.link_name_id});
  module.extern_decl_name_map.emplace(
      fallback_zero.name,
      lir::LirModule::ExternDeclInfo{
          fallback_zero.name, fallback_zero.return_type_str,
          fallback_zero.return_type, fallback_zero.return_ext_attr,
          fallback_zero.link_name_id});

  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(),
         "ordered link-backed and fallback external declarations should publish");
  expect(bir::FoundationVerifier::verify(imported.value()).ok(),
         "published external state should be reachable by FoundationVerifier");
  const auto view = imported.value().view();
  const auto ids = view.external_declarations();
  expect(ids.size() == 4 && ids[0].slot == 0 && ids[1].slot == 1 &&
             ids[2].slot == 2 && ids[3].slot == 3,
         "external declaration IDs must preserve vector authority and order");
  const auto raw_sign = view.external_declaration(ids[0]).value();
  const auto raw_void = view.external_declaration(ids[1]).value();
  const auto raw_struct = view.external_declaration(ids[2]).value();
  const auto raw_zero = view.external_declaration(ids[3]).value();
  expect(raw_sign.source_name == "fallback_sign" &&
             raw_sign.return_type == bir::Type{bir::TypeKind::I8} &&
             raw_sign.return_extension == bir::ReturnExtension::SignExt &&
             std::get<bir::FallbackExternalName>(raw_sign.identity).name ==
                 "fallback_sign" &&
             raw_void.return_type.kind == bir::TypeKind::Void &&
             raw_void.return_extension == bir::ReturnExtension::None &&
             std::holds_alternative<bir::LinkNameId>(raw_void.identity) &&
             raw_struct.return_type.kind == bir::TypeKind::Struct &&
             raw_struct.return_type.struct_name_id == payload_name &&
             std::holds_alternative<bir::LinkNameId>(raw_struct.identity) &&
             raw_zero.return_type == bir::Type{bir::TypeKind::I32} &&
             raw_zero.return_extension == bir::ReturnExtension::ZeroExt,
         "immutable extern views must preserve names, structured types, identities, and attrs");
  expect(view.external_declaration("fallback_zero").value() == ids[3] &&
             view.external_declaration(
                     std::get<bir::LinkNameId>(raw_struct.identity))
                     .value() == ids[2],
         "typed name and link lookups must resolve exact external identities");
}

void test_external_declaration_rejections_and_transactionality() {
  const auto valid_module = [] {
    lir::LirModule module;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    module.struct_names.attach_text_table(module.link_name_texts.get());
    const auto linked = module.link_names.intern("linked_ext");
    const lir::LirExternDecl link_row{
        "linked_ext", "i32", lir::LirTypeRef::integer(32),
        lir::LirExtAttr::None, linked};
    const lir::LirExternDecl fallback_row{
        "fallback_ext", "void", lir::LirTypeRef("void"),
        lir::LirExtAttr::None, c4c::kInvalidLinkName};
    module.extern_decls = {link_row, fallback_row};
    module.extern_decl_link_name_map.emplace(
        linked, lir::LirModule::ExternDeclInfo{
                    link_row.name, link_row.return_type_str,
                    link_row.return_type, link_row.return_ext_attr,
                    link_row.link_name_id});
    module.extern_decl_name_map.emplace(
        fallback_row.name,
        lir::LirModule::ExternDeclInfo{
            fallback_row.name, fallback_row.return_type_str,
            fallback_row.return_type, fallback_row.return_ext_attr,
            fallback_row.link_name_id});
    module.functions.push_back(void_declaration("valid_before_bad_extern"));
    return module;
  };
  const auto rejected = [&](auto mutate, const std::string& message) {
    auto module = valid_module();
    mutate(module);
    auto imported = bir::lower_lir_to_raw_bir(module);
    expect(!imported.has_value() &&
               imported.error().code ==
                   bir::ImportErrorCode::UnsupportedExternDeclarations,
           message);
  };

  rejected(
      [](lir::LirModule& module) {
        module.extern_decl_name_map.erase("fallback_ext");
      },
      "missing external parity evidence must reject the whole module");
  rejected(
      [](lir::LirModule& module) {
        const auto extra = module.link_names.intern("extra_ext");
        module.extern_decl_link_name_map.emplace(
            extra, lir::LirModule::ExternDeclInfo{
                       "extra_ext", "i32", lir::LirTypeRef::integer(32),
                       lir::LirExtAttr::None, extra});
      },
      "extra external map evidence must reject the whole module");
  rejected(
      [](lir::LirModule& module) {
        module.extern_decl_name_map["fallback_ext"].return_type =
            lir::LirTypeRef::integer(64);
      },
      "conflicting external map evidence must reject the whole module");
  rejected(
      [](lir::LirModule& module) {
        module.extern_decls[1].name = "linked_ext";
        auto info = module.extern_decl_name_map.extract("fallback_ext");
        info.key() = "linked_ext";
        info.mapped().name = "linked_ext";
        module.extern_decl_name_map.insert(std::move(info));
      },
      "duplicate semantic external identities must reject the whole module");
  rejected(
      [](lir::LirModule& module) {
        const auto invalid = static_cast<c4c::LinkNameId>(999);
        auto info = module.extern_decl_link_name_map.begin()->second;
        module.extern_decl_link_name_map.clear();
        info.link_name_id = invalid;
        module.extern_decl_link_name_map.emplace(invalid, info);
        module.extern_decls[0].link_name_id = invalid;
      },
      "unresolved external link identities must reject the whole module");
  rejected(
      [](lir::LirModule& module) {
        module.extern_decls[0].name = "wrong_link_spelling";
        module.extern_decl_link_name_map.begin()->second.name =
            "wrong_link_spelling";
      },
      "link/name incoherence must reject the whole module");
  rejected(
      [](lir::LirModule& module) {
        module.extern_decls[1].name.clear();
      },
      "empty fallback names must reject the whole module");
  rejected(
      [](lir::LirModule& module) {
        module.extern_decls[0].return_type =
            lir::LirTypeRef("semantic raw type");
        module.extern_decls[0].return_type_str = "semantic raw type";
        auto& info = module.extern_decl_link_name_map.begin()->second;
        info.return_type = module.extern_decls[0].return_type;
        info.return_type_str = module.extern_decls[0].return_type_str;
      },
      "RawText external return types must reject the whole module");
  rejected(
      [](lir::LirModule& module) {
        module.extern_decls[0].return_type = lir::LirTypeRef(
            "not-an-integer", lir::LirTypeKind::Integer);
        module.extern_decls[0].return_type_str = "not-an-integer";
        auto& info = module.extern_decl_link_name_map.begin()->second;
        info.return_type = module.extern_decls[0].return_type;
        info.return_type_str = module.extern_decls[0].return_type_str;
      },
      "malformed structured external return types must reject the whole module");
  rejected(
      [](lir::LirModule& module) {
        module.extern_decls[1].return_ext_attr = lir::LirExtAttr::SignExt;
        module.extern_decl_name_map["fallback_ext"].return_ext_attr =
            lir::LirExtAttr::SignExt;
      },
      "void external returns cannot carry extension attrs");
  rejected(
      [](lir::LirModule& module) {
        const auto unknown = static_cast<lir::LirExtAttr>(255);
        module.extern_decls[0].return_ext_attr = unknown;
        module.extern_decl_link_name_map.begin()->second.return_ext_attr =
            unknown;
      },
      "unknown external return extension attrs must reject the whole module");
  rejected(
      [](lir::LirModule& module) {
        module.extern_decls[0].return_type_str = "i64";
        module.extern_decl_link_name_map.begin()->second.return_type_str = "i64";
      },
      "compatibility return text must agree with structured authority");

  bir::ModuleBuilder builder;
  expect(builder.add_link_name(1, "linked_ext").has_value(),
         "staged link name should be available to external receipt");
  expect(builder
             .add_external_declaration("", bir::Type{bir::TypeKind::I32},
                                       bir::ReturnExtension::None)
             .error() == bir::BuildError::EmptyExternalName,
         "builder must reject empty fallback identities");
  expect(builder
             .add_external_declaration(
                 "linked_ext", bir::Type{bir::TypeKind::I32},
                 bir::ReturnExtension::None, c4c::kInvalidLinkName)
             .error() == bir::BuildError::InvalidExternalLinkName,
         "builder must reject an explicitly invalid link identity");
  expect(builder
             .add_external_declaration("fallback_bad",
                                       bir::Type{bir::TypeKind::Void},
                                       bir::ReturnExtension::SignExt)
             .has_value(),
         "builder should retain malformed staged external state for diagnosis");
  expect(builder
             .add_external_declaration("fallback_bad",
                                       bir::Type{bir::TypeKind::Void},
                                       bir::ReturnExtension::None)
             .error() == bir::BuildError::DuplicateExternalDeclaration,
         "duplicate builder receipt must fail without a second row");
  auto published = std::move(builder).publish();
  expect(!published.has_value() &&
             published.error().reason == bir::PublishError::VerificationFailed,
         "FoundationVerifier must reject malformed staged external storage");
}

void test_global_object_receipt_and_views() {
  lir::LirModule module;
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());
  const auto linked_name = module.link_names.intern("linked_global");
  const auto weak_linked_name = module.link_names.intern("weak_constant_global");
  const auto pointer_linked_name =
      module.link_names.intern("const_pointer_global");
  const auto init_fn_a = module.link_names.intern("init_fn_a");
  const auto init_fn_b = module.link_names.intern("init_fn_b");

  const auto external_global = [](std::string name, c4c::LinkNameId link,
                                  c4c::TypeBase base,
                                  lir::LirTypeRef type, int align,
                                  bool is_const) {
    lir::LirGlobal global;
    global.id = lir::LirGlobalId{0};
    global.name = std::move(name);
    global.link_name_id = link;
    global.type = scalar_type(base);
    global.is_const = is_const;
    global.linkage_vis = "external ";
    global.qualifier = "global ";
    global.llvm_type = type.str();
    global.llvm_type_ref = std::move(type);
    global.align_bytes = align;
    global.is_extern_decl = true;
    return global;
  };
  module.globals.push_back(external_global(
      "fallback_global", c4c::kInvalidLinkName, c4c::TB_INT,
      lir::LirTypeRef::integer(32), 4, false));
  module.globals.back().linkage_vis = "external hidden ";
  module.globals.push_back(external_global(
      "linked_global", linked_name, c4c::TB_DOUBLE,
      lir::LirTypeRef("double"), 8, true));
  auto initialized = external_global(
      "initialized_global", c4c::kInvalidLinkName, c4c::TB_INT,
      lir::LirTypeRef::integer(32), 4, false);
  initialized.linkage_vis = "protected ";
  initialized.is_extern_decl = false;
  initialized.init_text = std::string{"i32 7\n\0tail", 11};
  initialized.initializer_function_link_name_ids = {
      init_fn_b, init_fn_a, init_fn_b};
  module.globals.push_back(std::move(initialized));
  auto constant_initialized = external_global(
      "constant_initialized_global", c4c::kInvalidLinkName, c4c::TB_DOUBLE,
      lir::LirTypeRef("double"), 8, true);
  constant_initialized.linkage_vis.clear();
  constant_initialized.qualifier = "constant ";
  constant_initialized.is_extern_decl = false;
  constant_initialized.init_text = "double 1.25";
  constant_initialized.initializer_function_link_name_ids = {
      init_fn_a, init_fn_b};
  module.globals.push_back(std::move(constant_initialized));
  auto internal_initialized = external_global(
      "internal_initialized_global", c4c::kInvalidLinkName, c4c::TB_INT,
      lir::LirTypeRef::integer(32), 4, false);
  internal_initialized.is_internal = true;
  internal_initialized.linkage_vis = "internal hidden ";
  internal_initialized.is_extern_decl = false;
  internal_initialized.init_text = "i32 19";
  internal_initialized.initializer_function_link_name_ids = {init_fn_b};
  module.globals.push_back(std::move(internal_initialized));
  auto internal_constant_initialized = external_global(
      "internal_constant_initialized_global", c4c::kInvalidLinkName,
      c4c::TB_DOUBLE, lir::LirTypeRef("double"), 8, true);
  internal_constant_initialized.is_internal = true;
  internal_constant_initialized.linkage_vis = "internal ";
  internal_constant_initialized.qualifier = "constant ";
  internal_constant_initialized.is_extern_decl = false;
  internal_constant_initialized.init_text = "double 2.5";
  internal_constant_initialized.initializer_function_link_name_ids = {
      init_fn_b, init_fn_a};
  module.globals.push_back(std::move(internal_constant_initialized));
  auto weak_initialized = external_global(
      "weak_initialized_global", c4c::kInvalidLinkName, c4c::TB_INT,
      lir::LirTypeRef::integer(32), 4, false);
  weak_initialized.linkage_vis = "weak ";
  weak_initialized.is_extern_decl = false;
  weak_initialized.init_text = "i32 23";
  weak_initialized.initializer_function_link_name_ids = {init_fn_a};
  module.globals.push_back(std::move(weak_initialized));
  auto weak_constant_initialized = external_global(
      "weak_constant_global", weak_linked_name, c4c::TB_DOUBLE,
      lir::LirTypeRef("double"), 8, true);
  weak_constant_initialized.linkage_vis = "weak ";
  weak_constant_initialized.qualifier = "constant ";
  weak_constant_initialized.is_extern_decl = false;
  weak_constant_initialized.init_text = "double 3.5";
  weak_constant_initialized.initializer_function_link_name_ids = {
      init_fn_b, init_fn_a};
  module.globals.push_back(std::move(weak_constant_initialized));
  auto const_pointer_initialized = external_global(
      "const_pointer_global", pointer_linked_name, c4c::TB_INT,
      lir::LirTypeRef("ptr"), 8, true);
  const_pointer_initialized.type.ptr_level = 1;
  const_pointer_initialized.linkage_vis = "protected ";
  const_pointer_initialized.is_extern_decl = false;
  const_pointer_initialized.llvm_type_ref.reset();
  const_pointer_initialized.init_text = std::string{"ptr @target\0tail", 16};
  const_pointer_initialized.initializer_function_link_name_ids = {
      init_fn_a, init_fn_b, init_fn_a};
  module.globals.push_back(std::move(const_pointer_initialized));
  auto weak_external = external_global(
      "weak_external_global", c4c::kInvalidLinkName, c4c::TB_INT,
      lir::LirTypeRef::integer(32), 4, false);
  weak_external.linkage_vis = "extern_weak protected ";
  module.globals.push_back(std::move(weak_external));

  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(),
         "initializer-free external globals should publish transactionally");
  expect(bir::FoundationVerifier::verify(imported.value()).ok(),
         "published global storage must be verifier reachable");
  const auto view = imported.value().view();
  const auto ids = view.global_objects();
  expect(ids.size() == 10 && ids[0].slot == 0 && ids[1].slot == 1 &&
             ids[2].slot == 2 && ids[3].slot == 3 && ids[4].slot == 4 &&
             ids[5].slot == 5 && ids[6].slot == 6 && ids[7].slot == 7 &&
             ids[8].slot == 8 && ids[9].slot == 9,
         "Raw-BIR global IDs must follow source vector order, not LirGlobal.id");
  const auto fallback = view.global_object(ids[0]).value();
  const auto linked = view.global_object(ids[1]).value();
  const auto definition = view.global_object(ids[2]).value();
  const auto constant_definition = view.global_object(ids[3]).value();
  const auto internal_definition = view.global_object(ids[4]).value();
  const auto internal_constant_definition = view.global_object(ids[5]).value();
  const auto weak_definition = view.global_object(ids[6]).value();
  const auto weak_constant_definition = view.global_object(ids[7]).value();
  const auto const_pointer_definition = view.global_object(ids[8]).value();
  const auto weak_external_declaration = view.global_object(ids[9]).value();
  expect(fallback.source_name == "fallback_global" &&
             fallback.object_type == bir::Type{bir::TypeKind::I32} &&
             fallback.alignment == 4 && !fallback.is_internal &&
             !fallback.is_weak && !fallback.is_const &&
             fallback.visibility == bir::SymbolVisibility::Hidden &&
             fallback.is_extern_declaration &&
             std::get<bir::FallbackGlobalName>(fallback.identity).name ==
                 fallback.source_name &&
             linked.source_name == "linked_global" &&
             linked.object_type == bir::Type{bir::TypeKind::F64} &&
             linked.alignment == 8 && linked.is_const &&
             linked.visibility == bir::SymbolVisibility::Default &&
             std::holds_alternative<bir::LinkNameId>(linked.identity) &&
             !fallback.initializer && !linked.initializer &&
             !definition.is_extern_declaration && definition.initializer &&
             !definition.is_weak &&
             definition.visibility == bir::SymbolVisibility::Protected &&
             definition.initializer->opaque_payload ==
                 std::string{"i32 7\n\0tail", 11} &&
             constant_definition.object_type ==
                 bir::Type{bir::TypeKind::F64} &&
             constant_definition.is_const &&
             !constant_definition.is_extern_declaration &&
             constant_definition.initializer &&
             constant_definition.initializer->opaque_payload == "double 1.25",
         "global views must preserve type, identity, alignment, and semantic flags");
  const auto& initializer_links = definition.initializer->function_links;
  expect(initializer_links.size() == 3 &&
             view.spelling(initializer_links[0]).value() == "init_fn_b" &&
             view.spelling(initializer_links[1]).value() == "init_fn_a" &&
             initializer_links[2] == initializer_links[0],
         "initializer links must preserve ordered structured references through the link-name table");
  const auto& constant_initializer_links =
      constant_definition.initializer->function_links;
  expect(constant_initializer_links.size() == 2 &&
             view.spelling(constant_initializer_links[0]).value() ==
                 "init_fn_a" &&
             view.spelling(constant_initializer_links[1]).value() ==
                 "init_fn_b",
         "constant initializer links must preserve ordered structured references");
  expect(internal_definition.object_type == bir::Type{bir::TypeKind::I32} &&
             internal_definition.is_internal && !internal_definition.is_weak &&
             !internal_definition.is_const &&
             internal_definition.visibility == bir::SymbolVisibility::Hidden &&
             !internal_definition.is_extern_declaration &&
             internal_definition.initializer &&
             internal_definition.initializer->opaque_payload == "i32 19" &&
             internal_definition.initializer->function_links.size() == 1 &&
             view.spelling(
                     internal_definition.initializer->function_links[0])
                     .value() == "init_fn_b",
         "internal ordinary definitions must preserve typed flags, payloads, and initializer links");
  expect(internal_constant_definition.object_type ==
                 bir::Type{bir::TypeKind::F64} &&
             internal_constant_definition.is_internal &&
             !internal_constant_definition.is_weak &&
             internal_constant_definition.is_const &&
             !internal_constant_definition.is_extern_declaration &&
             internal_constant_definition.initializer &&
             internal_constant_definition.initializer->opaque_payload ==
                 "double 2.5" &&
             internal_constant_definition.initializer->function_links.size() ==
                 2 &&
             view.spelling(internal_constant_definition.initializer
                               ->function_links[0])
                     .value() == "init_fn_b" &&
             view.spelling(internal_constant_definition.initializer
                               ->function_links[1])
                     .value() == "init_fn_a",
         "internal constant definitions must preserve typed flags, payloads, and ordered initializer links");
  expect(weak_definition.object_type == bir::Type{bir::TypeKind::I32} &&
             !weak_definition.is_internal && weak_definition.is_weak &&
             !weak_definition.is_const &&
             !weak_definition.is_extern_declaration &&
             weak_definition.initializer &&
             weak_definition.initializer->opaque_payload == "i32 23" &&
             weak_definition.initializer->function_links.size() == 1 &&
             view.spelling(weak_definition.initializer->function_links[0])
                     .value() == "init_fn_a",
         "weak ordinary definitions must preserve typed linkage, type, payload, and initializer links");
  expect(weak_constant_definition.object_type ==
                 bir::Type{bir::TypeKind::F64} &&
             !weak_constant_definition.is_internal &&
             weak_constant_definition.is_weak &&
             weak_constant_definition.is_const &&
             !weak_constant_definition.is_extern_declaration &&
             std::holds_alternative<bir::LinkNameId>(
                 weak_constant_definition.identity) &&
             weak_constant_definition.initializer &&
             weak_constant_definition.initializer->opaque_payload ==
                 "double 3.5" &&
             weak_constant_definition.initializer->function_links.size() == 2 &&
             view.spelling(
                     weak_constant_definition.initializer->function_links[0])
                     .value() == "init_fn_b" &&
             view.spelling(
                     weak_constant_definition.initializer->function_links[1])
                     .value() == "init_fn_a",
         "weak constant definitions must preserve typed linkage, identity, type, payload, and ordered initializer links");
  expect(const_pointer_definition.object_type.kind == bir::TypeKind::Pointer &&
             const_pointer_definition.object_type.spelling == "ptr" &&
             const_pointer_definition.object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Integer, 32, 1}} &&
             !const_pointer_definition.is_internal &&
             !const_pointer_definition.is_weak &&
             const_pointer_definition.is_const &&
             const_pointer_definition.visibility ==
                 bir::SymbolVisibility::Protected &&
             !const_pointer_definition.is_extern_declaration &&
             std::holds_alternative<bir::LinkNameId>(
                 const_pointer_definition.identity) &&
             const_pointer_definition.initializer &&
             const_pointer_definition.initializer->opaque_payload ==
                 std::string{"ptr @target\0tail", 16} &&
             const_pointer_definition.initializer->function_links.size() == 3 &&
             view.spelling(
                     const_pointer_definition.initializer->function_links[0])
                     .value() == "init_fn_a" &&
             view.spelling(
                     const_pointer_definition.initializer->function_links[1])
                     .value() == "init_fn_b" &&
             const_pointer_definition.initializer->function_links[2] ==
                 const_pointer_definition.initializer->function_links[0],
         "const-pointer global definitions must preserve exact typed authority, identity, opaque payload, and ordered initializer links");
  expect(weak_external_declaration.source_name == "weak_external_global" &&
             weak_external_declaration.object_type ==
                 bir::Type{bir::TypeKind::I32} &&
             !weak_external_declaration.is_internal &&
             weak_external_declaration.is_weak &&
             weak_external_declaration.visibility ==
                 bir::SymbolVisibility::Protected &&
             !weak_external_declaration.is_const &&
             weak_external_declaration.is_extern_declaration &&
             !weak_external_declaration.initializer,
         "weak external declarations must preserve both external-declaration and weak-linkage facts");
  expect(view.global_object("fallback_global").value() == ids[0] &&
             view.global_object(std::get<bir::LinkNameId>(linked.identity))
                     .value() == ids[1] &&
             view.global_object(
                     std::get<bir::LinkNameId>(weak_constant_definition.identity))
                     .value() == ids[7] &&
             view.global_object(
                     std::get<bir::LinkNameId>(const_pointer_definition.identity))
                     .value() == ids[8],
         "global name and link lookups must resolve ordered typed identities");

  module.globals[8].llvm_type_ref = lir::LirTypeRef("ptr");
  auto corroborated = bir::lower_lir_to_raw_bir(module);
  expect(corroborated.has_value(),
         "an agreeing optional pointer mirror should preserve receipt");
  const auto corroborated_view = corroborated.value().view();
  const auto corroborated_ids = corroborated_view.global_objects();
  expect(corroborated_ids.size() == 10 &&
             corroborated_view.global_object(corroborated_ids[8])
                     .value()
                     .object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Integer, 32, 1}},
         "an optional pointer mirror that agrees exactly must remain corroborating evidence");
}

void test_scalar_global_type_authority_without_mirror() {
  lir::LirModule module;
  module.target_profile.arch = c4c::TargetArch::I686;

  lir::LirGlobal declaration;
  declaration.name = "producer_long_declaration";
  declaration.type = scalar_type(c4c::TB_LONG);
  declaration.linkage_vis = "external hidden ";
  declaration.qualifier = "global ";
  declaration.llvm_type = "i32";
  declaration.align_bytes = 4;
  declaration.is_extern_decl = true;
  module.globals.push_back(std::move(declaration));

  lir::LirGlobal definition;
  definition.name = "producer_double_definition";
  definition.type = scalar_type(c4c::TB_DOUBLE);
  definition.is_const = true;
  definition.linkage_vis = "protected ";
  definition.qualifier = "constant ";
  definition.llvm_type = "double";
  definition.init_text = "double 6.25";
  definition.align_bytes = 8;
  definition.is_extern_decl = false;
  module.globals.push_back(std::move(definition));

  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(),
         "actual producer-shaped scalar globals should not require an optional type mirror");
  const auto view = imported.value().view();
  const auto ids = view.global_objects();
  expect(ids.size() == 2,
         "scalar TypeSpec authority should preserve ordered global receipt");
  const auto raw_declaration = view.global_object(ids[0]).value();
  const auto raw_definition = view.global_object(ids[1]).value();
  expect(raw_declaration.object_type ==
                 bir::Type{bir::TypeKind::Integer, 32, "i32"} &&
             raw_declaration.is_extern_declaration &&
             raw_declaration.visibility == bir::SymbolVisibility::Hidden &&
             !raw_declaration.initializer,
         "target-aware long TypeSpec authority must lower to i32 on I686 declarations");
  expect(raw_definition.object_type ==
                 bir::Type{bir::TypeKind::Floating, 64, "double"} &&
             !raw_definition.is_extern_declaration &&
             raw_definition.is_const &&
             raw_definition.visibility == bir::SymbolVisibility::Protected &&
             raw_definition.initializer &&
             raw_definition.initializer->opaque_payload == "double 6.25",
         "floating TypeSpec authority must retain definition, visibility, and initializer facts");
  expect(bir::FoundationVerifier::verify(imported.value()).ok(),
         "mirror-free scalar global receipt must remain verifier reachable");
}

void test_enum_storage_global_receipt_and_rejections() {
  const auto valid_module = [] {
    lir::LirModule module;
    module.target_profile.arch = c4c::TargetArch::X86_64;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    module.struct_names.attach_text_table(module.link_name_texts.get());
    const auto definition_link =
        module.link_names.intern("default_enum_definition");
    const auto init_link = module.link_names.intern("enum_init_function");

    lir::LirGlobal definition;
    definition.name = "default_enum_definition";
    definition.link_name_id = definition_link;
    definition.type = scalar_type(c4c::TB_ENUM);
    definition.type.enum_underlying_base = c4c::TB_VOID;
    definition.linkage_vis = "protected ";
    definition.qualifier = "global ";
    definition.llvm_type = "i32";
    definition.init_text = std::string{"i32 7\0enum", 10};
    definition.initializer_function_link_name_ids = {init_link};
    definition.align_bytes = 4;
    module.globals.push_back(std::move(definition));

    lir::LirGlobal declaration;
    declaration.name = "explicit_enum_extern";
    declaration.type = scalar_type(c4c::TB_ENUM);
    declaration.type.enum_underlying_base = c4c::TB_LONG;
    declaration.linkage_vis = "external hidden ";
    declaration.qualifier = "global ";
    declaration.llvm_type = "i64";
    declaration.align_bytes = 8;
    declaration.is_extern_decl = true;
    module.globals.push_back(std::move(declaration));

    lir::LirGlobal pointer;
    pointer.name = "deep_enum_pointer";
    pointer.type = scalar_type(c4c::TB_ENUM);
    pointer.type.enum_underlying_base = c4c::TB_UINT128;
    pointer.type.ptr_level = 3;
    pointer.linkage_vis = "extern_weak protected ";
    pointer.qualifier = "global ";
    pointer.llvm_type = "ptr";
    pointer.align_bytes = 16;
    pointer.is_extern_decl = true;
    module.globals.push_back(std::move(pointer));

    lir::LirGlobal array;
    array.name = "enum_pointer_element_array";
    array.type = scalar_type(c4c::TB_ENUM);
    array.type.enum_underlying_base = c4c::TB_UCHAR;
    array.type.ptr_level = 2;
    array.type.array_rank = 2;
    array.type.array_size = 3;
    array.type.array_dims[0] = 3;
    array.type.array_dims[1] = 5;
    array.linkage_vis = "external hidden ";
    array.qualifier = "global ";
    array.llvm_type = "[3 x [5 x ptr]]";
    array.align_bytes = 8;
    array.is_extern_decl = true;
    module.globals.push_back(std::move(array));
    return module;
  };

  auto module = valid_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value(),
         "producer-shaped enum storage globals must import through typed integer normalization");
  expect(bir::FoundationVerifier::verify(raw.value()).ok(),
         "normalized enum storage globals must be Foundation-verifier reachable");
  const auto view = raw.value().view();
  const auto ids = view.global_objects();
  expect(ids.size() == 4 && ids[0].slot == 0 && ids[1].slot == 1 &&
             ids[2].slot == 2 && ids[3].slot == 3,
         "enum storage globals must preserve deterministic source order");
  const auto definition = view.global_object(ids[0]).value();
  const auto declaration = view.global_object(ids[1]).value();
  const auto pointer = view.global_object(ids[2]).value();
  const auto array = view.global_object(ids[3]).value();
  expect(definition.object_type ==
                 bir::Type{bir::TypeKind::Integer, 32, "i32"} &&
             std::holds_alternative<bir::LinkNameId>(definition.identity) &&
             view.spelling(std::get<bir::LinkNameId>(definition.identity))
                     .value() == "default_enum_definition" &&
             !definition.is_internal && !definition.is_weak &&
             !definition.is_const && !definition.is_extern_declaration &&
             definition.visibility == bir::SymbolVisibility::Protected &&
             definition.alignment == 4 && definition.initializer &&
             definition.initializer->opaque_payload ==
                 std::string{"i32 7\0enum", 10} &&
             definition.initializer->function_links.size() == 1 &&
             view.spelling(definition.initializer->function_links[0]).value() ==
                 "enum_init_function",
         "default-underlying enum definitions must retain normalized i32 storage and all object facts");
  expect(declaration.object_type ==
                 bir::Type{bir::TypeKind::Integer, 64, "i64"} &&
             std::holds_alternative<bir::FallbackGlobalName>(
                 declaration.identity) &&
             declaration.is_extern_declaration && !declaration.is_weak &&
             declaration.visibility == bir::SymbolVisibility::Hidden &&
             declaration.alignment == 8 && !declaration.initializer,
         "target-sized explicit-underlying enum externs must preserve exact integer storage and object facts");
  expect(pointer.object_type.kind == bir::TypeKind::Pointer &&
             pointer.object_type.spelling == "ptr" &&
             pointer.object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Integer, 128, 3}} &&
             pointer.is_extern_declaration && pointer.is_weak &&
             pointer.visibility == bir::SymbolVisibility::Protected &&
             pointer.alignment == 16 && !pointer.initializer,
         "enum pointer globals must retain normalized scalar width, exact depth, and object facts");
  expect(array.object_type.kind == bir::TypeKind::Array &&
             array.object_type.spelling == "[3 x [5 x ptr]]" &&
             array.object_type.array_facts ==
                 std::optional<bir::ArrayTypeFacts>{bir::ArrayTypeFacts{
                     bir::TypeKind::Integer, 8, 2, {3, 5}}} &&
             array.is_extern_declaration && !array.is_weak &&
             array.visibility == bir::SymbolVisibility::Hidden &&
             array.alignment == 8 && !array.initializer,
         "fixed enum arrays must retain normalized element width, pointer depth, dimensions, and object facts");

  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  const auto canonical_ids =
      canonical.has_value() ? canonical.value().view().global_objects()
                            : std::vector<bir::GlobalObjectId>{};
  expect(canonical.has_value() && canonical_ids.size() == 4 &&
             canonical.value()
                     .view()
                     .global_object(canonical_ids[0])
                     .value()
                     .object_type ==
                 bir::Type{bir::TypeKind::Integer, 32, "i32"} &&
             canonical.value()
                     .view()
                     .global_object(canonical_ids[1])
                     .value()
                     .object_type ==
                 bir::Type{bir::TypeKind::Integer, 64, "i64"} &&
             canonical.value()
                     .view()
                     .global_object(canonical_ids[2])
                     .value()
                     .object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Integer, 128, 3}} &&
             canonical.value()
                     .view()
                     .global_object(canonical_ids[3])
                     .value()
                     .object_type.array_facts ==
                 std::optional<bir::ArrayTypeFacts>{bir::ArrayTypeFacts{
                     bir::TypeKind::Integer, 8, 2, {3, 5}}},
         "enum direct, pointer, and fixed-array storage must publish Canonical BIR with exact normalized facts");

  const auto rejected = [&](auto mutate, const std::string& message) {
    auto candidate = valid_module();
    mutate(candidate);
    const auto rejected_raw = bir::lower_lir_to_raw_bir(candidate);
    expect(!rejected_raw.has_value() &&
               rejected_raw.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Raw rollback)");
    const auto rejected_canonical =
        bir::lower_lir_to_canonical_bir(candidate);
    expect(!rejected_canonical.has_value() &&
               rejected_canonical.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Canonical rollback)");
  };
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.enum_underlying_base = c4c::TB_FLOAT;
      },
      "floating enum underlying storage must remain closed");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.enum_underlying_base = c4c::TB_STRUCT;
      },
      "aggregate enum underlying storage must remain closed");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.enum_underlying_base = c4c::TB_ENUM;
      },
      "recursive enum underlying storage must remain closed");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.enum_underlying_base = c4c::TB_COMPLEX_INT;
      },
      "complex enum underlying storage must remain closed");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.enum_underlying_base = c4c::TB_VA_LIST;
      },
      "va-list enum underlying storage must remain closed");
  rejected([](lir::LirModule& m) { m.globals[1].llvm_type = "i32"; },
           "enum storage spelling must exactly match normalized width");
  rejected(
      [](lir::LirModule& m) {
        m.globals[1].llvm_type_ref = lir::LirTypeRef::integer(32);
      },
      "enum scalar mirrors must corroborate normalized typed storage");
  rejected([](lir::LirModule& m) { m.globals[2].type.is_fn_ptr = true; },
           "enum function-pointer shapes remain outside scalar-pointer storage");
  rejected(
      [](lir::LirModule& m) {
        m.globals[3].llvm_type_ref = lir::LirTypeRef("[3 x [5 x ptr]]");
      },
      "producer-valid fixed enum arrays must not carry an LLVM type mirror");
  rejected(
      [](lir::LirModule& m) {
        auto& type = m.globals[0].type;
        type.is_vector = true;
        type.vector_lanes = 4;
        type.vector_bytes = 16;
        m.globals[0].llvm_type = "<4 x i32>";
      },
      "enum vectors remain outside the direct enum-storage packet");
}

void test_vrm_register_global_receipt_and_rejections() {
  const auto valid_module = [] {
    lir::LirModule module;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    module.struct_names.attach_text_table(module.link_name_texts.get());
    const auto definition_link = module.link_names.intern("vrm4_definition");
    const auto init_link = module.link_names.intern("vrm_init_function");

    lir::LirGlobal definition;
    definition.name = "vrm4_definition";
    definition.link_name_id = definition_link;
    definition.type = scalar_type(c4c::TB_VRM_REGISTER);
    definition.type.vrm_width = 4;
    definition.linkage_vis = "protected ";
    definition.qualifier = "constant ";
    definition.llvm_type = "c4c.vrm4";
    definition.init_text = "c4c.vrm4 zeroinitializer";
    definition.initializer_function_link_name_ids = {init_link};
    definition.align_bytes = 4;
    definition.is_const = true;
    module.globals.push_back(std::move(definition));

    lir::LirGlobal declaration;
    declaration.name = "vrm8_extern";
    declaration.type = scalar_type(c4c::TB_VRM_REGISTER);
    declaration.type.vrm_width = 8;
    declaration.linkage_vis = "external hidden ";
    declaration.qualifier = "global ";
    declaration.llvm_type = "c4c.vrm8";
    declaration.align_bytes = 8;
    declaration.is_extern_decl = true;
    module.globals.push_back(std::move(declaration));

    lir::LirGlobal pointer;
    pointer.name = "deep_vrm_pointer";
    pointer.type = scalar_type(c4c::TB_VRM_REGISTER);
    pointer.type.vrm_width = 1;
    pointer.type.ptr_level = 3;
    pointer.linkage_vis = "extern_weak protected ";
    pointer.qualifier = "global ";
    pointer.llvm_type = "ptr";
    pointer.llvm_type_ref = lir::LirTypeRef("ptr");
    pointer.align_bytes = 8;
    pointer.is_extern_decl = true;
    module.globals.push_back(std::move(pointer));

    lir::LirGlobal array;
    array.name = "vrm_pointer_array";
    array.type = scalar_type(c4c::TB_VRM_REGISTER);
    array.type.vrm_width = 2;
    array.type.ptr_level = 2;
    array.type.array_rank = 2;
    array.type.array_size = 3;
    array.type.array_dims[0] = 3;
    array.type.array_dims[1] = 5;
    array.linkage_vis = "external hidden ";
    array.qualifier = "global ";
    array.llvm_type = "[3 x [5 x ptr]]";
    array.align_bytes = 8;
    array.is_extern_decl = true;
    module.globals.push_back(std::move(array));
    return module;
  };

  auto module = valid_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value(),
         "producer-shaped direct, pointer, and array VRM globals must import");
  expect(bir::FoundationVerifier::verify(raw.value()).ok(),
         "typed VRM globals must remain Foundation-verifier reachable");
  const auto view = raw.value().view();
  const auto ids = view.global_objects();
  expect(ids.size() == 4 && ids[0].slot == 0 && ids[1].slot == 1 &&
             ids[2].slot == 2 && ids[3].slot == 3,
         "VRM globals must preserve deterministic source order");
  const auto definition = view.global_object(ids[0]).value();
  const auto declaration = view.global_object(ids[1]).value();
  const auto pointer = view.global_object(ids[2]).value();
  const auto array = view.global_object(ids[3]).value();

  expect(definition.object_type.kind == bir::TypeKind::VrmRegister &&
             definition.object_type.bit_width == 4 &&
             definition.object_type.spelling == "c4c.vrm4" &&
             std::holds_alternative<bir::LinkNameId>(definition.identity) &&
             view.spelling(std::get<bir::LinkNameId>(definition.identity))
                     .value() == "vrm4_definition" &&
             !definition.is_internal && !definition.is_weak &&
             definition.is_const && !definition.is_extern_declaration &&
             definition.visibility == bir::SymbolVisibility::Protected &&
             definition.alignment == 4 && definition.initializer &&
             definition.initializer->opaque_payload ==
                 "c4c.vrm4 zeroinitializer" &&
             definition.initializer->function_links.size() == 1 &&
             view.spelling(definition.initializer->function_links[0]).value() ==
                 "vrm_init_function",
         "direct VRM definitions must retain exact width, spelling, and object and initializer facts");
  expect(declaration.object_type.kind == bir::TypeKind::VrmRegister &&
             declaration.object_type.bit_width == 8 &&
             declaration.object_type.spelling == "c4c.vrm8" &&
             declaration.is_extern_declaration && !declaration.is_weak &&
             declaration.visibility == bir::SymbolVisibility::Hidden &&
             declaration.alignment == 8 && !declaration.initializer,
         "VRM externs must retain exact width, spelling, linkage, and object facts");
  expect(pointer.object_type.kind == bir::TypeKind::Pointer &&
             pointer.object_type.spelling == "ptr" &&
             pointer.object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::VrmRegister, 1, 3}} &&
             pointer.is_extern_declaration && pointer.is_weak &&
             pointer.visibility == bir::SymbolVisibility::Protected &&
             pointer.alignment == 8 && !pointer.initializer,
         "deep VRM pointers must retain exact base width, pointer depth, and object facts");
  expect(array.object_type.kind == bir::TypeKind::Array &&
             array.object_type.spelling == "[3 x [5 x ptr]]" &&
             array.object_type.array_facts ==
                 std::optional<bir::ArrayTypeFacts>{bir::ArrayTypeFacts{
                     bir::TypeKind::VrmRegister, 2, 2, {3, 5}}} &&
             array.is_extern_declaration && !array.is_weak &&
             array.visibility == bir::SymbolVisibility::Hidden &&
             array.alignment == 8 && !array.initializer,
         "fixed VRM pointer arrays must retain exact base width, depth, dimensions, and object facts");

  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  const auto canonical_ids =
      canonical.has_value() ? canonical.value().view().global_objects()
                            : std::vector<bir::GlobalObjectId>{};
  expect(canonical.has_value() && canonical_ids.size() == 4 &&
             canonical.value().view().global_object(canonical_ids[0])
                     .value().object_type == definition.object_type &&
             canonical.value().view().global_object(canonical_ids[1])
                     .value().object_type == declaration.object_type &&
             canonical.value().view().global_object(canonical_ids[2])
                     .value().object_type.pointer_facts ==
                 pointer.object_type.pointer_facts &&
             canonical.value().view().global_object(canonical_ids[3])
                     .value().object_type.array_facts ==
                 array.object_type.array_facts,
         "VRM direct, pointer, and array storage must publish exact Canonical BIR facts");

  const auto rejected = [&](auto mutate, const std::string& message) {
    auto candidate = valid_module();
    mutate(candidate);
    const auto rejected_raw = bir::lower_lir_to_raw_bir(candidate);
    expect(!rejected_raw.has_value() &&
               rejected_raw.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Raw rollback)");
    const auto rejected_canonical =
        bir::lower_lir_to_canonical_bir(candidate);
    expect(!rejected_canonical.has_value() &&
               rejected_canonical.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Canonical rollback)");
  };
  rejected([](lir::LirModule& m) { m.globals[0].type.vrm_width = 0; },
           "zero direct VRM width must reject transactionally");
  rejected([](lir::LirModule& m) { m.globals[0].type.vrm_width = -1; },
           "negative direct VRM width must reject transactionally");
  rejected([](lir::LirModule& m) { m.globals[0].type.vrm_width = 3; },
           "unsupported direct VRM width must reject transactionally");
  rejected([](lir::LirModule& m) { m.globals[0].llvm_type = "c4c.vrm2"; },
           "VRM spelling must exactly corroborate typed width");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].llvm_type_ref = lir::LirTypeRef("c4c.vrm4");
      },
      "producer-shaped direct VRM globals must reject unexpected mirrors");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.base = c4c::TB_INT;
        m.globals[0].llvm_type = "i32";
        m.globals[0].llvm_type_ref = lir::LirTypeRef::integer(32);
      },
      "residual VRM width must not fall through a corroborated scalar route");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.is_vector = true;
        m.globals[0].type.vector_lanes = 4;
        m.globals[0].type.vector_bytes = 4;
        m.globals[0].llvm_type = "<4 x c4c.vrm4>";
        m.globals[0].llvm_type_ref.reset();
      },
      "VRM vectors remain excluded from the existing scalar vector route");
  rejected([](lir::LirModule& m) { m.globals[2].type.is_fn_ptr = true; },
           "VRM function pointers remain outside scalar-pointer storage");
  rejected(
      [](lir::LirModule& m) {
        m.globals[2].type.is_ptr_to_array = true;
        m.globals[2].type.inner_rank = 1;
      },
      "VRM pointer-to-array declarators remain unsupported");
  rejected([](lir::LirModule& m) { m.globals[0].type.is_lvalue_ref = true; },
           "VRM references remain outside global storage receipt");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.base = c4c::TB_STRUCT;
        m.globals[0].llvm_type = "{ i32 }";
        m.globals[0].llvm_type_ref = lir::LirTypeRef("{ i32 }");
      },
      "residual VRM width must not fall through a corroborated aggregate route");
  rejected([](lir::LirModule& m) { m.globals[0].type.base = c4c::TB_VA_LIST; },
           "va-list TypeSpec neighbors must remain closed");
  rejected(
      [](lir::LirModule& m) {
        m.globals[2].llvm_type_ref = lir::LirTypeRef("c4c.vrm1");
      },
      "VRM pointers must reject non-pointer mirrors");
  rejected(
      [](lir::LirModule& m) {
        m.globals[3].llvm_type_ref = lir::LirTypeRef("[3 x [5 x ptr]]");
      },
      "VRM arrays must reject unexpected mirrors");
}

void test_complex_storage_global_receipt_and_rejections() {
  const auto valid_module = [] {
    lir::LirModule module;
    module.target_profile.arch = c4c::TargetArch::I686;
    module.target_profile.os = c4c::TargetOs::Linux;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    module.struct_names.attach_text_table(module.link_name_texts.get());
    const auto definition_link =
        module.link_names.intern("complex_long_double_definition");
    const auto init_link = module.link_names.intern("complex_init_function");

    lir::LirGlobal definition;
    definition.name = "complex_long_double_definition";
    definition.link_name_id = definition_link;
    definition.type = scalar_type(c4c::TB_COMPLEX_LONGDOUBLE);
    definition.linkage_vis = "protected ";
    definition.qualifier = "constant ";
    definition.llvm_type = "{ x86_fp80, x86_fp80 }";
    definition.init_text = std::string{
        "{ x86_fp80, x86_fp80 } zeroinitializer\0complex", 46};
    definition.initializer_function_link_name_ids = {init_link};
    definition.align_bytes = 16;
    definition.is_const = true;
    module.globals.push_back(std::move(definition));

    lir::LirGlobal declaration;
    declaration.name = "complex_integer_extern";
    declaration.type = scalar_type(c4c::TB_COMPLEX_INT);
    declaration.linkage_vis = "external hidden ";
    declaration.qualifier = "global ";
    declaration.llvm_type = "{ i32, i32 }";
    declaration.align_bytes = 8;
    declaration.is_extern_decl = true;
    module.globals.push_back(std::move(declaration));

    lir::LirGlobal pointer;
    pointer.name = "deep_complex_integer_pointer";
    pointer.type = scalar_type(c4c::TB_COMPLEX_ULONG);
    pointer.type.ptr_level = 3;
    pointer.linkage_vis = "extern_weak protected ";
    pointer.qualifier = "global ";
    pointer.llvm_type = "ptr";
    pointer.align_bytes = 8;
    pointer.is_extern_decl = true;
    module.globals.push_back(std::move(pointer));

    lir::LirGlobal array;
    array.name = "complex_float_pointer_array";
    array.type = scalar_type(c4c::TB_COMPLEX_FLOAT);
    array.type.ptr_level = 2;
    array.type.array_rank = 2;
    array.type.array_size = 3;
    array.type.array_dims[0] = 3;
    array.type.array_dims[1] = 4;
    array.linkage_vis = "external hidden ";
    array.qualifier = "global ";
    array.llvm_type = "[3 x [4 x ptr]]";
    array.align_bytes = 8;
    array.is_extern_decl = true;
    module.globals.push_back(std::move(array));
    return module;
  };

  auto module = valid_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value(),
         "producer-shaped complex globals must import from typed TypeSpec authority");
  expect(bir::FoundationVerifier::verify(raw.value()).ok(),
         "typed complex globals must remain Foundation-verifier reachable");
  const auto view = raw.value().view();
  const auto ids = view.global_objects();
  expect(ids.size() == 4 && ids[0].slot == 0 && ids[1].slot == 1 &&
             ids[2].slot == 2 && ids[3].slot == 3,
         "complex globals must preserve deterministic source order");
  const auto definition = view.global_object(ids[0]).value();
  const auto declaration = view.global_object(ids[1]).value();
  const auto pointer = view.global_object(ids[2]).value();
  const auto array = view.global_object(ids[3]).value();

  bir::Type expected_definition{bir::TypeKind::Complex, 80,
                                "{ x86_fp80, x86_fp80 }"};
  expected_definition.complex_facts =
      bir::ComplexTypeFacts{bir::TypeKind::Floating, 80};
  bir::Type expected_declaration{bir::TypeKind::Complex, 32,
                                 "{ i32, i32 }"};
  expected_declaration.complex_facts =
      bir::ComplexTypeFacts{bir::TypeKind::Integer, 32};
  expect(definition.object_type == expected_definition &&
             std::holds_alternative<bir::LinkNameId>(definition.identity) &&
             view.spelling(std::get<bir::LinkNameId>(definition.identity))
                     .value() == "complex_long_double_definition" &&
             !definition.is_internal && !definition.is_weak &&
             definition.is_const && !definition.is_extern_declaration &&
             definition.visibility == bir::SymbolVisibility::Protected &&
             definition.alignment == 16 && definition.initializer &&
             definition.initializer->opaque_payload ==
                 std::string{
                     "{ x86_fp80, x86_fp80 } zeroinitializer\0complex", 46} &&
             definition.initializer->function_links.size() == 1 &&
             view.spelling(definition.initializer->function_links[0]).value() ==
                 "complex_init_function",
         "floating complex definitions must retain exact component and object authority");
  expect(declaration.object_type == expected_declaration &&
             declaration.is_extern_declaration && !declaration.is_weak &&
             declaration.visibility == bir::SymbolVisibility::Hidden &&
             declaration.alignment == 8 && !declaration.initializer,
         "integer complex externs must retain exact component and linkage facts");
  expect(pointer.object_type.kind == bir::TypeKind::Pointer &&
             pointer.object_type.spelling == "ptr" &&
             pointer.object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Complex, 64, 3,
                     bir::ComplexTypeFacts{bir::TypeKind::Integer, 64}}} &&
             pointer.is_extern_declaration && pointer.is_weak &&
             pointer.visibility == bir::SymbolVisibility::Protected &&
             pointer.alignment == 8 && !pointer.initializer,
         "deep complex pointers must retain typed components and exact depth");
  expect(array.object_type.kind == bir::TypeKind::Array &&
             array.object_type.spelling == "[3 x [4 x ptr]]" &&
             array.object_type.array_facts ==
                 std::optional<bir::ArrayTypeFacts>{bir::ArrayTypeFacts{
                     bir::TypeKind::Complex, 32, 2, {3, 4},
                     bir::ComplexTypeFacts{bir::TypeKind::Floating, 32}}} &&
             array.is_extern_declaration && !array.is_weak &&
             array.visibility == bir::SymbolVisibility::Hidden &&
             array.alignment == 8 && !array.initializer,
         "fixed complex arrays must retain typed components, dimensions, and pointer depth");

  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  const auto canonical_ids =
      canonical.has_value() ? canonical.value().view().global_objects()
                            : std::vector<bir::GlobalObjectId>{};
  expect(canonical.has_value() && canonical_ids.size() == 4 &&
             canonical.value().view().global_object(canonical_ids[0])
                     .value().object_type == expected_definition &&
             canonical.value().view().global_object(canonical_ids[1])
                     .value().object_type == expected_declaration &&
             canonical.value().view().global_object(canonical_ids[2])
                     .value().object_type.pointer_facts ==
                 pointer.object_type.pointer_facts &&
             canonical.value().view().global_object(canonical_ids[3])
                     .value().object_type.array_facts ==
                 array.object_type.array_facts,
         "complex direct, pointer, and array storage must publish exact Canonical BIR facts");

  const auto rejected = [&](auto mutate, const std::string& message) {
    auto candidate = valid_module();
    mutate(candidate);
    const auto rejected_raw = bir::lower_lir_to_raw_bir(candidate);
    expect(!rejected_raw.has_value() &&
               rejected_raw.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Raw rollback)");
    const auto rejected_canonical =
        bir::lower_lir_to_canonical_bir(candidate);
    expect(!rejected_canonical.has_value() &&
               rejected_canonical.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Canonical rollback)");
  };
  rejected([](lir::LirModule& m) { m.globals[0].llvm_type = "{ double, double }"; },
           "complex storage spelling must exactly match typed components");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].llvm_type_ref = lir::LirTypeRef(m.globals[0].llvm_type);
      },
      "complex globals must reject unexpected aggregate-shaped mirrors");
  rejected([](lir::LirModule& m) { m.globals[2].type.is_fn_ptr = true; },
           "complex function pointers remain outside scalar-pointer storage");
  rejected([](lir::LirModule& m) { m.globals[2].type.is_ptr_to_array = true; },
           "complex pointers to arrays remain outside scalar-pointer storage");
  rejected([](lir::LirModule& m) { m.globals[0].type.is_lvalue_ref = true; },
           "complex references remain outside global storage receipt");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.is_vector = true;
        m.globals[0].type.vector_lanes = 2;
        m.globals[0].type.vector_bytes = 32;
        m.globals[0].llvm_type = "<2 x { x86_fp80, x86_fp80 }>";
      },
      "complex vectors remain outside direct scalar-vector receipt");
  rejected([](lir::LirModule& m) { m.globals[0].type.base = c4c::TB_STRUCT; },
           "aggregate TypeSpec neighbors must not borrow complex storage authority");
  rejected([](lir::LirModule& m) { m.globals[0].type.base = c4c::TB_VA_LIST; },
           "va-list TypeSpec neighbors must remain closed");
  rejected([](lir::LirModule& m) { m.globals[3].type.array_dims[1] = -2; },
           "malformed complex fixed-array dimensions must roll back transactionally");
}

void test_scalar_pointer_global_receipt_and_rejections() {
  const auto valid_module = [] {
    lir::LirModule module;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    module.struct_names.attach_text_table(module.link_name_texts.get());
    const auto linked = module.link_names.intern("extern_integer_pointer");
    const auto ordinary_link =
        module.link_names.intern("ordinary_integer_pointer");
    const auto internal_const_link =
        module.link_names.intern("internal_const_float_pointer");
    const auto weak_const_link =
        module.link_names.intern("weak_const_integer_pointer");
    const auto init_a = module.link_names.intern("pointer_init_a");
    const auto init_b = module.link_names.intern("pointer_init_b");

    lir::LirGlobal external;
    external.name = "extern_integer_pointer";
    external.link_name_id = linked;
    external.type = scalar_type(c4c::TB_LONGLONG);
    external.type.ptr_level = 1;
    external.linkage_vis = "external hidden ";
    external.qualifier = "global ";
    external.llvm_type = "ptr";
    external.align_bytes = 8;
    external.is_extern_decl = true;
    module.globals.push_back(std::move(external));

    lir::LirGlobal weak_external;
    weak_external.name = "weak_external_float_pointer";
    weak_external.type = scalar_type(c4c::TB_FLOAT);
    weak_external.type.ptr_level = 3;
    weak_external.linkage_vis = "extern_weak protected ";
    weak_external.qualifier = "global ";
    weak_external.llvm_type = "ptr";
    weak_external.align_bytes = 16;
    weak_external.is_extern_decl = true;
    module.globals.push_back(std::move(weak_external));

    lir::LirGlobal ordinary;
    ordinary.name = "ordinary_integer_pointer";
    ordinary.link_name_id = ordinary_link;
    ordinary.type = scalar_type(c4c::TB_UINT);
    ordinary.type.ptr_level = 2;
    ordinary.linkage_vis.clear();
    ordinary.qualifier = "global ";
    ordinary.llvm_type = "ptr";
    ordinary.align_bytes = 8;
    ordinary.init_text = std::string{"ptr @ordinary\0tail", 18};
    ordinary.initializer_function_link_name_ids = {init_b, init_a, init_b};
    module.globals.push_back(std::move(ordinary));

    lir::LirGlobal internal;
    internal.name = "internal_float_pointer";
    internal.type = scalar_type(c4c::TB_DOUBLE);
    internal.type.ptr_level = 1;
    internal.is_internal = true;
    internal.linkage_vis = "internal hidden ";
    internal.qualifier = "global ";
    internal.llvm_type = "ptr";
    internal.align_bytes = 16;
    internal.init_text = "ptr @internal";
    internal.initializer_function_link_name_ids = {init_a};
    module.globals.push_back(std::move(internal));

    lir::LirGlobal weak;
    weak.name = "weak_integer_pointer";
    weak.type = scalar_type(c4c::TB_SHORT);
    weak.type.ptr_level = 1;
    weak.linkage_vis = "weak protected ";
    weak.qualifier = "global ";
    weak.llvm_type = "ptr";
    weak.align_bytes = 32;
    weak.init_text = "ptr @weak";
    weak.initializer_function_link_name_ids = {init_a, init_b};
    module.globals.push_back(std::move(weak));

    lir::LirGlobal internal_const;
    internal_const.name = "internal_const_float_pointer";
    internal_const.link_name_id = internal_const_link;
    internal_const.type = scalar_type(c4c::TB_FLOAT);
    internal_const.type.ptr_level = 1;
    internal_const.is_internal = true;
    internal_const.is_const = true;
    internal_const.linkage_vis = "internal protected ";
    internal_const.qualifier = "global ";
    internal_const.llvm_type = "ptr";
    internal_const.align_bytes = 64;
    internal_const.init_text =
        std::string{"ptr @internal_const\0tail", 24};
    internal_const.initializer_function_link_name_ids = {init_b, init_a,
                                                         init_b};
    module.globals.push_back(std::move(internal_const));

    lir::LirGlobal weak_const;
    weak_const.name = "weak_const_integer_pointer";
    weak_const.link_name_id = weak_const_link;
    weak_const.type = scalar_type(c4c::TB_LONGLONG);
    weak_const.type.ptr_level = 1;
    weak_const.is_const = true;
    weak_const.linkage_vis = "weak hidden ";
    weak_const.qualifier = "global ";
    weak_const.llvm_type = "ptr";
    weak_const.align_bytes = 8;
    weak_const.init_text = std::string{"ptr @weak_const\0tail", 20};
    weak_const.initializer_function_link_name_ids = {init_a, init_b, init_a};
    module.globals.push_back(std::move(weak_const));
    return module;
  };

  auto module = valid_module();
  const auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(),
         "producer-shaped scalar-pointer extern declarations must import without a mirror");
  expect(bir::FoundationVerifier::verify(imported.value()).ok(),
         "typed scalar-pointer extern declarations must be Foundation reachable");
  const auto view = imported.value().view();
  const auto ids = view.global_objects();
  expect(ids.size() == 7 && ids[0].slot == 0 && ids[1].slot == 1 &&
             ids[2].slot == 2 && ids[3].slot == 3 && ids[4].slot == 4 &&
             ids[5].slot == 5 && ids[6].slot == 6,
         "typed scalar-pointer objects must preserve source order");
  const auto external = view.global_object(ids[0]).value();
  const auto weak_external = view.global_object(ids[1]).value();
  const auto ordinary = view.global_object(ids[2]).value();
  const auto internal = view.global_object(ids[3]).value();
  const auto weak = view.global_object(ids[4]).value();
  const auto internal_const = view.global_object(ids[5]).value();
  const auto weak_const = view.global_object(ids[6]).value();
  expect(external.source_name == "extern_integer_pointer" &&
             external.object_type.kind == bir::TypeKind::Pointer &&
             external.object_type.bit_width == 0 &&
             external.object_type.spelling == "ptr" &&
             external.object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Integer, 64, 1}} &&
             std::holds_alternative<bir::LinkNameId>(external.identity) &&
             view.spelling(std::get<bir::LinkNameId>(external.identity))
                     .value() == "extern_integer_pointer" &&
             !external.is_internal && !external.is_weak &&
             !external.is_const && external.is_extern_declaration &&
             external.visibility == bir::SymbolVisibility::Hidden &&
             external.alignment == 8 && !external.initializer,
         "external scalar pointers must retain exact pointee and object facts");
  expect(weak_external.object_type.kind == bir::TypeKind::Pointer &&
             weak_external.object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Floating, 32, 3}} &&
             std::holds_alternative<bir::FallbackGlobalName>(
                 weak_external.identity) &&
             std::get<bir::FallbackGlobalName>(weak_external.identity).name ==
                 "weak_external_float_pointer" &&
             !weak_external.is_internal && weak_external.is_weak &&
             !weak_external.is_const && weak_external.is_extern_declaration &&
             weak_external.visibility == bir::SymbolVisibility::Protected &&
             weak_external.alignment == 16 && !weak_external.initializer,
         "deep weak external scalar pointers must retain exact typed depth, pointee, linkage, visibility, alignment, and identity");
  expect(ordinary.object_type.kind == bir::TypeKind::Pointer &&
             ordinary.object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Integer, 32, 2}} &&
             std::holds_alternative<bir::LinkNameId>(ordinary.identity) &&
             view.spelling(std::get<bir::LinkNameId>(ordinary.identity))
                     .value() == "ordinary_integer_pointer" &&
             !ordinary.is_internal && !ordinary.is_weak &&
             !ordinary.is_const && !ordinary.is_extern_declaration &&
             ordinary.visibility == bir::SymbolVisibility::Default &&
             ordinary.alignment == 8 && ordinary.initializer &&
             ordinary.initializer->opaque_payload ==
                 std::string{"ptr @ordinary\0tail", 18} &&
             ordinary.initializer->function_links.size() == 3 &&
             view.spelling(ordinary.initializer->function_links[0]).value() ==
                 "pointer_init_b" &&
             view.spelling(ordinary.initializer->function_links[1]).value() ==
                 "pointer_init_a" &&
             ordinary.initializer->function_links[2] ==
                 ordinary.initializer->function_links[0],
         "deep ordinary scalar-pointer definitions must preserve exact typed depth, authority, identity, byte-exact payload, and ordered initializer links");
  expect(internal.object_type.kind == bir::TypeKind::Pointer &&
             internal.object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Floating, 64, 1}} &&
             std::holds_alternative<bir::FallbackGlobalName>(
                 internal.identity) &&
             std::get<bir::FallbackGlobalName>(internal.identity).name ==
                 "internal_float_pointer" &&
             internal.is_internal && !internal.is_weak &&
             !internal.is_const && !internal.is_extern_declaration &&
             internal.visibility == bir::SymbolVisibility::Hidden &&
             internal.alignment == 16 && internal.initializer &&
             internal.initializer->opaque_payload == "ptr @internal" &&
             internal.initializer->function_links.size() == 1 &&
             view.spelling(internal.initializer->function_links[0]).value() ==
                 "pointer_init_a",
         "internal scalar-pointer definitions must preserve pointee, linkage, visibility, alignment, and initializer facts");
  expect(weak.object_type.kind == bir::TypeKind::Pointer &&
             weak.object_type.pointer_facts ==
             std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Integer, 16, 1}} &&
             std::holds_alternative<bir::FallbackGlobalName>(weak.identity) &&
             std::get<bir::FallbackGlobalName>(weak.identity).name ==
                 "weak_integer_pointer" &&
             !weak.is_internal && weak.is_weak && !weak.is_const &&
             !weak.is_extern_declaration &&
             weak.visibility == bir::SymbolVisibility::Protected &&
             weak.alignment == 32 && weak.initializer &&
             weak.initializer->opaque_payload == "ptr @weak" &&
             weak.initializer->function_links.size() == 2 &&
             view.spelling(weak.initializer->function_links[0]).value() ==
                 "pointer_init_a" &&
             view.spelling(weak.initializer->function_links[1]).value() ==
                 "pointer_init_b",
         "weak scalar-pointer definitions must preserve typed pointee, weak linkage, visibility, and ordered initializer links");
  expect(internal_const.object_type.kind == bir::TypeKind::Pointer &&
             internal_const.object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Floating, 32, 1}} &&
             std::holds_alternative<bir::LinkNameId>(
                 internal_const.identity) &&
             view.spelling(std::get<bir::LinkNameId>(internal_const.identity))
                     .value() == "internal_const_float_pointer" &&
             internal_const.is_internal && !internal_const.is_weak &&
             internal_const.is_const &&
             !internal_const.is_extern_declaration &&
             internal_const.visibility == bir::SymbolVisibility::Protected &&
             internal_const.alignment == 64 && internal_const.initializer &&
             internal_const.initializer->opaque_payload ==
                 std::string{"ptr @internal_const\0tail", 24} &&
             internal_const.initializer->function_links.size() == 3 &&
             view.spelling(internal_const.initializer->function_links[0])
                     .value() == "pointer_init_b" &&
             view.spelling(internal_const.initializer->function_links[1])
                     .value() == "pointer_init_a" &&
             internal_const.initializer->function_links[2] ==
                 internal_const.initializer->function_links[0],
         "internal const-pointer definitions must preserve typed pointee, linkage, identity, visibility, alignment, opaque payload, and ordered links");
  expect(weak_const.object_type.kind == bir::TypeKind::Pointer &&
             weak_const.object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Integer, 64, 1}} &&
             std::holds_alternative<bir::LinkNameId>(weak_const.identity) &&
             view.spelling(std::get<bir::LinkNameId>(weak_const.identity))
                     .value() == "weak_const_integer_pointer" &&
             !weak_const.is_internal && weak_const.is_weak &&
             weak_const.is_const && !weak_const.is_extern_declaration &&
             weak_const.visibility == bir::SymbolVisibility::Hidden &&
             weak_const.alignment == 8 && weak_const.initializer &&
             weak_const.initializer->opaque_payload ==
                 std::string{"ptr @weak_const\0tail", 20} &&
             weak_const.initializer->function_links.size() == 3 &&
             view.spelling(weak_const.initializer->function_links[0]).value() ==
                 "pointer_init_a" &&
             view.spelling(weak_const.initializer->function_links[1]).value() ==
                 "pointer_init_b" &&
             weak_const.initializer->function_links[2] ==
                 weak_const.initializer->function_links[0],
         "weak const-pointer definitions must preserve typed pointee, weak linkage, identity, visibility, alignment, opaque payload, and ordered links");
  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  const auto canonical_ids =
      canonical.has_value() ? canonical.value().view().global_objects()
                            : std::vector<bir::GlobalObjectId>{};
  expect(canonical.has_value() && canonical_ids.size() == 7 &&
             canonical.value()
                     .view()
                     .global_object(canonical_ids[0])
                     .value()
                     .object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Integer, 64, 1}} &&
             canonical.value()
                     .view()
                     .global_object(canonical_ids[1])
                     .value()
                     .object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Floating, 32, 3}} &&
             canonical.value()
                     .view()
                     .global_object(canonical_ids[2])
                     .value()
                     .object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Integer, 32, 2}} &&
             canonical.value()
                     .view()
                     .global_object(canonical_ids[3])
                     .value()
                     .object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Floating, 64, 1}} &&
             canonical.value()
                     .view()
                     .global_object(canonical_ids[4])
                     .value()
                     .object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Integer, 16, 1}} &&
             canonical.value()
                     .view()
                     .global_object(canonical_ids[5])
                     .value()
                     .object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Floating, 32, 1}} &&
             canonical.value()
                     .view()
                     .global_object(canonical_ids[6])
                     .value()
                     .object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Integer, 64, 1}},
         "producer-shaped scalar-pointer declarations and definitions must publish Canonical BIR");

  const auto rejected = [&](auto mutate, const std::string& message) {
    auto candidate = valid_module();
    mutate(candidate);
    const auto raw = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw.has_value() &&
               raw.error().code == bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Raw rollback)");
    const auto canonical_candidate =
        bir::lower_lir_to_canonical_bir(candidate);
    expect(!canonical_candidate.has_value() &&
               canonical_candidate.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Canonical rollback)");
  };
  rejected([](lir::LirModule& m) { m.globals[0].type.ptr_level = -1; },
           "negative scalar-pointer extern depth must remain closed");
  rejected([](lir::LirModule& m) { m.globals[0].type.base = c4c::TB_STRUCT; },
           "aggregate scalar-pointer pointees must remain closed");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.is_ptr_to_array = true;
        m.globals[0].type.inner_rank = 1;
      },
      "pointer-to-array and inner-rank extern shapes must remain closed");
  rejected([](lir::LirModule& m) { m.globals[0].type.is_fn_ptr = true; },
           "function-pointer extern shapes must remain closed");
  rejected([](lir::LirModule& m) { m.globals[0].llvm_type = "i64*"; },
           "scalar-pointer rendered spelling is parity-only and must be ptr");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].llvm_type_ref = lir::LirTypeRef::integer(64);
      },
      "optional scalar-pointer mirrors must corroborate generic ptr evidence");
  rejected([](lir::LirModule& m) { m.globals[2].init_text.clear(); },
           "scalar-pointer definitions require initializer payloads");
  rejected([](lir::LirModule& m) { m.globals[2].qualifier = "constant "; },
           "nonconst scalar-pointer definitions reject constant qualifiers");
  rejected([](lir::LirModule& m) { m.globals[3].is_internal = false; },
           "internal scalar-pointer linkage must agree with its flag");
  rejected([](lir::LirModule& m) { m.globals[5].qualifier = "constant "; },
           "internal const-pointer definitions reject constant qualifiers");
  rejected([](lir::LirModule& m) { m.globals[6].init_text.clear(); },
           "weak const-pointer definitions require initializer payloads");
  rejected([](lir::LirModule& m) { m.globals[5].is_internal = false; },
           "internal const-pointer linkage must agree with its flag");
  rejected([](lir::LirModule& m) { m.globals[5].linkage_vis = "weak "; },
           "internal and weak const-pointer facts must not coexist");
  rejected([](lir::LirModule& m) { m.globals[5].is_extern_decl = true; },
           "internal const-pointer definitions cannot also be extern declarations");
  rejected([](lir::LirModule& m) { m.globals[6].is_extern_decl = true; },
           "weak const-pointer definitions cannot contradict external linkage");
  rejected([](lir::LirModule& m) { m.globals[5].type.ptr_level = -2; },
           "negative internal const-pointer depth must remain closed");
  rejected([](lir::LirModule& m) { m.globals[6].type.base = c4c::TB_STRUCT; },
           "weak const-pointer aggregate pointees must remain closed");
  rejected([](lir::LirModule& m) { m.globals[5].type.is_fn_ptr = true; },
           "internal const function-pointer shapes must remain closed");
  rejected(
      [](lir::LirModule& m) {
        m.globals[5].llvm_type_ref = lir::LirTypeRef::integer(32);
      },
      "internal const-pointer mirrors must corroborate generic ptr evidence");
  rejected([](lir::LirModule& m) { m.globals[6].llvm_type = "i64*"; },
           "weak const-pointer rendered spelling is parity-only and must be ptr");
}

void test_pointer_to_array_global_receipt_and_rejections() {
  const auto valid_module = [] {
    lir::LirModule module;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    module.struct_names.attach_text_table(module.link_name_texts.get());
    const auto definition_link =
        module.link_names.intern("complex_pointer_to_array");
    const auto init_a = module.link_names.intern("pointer_array_init_a");
    const auto init_b = module.link_names.intern("pointer_array_init_b");

    lir::LirGlobal definition;
    definition.name = "complex_pointer_to_array";
    definition.link_name_id = definition_link;
    definition.type = scalar_type(c4c::TB_COMPLEX_DOUBLE);
    definition.type.ptr_level = 2;
    definition.type.array_rank = 1;
    definition.type.array_size = 4;
    definition.type.array_dims[0] = 4;
    definition.type.is_ptr_to_array = true;
    definition.type.inner_rank = -1;
    definition.linkage_vis = "protected ";
    definition.qualifier = "global ";
    definition.llvm_type = "ptr";
    definition.align_bytes = 16;
    definition.init_text = std::string{"ptr @matrix\0tail", 16};
    definition.initializer_function_link_name_ids = {init_b, init_a, init_b};
    module.globals.push_back(std::move(definition));

    lir::LirGlobal weak_external;
    weak_external.name = "vrm_pointer_to_array";
    weak_external.type = scalar_type(c4c::TB_VRM_REGISTER);
    weak_external.type.vrm_width = 4;
    weak_external.type.ptr_level = 1;
    weak_external.type.array_rank = 3;
    weak_external.type.array_size = 2;
    weak_external.type.array_dims[0] = 2;
    weak_external.type.array_dims[1] = 0;
    weak_external.type.array_dims[2] = 5;
    weak_external.type.is_ptr_to_array = true;
    weak_external.type.inner_rank = 3;
    weak_external.linkage_vis = "extern_weak hidden ";
    weak_external.qualifier = "global ";
    weak_external.llvm_type = "ptr";
    weak_external.align_bytes = 8;
    weak_external.is_extern_decl = true;
    module.globals.push_back(std::move(weak_external));
    return module;
  };

  auto module = valid_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value(),
         "producer-shaped pure pointer-to-array globals must import from TypeSpec authority");
  expect(bir::FoundationVerifier::verify(raw.value()).ok(),
         "typed pure pointer-to-array globals must be Foundation reachable");
  const auto view = raw.value().view();
  const auto ids = view.global_objects();
  expect(ids.size() == 2 && ids[0].slot == 0 && ids[1].slot == 1,
         "pure pointer-to-array globals must preserve source order");
  const auto definition = view.global_object(ids[0]).value();
  const auto weak_external = view.global_object(ids[1]).value();
  expect(definition.object_type.kind == bir::TypeKind::Pointer &&
             definition.object_type.bit_width == 0 &&
             definition.object_type.spelling == "ptr" &&
             definition.object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Complex, 64, 2,
                     bir::ComplexTypeFacts{bir::TypeKind::Floating, 64},
                     bir::PointerArrayTypeFacts{{4}, -1}}} &&
             std::holds_alternative<bir::LinkNameId>(definition.identity) &&
             view.spelling(std::get<bir::LinkNameId>(definition.identity))
                     .value() == "complex_pointer_to_array" &&
             !definition.is_internal && !definition.is_weak &&
             !definition.is_const && !definition.is_extern_declaration &&
             definition.visibility == bir::SymbolVisibility::Protected &&
             definition.alignment == 16 && definition.initializer &&
             definition.initializer->opaque_payload ==
                 std::string{"ptr @matrix\0tail", 16} &&
             definition.initializer->function_links.size() == 3 &&
             view.spelling(definition.initializer->function_links[0]).value() ==
                 "pointer_array_init_b" &&
             view.spelling(definition.initializer->function_links[1]).value() ==
                 "pointer_array_init_a" &&
             definition.initializer->function_links[2] ==
                 definition.initializer->function_links[0],
         "initialized complex pointer-to-array definitions must preserve exact nested type and object facts");
  expect(weak_external.object_type.kind == bir::TypeKind::Pointer &&
             weak_external.object_type.spelling == "ptr" &&
             weak_external.object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::VrmRegister, 4, 1, std::nullopt,
                     bir::PointerArrayTypeFacts{{2, 0, 5}, 3}}} &&
             std::holds_alternative<bir::FallbackGlobalName>(
                 weak_external.identity) &&
             weak_external.is_weak && weak_external.is_extern_declaration &&
             !weak_external.is_internal && !weak_external.is_const &&
             weak_external.visibility == bir::SymbolVisibility::Hidden &&
             weak_external.alignment == 8 && !weak_external.initializer,
         "weak VRM pointer-to-array externs must preserve zero dimensions, exact-rank form, and all object facts");

  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  const auto canonical_ids =
      canonical.has_value() ? canonical.value().view().global_objects()
                            : std::vector<bir::GlobalObjectId>{};
  expect(canonical.has_value() && canonical_ids.size() == 2 &&
             canonical.value().view().global_object(canonical_ids[0])
                     .value().object_type == definition.object_type &&
             canonical.value().view().global_object(canonical_ids[1])
                     .value().object_type == weak_external.object_type,
         "pure pointer-to-array globals must publish exact Canonical BIR facts");

  const auto rejected = [&](auto mutate, const std::string& message) {
    auto candidate = valid_module();
    mutate(candidate);
    const auto rejected_raw = bir::lower_lir_to_raw_bir(candidate);
    expect(!rejected_raw.has_value() &&
               rejected_raw.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Raw rollback)");
    const auto rejected_canonical =
        bir::lower_lir_to_canonical_bir(candidate);
    expect(!rejected_canonical.has_value() &&
               rejected_canonical.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Canonical rollback)");
  };
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.array_size = -1;
        m.globals[0].type.array_dims[0] = -1;
      },
      "negative pointer-to-array dimensions must remain closed");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.array_size = -2;
        m.globals[0].type.array_dims[0] = -2;
      },
      "unsized pointer-to-array dimensions must remain closed");
  rejected([](lir::LirModule& m) { m.globals[0].type.array_dims[0] = 3; },
           "pointer-to-array outer size and first dimension must agree");
  rejected([](lir::LirModule& m) { m.globals[1].type.inner_rank = 2; },
           "mixed outer-array and inner-array pointer shapes remain closed");
  rejected([](lir::LirModule& m) { m.globals[0].type.is_ptr_to_array = false; },
           "ordinary pointers must reject residual array shape");
  rejected([](lir::LirModule& m) { m.globals[0].type.is_fn_ptr = true; },
           "function-pointer-to-array shapes remain closed");
  rejected([](lir::LirModule& m) { m.globals[0].type.is_lvalue_ref = true; },
           "reference pointer-to-array shapes remain closed");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.is_vector = true;
        m.globals[0].type.vector_lanes = 2;
        m.globals[0].type.vector_bytes = 32;
      },
      "vector pointer-to-array shapes remain closed");
  rejected([](lir::LirModule& m) { m.globals[0].type.base = c4c::TB_STRUCT; },
           "aggregate pointer-to-array pointees remain closed");
  rejected([](lir::LirModule& m) { m.globals[0].type.base = c4c::TB_VA_LIST; },
           "va-list pointer-to-array pointees remain closed");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].llvm_type_ref = lir::LirTypeRef("ptr");
      },
      "producer-shaped pointer-to-array globals reject unexpected mirrors");
  rejected([](lir::LirModule& m) { m.globals[0].llvm_type = "[4 x ptr]"; },
           "pointer-to-array storage spelling remains opaque ptr");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.array_size_expr =
            reinterpret_cast<c4c::Node*>(static_cast<std::uintptr_t>(1));
      },
      "computed pointer-to-array bounds remain closed");
}

void test_fixed_scalar_base_array_global_receipt_and_rejections() {
  const auto valid_module = [] {
    lir::LirModule module;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    module.struct_names.attach_text_table(module.link_name_texts.get());
    const auto array_link = module.link_names.intern("fixed_scalar_array");
    const auto init_a = module.link_names.intern("array_init_a");
    const auto init_b = module.link_names.intern("array_init_b");

    lir::LirGlobal definition;
    definition.name = "fixed_scalar_array";
    definition.link_name_id = array_link;
    definition.type = scalar_type(c4c::TB_SHORT);
    definition.type.array_rank = 1;
    definition.type.array_size = 0;
    definition.type.array_dims[0] = 0;
    definition.is_const = true;
    definition.linkage_vis = "weak protected ";
    definition.qualifier = "constant ";
    definition.llvm_type = "[0 x i16]";
    definition.init_text = std::string{"opaque\0array-payload", 20};
    definition.initializer_function_link_name_ids = {init_b, init_a, init_b};
    definition.align_bytes = 16;
    module.globals.push_back(std::move(definition));

    lir::LirGlobal declaration;
    declaration.name = "extern_scalar_array";
    declaration.type = scalar_type(c4c::TB_DOUBLE);
    declaration.type.array_rank = 1;
    declaration.type.array_size = 3;
    declaration.type.array_dims[0] = 3;
    declaration.linkage_vis = "external hidden ";
    declaration.qualifier = "global ";
    declaration.llvm_type = "[3 x double]";
    declaration.align_bytes = 8;
    declaration.is_extern_decl = true;
    module.globals.push_back(std::move(declaration));

    lir::LirGlobal multidimensional;
    multidimensional.name = "extern_multidimensional_scalar_array";
    multidimensional.type = scalar_type(c4c::TB_UINT);
    multidimensional.type.array_rank = 3;
    multidimensional.type.array_size = 2;
    multidimensional.type.array_dims[0] = 2;
    multidimensional.type.array_dims[1] = 3;
    multidimensional.type.array_dims[2] = 7;
    multidimensional.linkage_vis = "external protected ";
    multidimensional.qualifier = "global ";
    multidimensional.llvm_type = "[2 x [3 x [7 x i32]]]";
    multidimensional.align_bytes = 32;
    multidimensional.is_extern_decl = true;
    module.globals.push_back(std::move(multidimensional));

    lir::LirGlobal pointer_elements;
    pointer_elements.name = "extern_multidimensional_pointer_array";
    pointer_elements.type = scalar_type(c4c::TB_FLOAT);
    pointer_elements.type.ptr_level = 1;
    pointer_elements.type.array_rank = 2;
    pointer_elements.type.array_size = 4;
    pointer_elements.type.array_dims[0] = 4;
    pointer_elements.type.array_dims[1] = 0;
    pointer_elements.linkage_vis = "external hidden ";
    pointer_elements.qualifier = "global ";
    pointer_elements.llvm_type = "[4 x [0 x ptr]]";
    pointer_elements.align_bytes = 16;
    pointer_elements.is_extern_decl = true;
    module.globals.push_back(std::move(pointer_elements));

    lir::LirGlobal deep_pointer_elements;
    deep_pointer_elements.name = "extern_deep_pointer_array";
    deep_pointer_elements.type = scalar_type(c4c::TB_LONGLONG);
    deep_pointer_elements.type.ptr_level = 3;
    deep_pointer_elements.type.array_rank = 2;
    deep_pointer_elements.type.array_size = 6;
    deep_pointer_elements.type.array_dims[0] = 6;
    deep_pointer_elements.type.array_dims[1] = 3;
    deep_pointer_elements.linkage_vis = "external protected ";
    deep_pointer_elements.qualifier = "global ";
    deep_pointer_elements.llvm_type = "[6 x [3 x ptr]]";
    deep_pointer_elements.align_bytes = 64;
    deep_pointer_elements.is_extern_decl = true;
    module.globals.push_back(std::move(deep_pointer_elements));
    return module;
  };

  auto module = valid_module();
  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(),
         "producer-shaped fixed scalar-base array definitions and declarations must import");
  expect(bir::FoundationVerifier::verify(imported.value()).ok(),
         "fixed scalar-base array global storage must be verifier reachable");
  const auto view = imported.value().view();
  const auto ids = view.global_objects();
  expect(ids.size() == 5 && ids[0].slot == 0 && ids[1].slot == 1 &&
             ids[2].slot == 2 && ids[3].slot == 3 && ids[4].slot == 4,
         "fixed scalar-base arrays must preserve deterministic source order");
  const auto definition = view.global_object(ids[0]).value();
  const auto declaration = view.global_object(ids[1]).value();
  const auto multidimensional = view.global_object(ids[2]).value();
  const auto pointer_elements = view.global_object(ids[3]).value();
  const auto deep_pointer_elements = view.global_object(ids[4]).value();
  expect(definition.object_type.kind == bir::TypeKind::Array &&
             definition.object_type.bit_width == 0 &&
             definition.object_type.spelling == "[0 x i16]" &&
             definition.object_type.array_facts ==
                 std::optional<bir::ArrayTypeFacts>{bir::ArrayTypeFacts{
                     bir::TypeKind::Integer, 16, 0, {0}}} &&
             std::holds_alternative<bir::LinkNameId>(definition.identity) &&
             !definition.is_internal && definition.is_weak &&
             definition.is_const &&
             definition.visibility == bir::SymbolVisibility::Protected &&
             definition.alignment == 16 &&
             !definition.is_extern_declaration && definition.initializer &&
             definition.initializer->opaque_payload ==
                 std::string{"opaque\0array-payload", 20} &&
             definition.initializer->function_links.size() == 3 &&
             view.spelling(definition.initializer->function_links[0]).value() ==
                 "array_init_b" &&
             view.spelling(definition.initializer->function_links[1]).value() ==
                 "array_init_a" &&
             definition.initializer->function_links[2] ==
                 definition.initializer->function_links[0],
         "fixed scalar array definitions must preserve exact typed shape and all object facts");
  expect(declaration.object_type.kind == bir::TypeKind::Array &&
             declaration.object_type.spelling == "[3 x double]" &&
             declaration.object_type.array_facts ==
                 std::optional<bir::ArrayTypeFacts>{bir::ArrayTypeFacts{
                     bir::TypeKind::Floating, 64, 0, {3}}} &&
             declaration.is_extern_declaration &&
             declaration.visibility == bir::SymbolVisibility::Hidden &&
             !declaration.initializer,
         "extern fixed scalar arrays must use the same typed array path");
  expect(multidimensional.object_type.kind == bir::TypeKind::Array &&
             multidimensional.object_type.spelling ==
                 "[2 x [3 x [7 x i32]]]" &&
             multidimensional.object_type.array_facts ==
                 std::optional<bir::ArrayTypeFacts>{bir::ArrayTypeFacts{
                     bir::TypeKind::Integer, 32, 0, {2, 3, 7}}} &&
             multidimensional.is_extern_declaration &&
             multidimensional.visibility ==
                 bir::SymbolVisibility::Protected &&
             multidimensional.alignment == 32 &&
             !multidimensional.initializer,
         "multidimensional scalar arrays must retain outer-to-inner typed dimensions and object facts");
  expect(pointer_elements.object_type.kind == bir::TypeKind::Array &&
             pointer_elements.object_type.bit_width == 0 &&
             pointer_elements.object_type.spelling == "[4 x [0 x ptr]]" &&
             pointer_elements.object_type.array_facts ==
                 std::optional<bir::ArrayTypeFacts>{bir::ArrayTypeFacts{
                     bir::TypeKind::Floating, 32, 1, {4, 0}}} &&
             std::holds_alternative<bir::FallbackGlobalName>(
                 pointer_elements.identity) &&
             std::get<bir::FallbackGlobalName>(pointer_elements.identity).name ==
                 "extern_multidimensional_pointer_array" &&
             pointer_elements.is_extern_declaration &&
             pointer_elements.visibility == bir::SymbolVisibility::Hidden &&
             pointer_elements.alignment == 16 &&
             !pointer_elements.initializer,
         "pointer-element arrays must retain scalar pointee facts, pointer depth, ordered dimensions, and object facts");
  expect(deep_pointer_elements.object_type.kind == bir::TypeKind::Array &&
             deep_pointer_elements.object_type.bit_width == 0 &&
             deep_pointer_elements.object_type.spelling ==
                 "[6 x [3 x ptr]]" &&
             deep_pointer_elements.object_type.array_facts ==
                 std::optional<bir::ArrayTypeFacts>{bir::ArrayTypeFacts{
                     bir::TypeKind::Integer, 64, 3, {6, 3}}} &&
             std::holds_alternative<bir::FallbackGlobalName>(
                 deep_pointer_elements.identity) &&
             deep_pointer_elements.is_extern_declaration &&
             deep_pointer_elements.visibility ==
                 bir::SymbolVisibility::Protected &&
             deep_pointer_elements.alignment == 64 &&
             !deep_pointer_elements.initializer,
         "deep pointer-element arrays must retain exact scalar base, width, pointer depth, dimensions, opaque ptr parity, and object facts");
  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  const auto canonical_array_ids =
      canonical.has_value() ? canonical.value().view().global_objects()
                            : std::vector<bir::GlobalObjectId>{};
  expect(canonical.has_value() && canonical_array_ids.size() == 5 &&
             canonical.value()
                     .view()
                     .global_object(canonical_array_ids[0])
                     .value()
                     .object_type == definition.object_type &&
             canonical.value()
                     .view()
                     .global_object(canonical_array_ids[3])
                     .value()
                     .object_type == pointer_elements.object_type &&
             canonical.value()
                     .view()
                     .global_object(canonical_array_ids[4])
                     .value()
                     .object_type.array_facts ==
                 std::optional<bir::ArrayTypeFacts>{bir::ArrayTypeFacts{
                     bir::TypeKind::Integer, 64, 3, {6, 3}}},
         "producer-shaped fixed scalar-base arrays must publish Canonical BIR");

  const auto rejected = [&](auto mutate, const std::string& message) {
    auto candidate = valid_module();
    mutate(candidate);
    const auto raw = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw.has_value() &&
               raw.error().code == bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Raw rollback)");
    const auto rejected_canonical =
        bir::lower_lir_to_canonical_bir(candidate);
    expect(!rejected_canonical.has_value() &&
               rejected_canonical.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Canonical rollback)");
  };
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.array_size = -1;
        m.globals[0].type.array_dims[0] = -1;
        m.globals[0].llvm_type = "[-1 x i16]";
      },
      "negative outer array dimensions must remain unsupported");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.array_size = -2;
        m.globals[0].type.array_dims[0] = -2;
        m.globals[0].llvm_type = "[-2 x i16]";
      },
      "unsized outer array sentinels must remain unsupported");
  rejected(
      [](lir::LirModule& m) { m.globals[0].type.array_dims[0] = 6; },
      "array_size and outer dimension must agree");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.array_rank = 9;
      },
      "array rank must fit the producer dimension vector capacity");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.array_rank = 2;
        m.globals[0].type.array_dims[1] = -2;
        m.globals[0].llvm_type = "[0 x [-2 x i16]]";
      },
      "negative inner dimensions must remain unsupported");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.array_rank = 2;
        m.globals[0].type.array_dims[1] = -1;
        m.globals[0].llvm_type = "[0 x [-1 x i16]]";
      },
      "unsized inner array sentinels must remain unsupported");
  rejected(
      [](lir::LirModule& m) {
        m.globals[3].type.ptr_level = -1;
      },
      "negative pointer-element array depth must remain unsupported");
  rejected(
      [](lir::LirModule& m) {
        m.globals[3].type.base = c4c::TB_STRUCT;
      },
      "aggregate pointees in pointer-element arrays must remain unsupported");
  rejected(
      [](lir::LirModule& m) {
        m.globals[3].type.is_ptr_to_array = true;
        m.globals[3].type.inner_rank = 1;
      },
      "pointer-to-array declarators must not be confused with arrays of pointers");
  rejected(
      [](lir::LirModule& m) { m.globals[3].type.inner_rank = 1; },
      "inner array rank without pointer-to-array authority must remain unsupported");
  rejected(
      [](lir::LirModule& m) { m.globals[3].type.is_fn_ptr = true; },
      "function-pointer element arrays must remain unsupported");
  rejected(
      [](lir::LirModule& m) {
        m.globals[3].llvm_type_ref = lir::LirTypeRef("[4 x [0 x ptr]]");
      },
      "producer-valid pointer-element arrays must not carry llvm_type_ref");
  rejected(
      [](lir::LirModule& m) {
        m.globals[3].llvm_type = "[4 x [0 x i32]]";
      },
      "pointer-element array LLVM spelling remains parity-only and must match opaque ptr nesting");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.base = c4c::TB_STRUCT;
        m.globals[0].llvm_type = "[0 x %struct.Payload]";
      },
      "aggregate element arrays remain outside this packet");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].llvm_type_ref = lir::LirTypeRef("[0 x i16]");
      },
      "producer-valid fixed scalar arrays must not carry llvm_type_ref");
  rejected(
      [](lir::LirModule& m) {
        m.globals[2].llvm_type = "[2 x [7 x [3 x i32]]]";
      },
      "nested rendered array spelling must match ordered typed dimensions");
}

void test_direct_vector_global_receipt_and_rejections() {
  const auto valid_module = [] {
    lir::LirModule module;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    module.struct_names.attach_text_table(module.link_name_texts.get());
    const auto vector_link = module.link_names.intern("integer_vector_global");
    const auto init_a = module.link_names.intern("vector_init_a");
    const auto init_b = module.link_names.intern("vector_init_b");

    lir::LirGlobal definition;
    definition.name = "integer_vector_global";
    definition.link_name_id = vector_link;
    definition.type = scalar_type(c4c::TB_SHORT);
    definition.type.is_vector = true;
    definition.type.vector_lanes = 4;
    definition.type.vector_bytes = 8;
    definition.linkage_vis = "protected ";
    definition.qualifier = "global ";
    definition.llvm_type = "<4 x i16>";
    definition.init_text =
        std::string{"<i16 1, i16 2, i16 3, i16 4>\0tail", 33};
    definition.initializer_function_link_name_ids = {init_b, init_a, init_b};
    definition.align_bytes = 8;
    module.globals.push_back(std::move(definition));

    lir::LirGlobal declaration;
    declaration.name = "floating_vector_extern";
    declaration.type = scalar_type(c4c::TB_DOUBLE);
    declaration.type.is_vector = true;
    declaration.type.vector_lanes = 2;
    declaration.type.vector_bytes = 16;
    declaration.linkage_vis = "external hidden ";
    declaration.qualifier = "global ";
    declaration.llvm_type = "<2 x double>";
    declaration.align_bytes = 16;
    declaration.is_extern_decl = true;
    module.globals.push_back(std::move(declaration));
    return module;
  };

  auto module = valid_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value(),
         "producer-shaped direct integer and floating vector globals must import");
  expect(bir::FoundationVerifier::verify(raw.value()).ok(),
         "typed direct vector globals must remain Foundation-verifier reachable");
  const auto view = raw.value().view();
  const auto ids = view.global_objects();
  expect(ids.size() == 2 && ids[0].slot == 0 && ids[1].slot == 1,
         "direct vector globals must preserve source order and stable identity");
  const auto definition = view.global_object(ids[0]).value();
  const auto declaration = view.global_object(ids[1]).value();
  expect(definition.object_type.kind == bir::TypeKind::Vector &&
             definition.object_type.spelling == "<4 x i16>" &&
             definition.object_type.vector_facts ==
                 std::optional<bir::VectorTypeFacts>{bir::VectorTypeFacts{
                     bir::TypeKind::Integer, 16, 4, 8}} &&
             std::holds_alternative<bir::LinkNameId>(definition.identity) &&
             !definition.is_internal && !definition.is_weak &&
             !definition.is_const &&
             definition.visibility == bir::SymbolVisibility::Protected &&
             definition.alignment == 8 &&
             !definition.is_extern_declaration && definition.initializer &&
             definition.initializer->opaque_payload ==
                 std::string{"<i16 1, i16 2, i16 3, i16 4>\0tail", 33} &&
             definition.initializer->function_links.size() == 3 &&
             view.spelling(definition.initializer->function_links[0]).value() ==
                 "vector_init_b" &&
             view.spelling(definition.initializer->function_links[1]).value() ==
                 "vector_init_a" &&
             definition.initializer->function_links[2] ==
                 definition.initializer->function_links[0],
         "integer vector definitions must preserve exact typed authority and all object and initializer facts");
  expect(declaration.object_type.kind == bir::TypeKind::Vector &&
             declaration.object_type.spelling == "<2 x double>" &&
             declaration.object_type.vector_facts ==
                 std::optional<bir::VectorTypeFacts>{bir::VectorTypeFacts{
                     bir::TypeKind::Floating, 64, 2, 16}} &&
             std::holds_alternative<bir::FallbackGlobalName>(
                 declaration.identity) &&
             declaration.visibility == bir::SymbolVisibility::Hidden &&
             declaration.alignment == 16 &&
             declaration.is_extern_declaration && !declaration.initializer,
         "floating vector externs must preserve typed element, lanes, storage, linkage, visibility, and alignment");

  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(),
         "producer-shaped direct vector globals must publish Canonical BIR");
  const auto canonical_view = canonical.value().view();
  const auto canonical_ids = canonical_view.global_objects();
  expect(canonical_ids.size() == 2 &&
             canonical_view.global_object(canonical_ids[0])
                     .value()
                     .object_type.vector_facts ==
                 definition.object_type.vector_facts &&
             canonical_view.global_object(canonical_ids[0])
                     .value()
                     .initializer->opaque_payload ==
                 definition.initializer->opaque_payload &&
             canonical_view.global_object(canonical_ids[1])
                     .value()
                     .object_type.vector_facts ==
                 declaration.object_type.vector_facts &&
             canonical_view.global_object(canonical_ids[1])
                 .value()
                 .is_extern_declaration,
         "Canonical BIR must retain ordered direct vector authority and object facts");

  const auto rejected = [&](auto mutate, const std::string& message) {
    auto candidate = valid_module();
    mutate(candidate);
    const auto rejected_raw = bir::lower_lir_to_raw_bir(candidate);
    expect(!rejected_raw.has_value() &&
               rejected_raw.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Raw rollback)");
    const auto rejected_canonical =
        bir::lower_lir_to_canonical_bir(candidate);
    expect(!rejected_canonical.has_value() &&
               rejected_canonical.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Canonical rollback)");
  };
  rejected([](lir::LirModule& m) { m.globals[0].type.vector_lanes = 0; },
           "zero vector lane counts must reject transactionally");
  rejected([](lir::LirModule& m) { m.globals[0].type.vector_lanes = -4; },
           "negative vector lane counts must reject transactionally");
  rejected([](lir::LirModule& m) { m.globals[0].type.vector_bytes = 0; },
           "zero vector storage bytes must reject transactionally");
  rejected([](lir::LirModule& m) { m.globals[0].type.vector_bytes = -8; },
           "negative vector storage bytes must reject transactionally");
  rejected([](lir::LirModule& m) { m.globals[0].llvm_type = "<8 x i16>"; },
           "vector spelling must exactly corroborate typed lane and element facts");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].llvm_type_ref = lir::LirTypeRef("<4 x i16>");
      },
      "producer-valid direct vectors must not carry llvm_type_ref");
  rejected([](lir::LirModule& m) { m.globals[0].type.vrm_width = 2; },
           "VRM metadata must remain excluded from direct vector globals");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.is_vector = false;
        m.globals[0].llvm_type = "i16";
      },
      "residual vector lanes and storage must not fall through as scalar authority");
  rejected([](lir::LirModule& m) { m.globals[0].type.ptr_level = 1; },
           "pointer-to-vector shapes remain outside direct vector globals");
  rejected(
      [](lir::LirModule& m) { m.globals[0].type.is_lvalue_ref = true; },
      "vector reference shapes remain unsupported");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.array_rank = 1;
        m.globals[0].type.array_size = 2;
        m.globals[0].type.array_dims[0] = 2;
      },
      "array-of-vector shapes remain outside direct vector globals");
  rejected([](lir::LirModule& m) { m.globals[0].type.is_fn_ptr = true; },
           "function-pointer vector shapes remain unsupported");
  rejected(
      [](lir::LirModule& m) { m.globals[0].type.base = c4c::TB_STRUCT; },
      "aggregate vector bases remain unsupported");
}

void test_named_aggregate_global_receipt_and_rejections() {
  const auto valid_module = [] {
    lir::LirModule module;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    module.struct_names.attach_text_table(module.link_name_texts.get());

    const auto pair_id = module.struct_names.intern("%struct.PairGlobal");
    const auto choice_id = module.struct_names.intern("%struct.ChoiceGlobal");
    module.record_struct_decl(lir::LirStructDecl{
        pair_id,
        {{lir::LirTypeRef::integer(32)}, {lir::LirTypeRef("double")}}});
    module.record_struct_decl(lir::LirStructDecl{
        choice_id,
        {{lir::LirTypeRef::integer(64)}, {lir::LirTypeRef("double")}}});

    const auto pair_link = module.link_names.intern("named_pair_global");
    const auto extern_pair_link =
        module.link_names.intern("extern_named_pair_global");
    const auto init_a = module.link_names.intern("aggregate_init_a");
    const auto init_b = module.link_names.intern("aggregate_init_b");

    lir::LirGlobal pair;
    pair.name = "named_pair_global";
    pair.link_name_id = pair_link;
    pair.type = scalar_type(c4c::TB_STRUCT);
    pair.linkage_vis = "protected ";
    pair.qualifier = "global ";
    pair.llvm_type = "%struct.PairGlobal";
    pair.llvm_type_ref =
        lir::LirTypeRef::struct_type(pair.llvm_type, pair_id);
    pair.init_text = std::string{"{ i32 7, double 2.5 }\0tail", 26};
    pair.initializer_function_link_name_ids = {init_b, init_a, init_b};
    pair.align_bytes = 8;
    module.globals.push_back(std::move(pair));

    lir::LirGlobal choice;
    choice.name = "named_choice_global";
    choice.type = scalar_type(c4c::TB_UNION);
    choice.is_internal = true;
    choice.is_const = true;
    choice.linkage_vis = "internal hidden ";
    choice.qualifier = "constant ";
    choice.llvm_type = "%struct.ChoiceGlobal";
    choice.llvm_type_ref =
        lir::LirTypeRef::union_type(choice.llvm_type, choice_id);
    choice.init_text = "{ i64 11 }";
    choice.initializer_function_link_name_ids = {init_a, init_b};
    choice.align_bytes = 16;
    module.globals.push_back(std::move(choice));

    lir::LirGlobal extern_pair;
    extern_pair.name = "extern_named_pair_global";
    extern_pair.link_name_id = extern_pair_link;
    extern_pair.type = scalar_type(c4c::TB_STRUCT);
    extern_pair.linkage_vis = "external hidden ";
    extern_pair.qualifier = "global ";
    extern_pair.llvm_type = "%struct.PairGlobal";
    extern_pair.llvm_type_ref =
        lir::LirTypeRef::struct_type(extern_pair.llvm_type, pair_id);
    extern_pair.align_bytes = 32;
    extern_pair.is_extern_decl = true;
    module.globals.push_back(std::move(extern_pair));

    lir::LirGlobal weak_extern_choice;
    weak_extern_choice.name = "weak_extern_named_choice_global";
    weak_extern_choice.type = scalar_type(c4c::TB_UNION);
    weak_extern_choice.linkage_vis = "extern_weak protected ";
    weak_extern_choice.qualifier = "global ";
    weak_extern_choice.llvm_type = "%struct.ChoiceGlobal";
    weak_extern_choice.llvm_type_ref = lir::LirTypeRef::union_type(
        weak_extern_choice.llvm_type, choice_id);
    weak_extern_choice.align_bytes = 64;
    weak_extern_choice.is_extern_decl = true;
    module.globals.push_back(std::move(weak_extern_choice));
    return module;
  };

  auto module = valid_module();
  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(),
         "producer-shaped named struct and union definitions must import");
  expect(bir::FoundationVerifier::verify(imported.value()).ok(),
         "named aggregate global storage must be verifier reachable");
  const auto view = imported.value().view();
  const auto ids = view.global_objects();
  expect(ids.size() == 4,
         "named aggregate globals must preserve deterministic source order");
  const auto pair = view.global_object(ids[0]).value();
  const auto choice = view.global_object(ids[1]).value();
  const auto extern_pair = view.global_object(ids[2]).value();
  const auto weak_extern_choice = view.global_object(ids[3]).value();
  expect(pair.object_type.kind == bir::TypeKind::Struct &&
             pair.object_type.struct_name_id ==
                 module.globals[0].llvm_type_ref->struct_name_id() &&
             pair.object_type.spelling == "%struct.PairGlobal" &&
             std::holds_alternative<bir::LinkNameId>(pair.identity) &&
             !pair.is_internal && !pair.is_weak && !pair.is_const &&
             pair.visibility == bir::SymbolVisibility::Protected &&
             pair.alignment == 8 && !pair.is_extern_declaration &&
             pair.initializer &&
             pair.initializer->opaque_payload ==
                 std::string{"{ i32 7, double 2.5 }\0tail", 26} &&
             pair.initializer->function_links.size() == 3 &&
             view.spelling(pair.initializer->function_links[0]).value() ==
                 "aggregate_init_b" &&
             view.spelling(pair.initializer->function_links[1]).value() ==
                 "aggregate_init_a" &&
             pair.initializer->function_links[2] ==
                 pair.initializer->function_links[0],
         "named struct globals must preserve semantic type identity and all object facts");
  expect(choice.object_type.kind == bir::TypeKind::Struct &&
             choice.object_type.struct_name_id ==
                 module.globals[1].llvm_type_ref->struct_name_id() &&
             choice.object_type.spelling == "%struct.ChoiceGlobal" &&
             std::holds_alternative<bir::FallbackGlobalName>(choice.identity) &&
             choice.is_internal && !choice.is_weak && choice.is_const &&
             choice.visibility == bir::SymbolVisibility::Hidden &&
             choice.alignment == 16 && !choice.is_extern_declaration &&
             choice.initializer &&
             choice.initializer->opaque_payload == "{ i64 11 }" &&
             choice.initializer->function_links.size() == 2 &&
             view.spelling(choice.initializer->function_links[0]).value() ==
                 "aggregate_init_a" &&
             view.spelling(choice.initializer->function_links[1]).value() ==
                 "aggregate_init_b",
         "named union globals must preserve Struct identity, const linkage facts, and ordered initializer links");
  expect(extern_pair.object_type.kind == bir::TypeKind::Struct &&
             extern_pair.object_type.struct_name_id ==
                 module.globals[2].llvm_type_ref->struct_name_id() &&
             extern_pair.object_type.spelling == "%struct.PairGlobal" &&
             std::holds_alternative<bir::LinkNameId>(extern_pair.identity) &&
             !extern_pair.is_internal && !extern_pair.is_weak &&
             !extern_pair.is_const &&
             extern_pair.visibility == bir::SymbolVisibility::Hidden &&
             extern_pair.alignment == 32 &&
             extern_pair.is_extern_declaration && !extern_pair.initializer,
         "named struct externs must preserve link-backed identity, structured authority, visibility, alignment, and no initializer");
  expect(weak_extern_choice.object_type.kind == bir::TypeKind::Struct &&
             weak_extern_choice.object_type.struct_name_id ==
                 module.globals[3].llvm_type_ref->struct_name_id() &&
             weak_extern_choice.object_type.spelling ==
                 "%struct.ChoiceGlobal" &&
             std::holds_alternative<bir::FallbackGlobalName>(
                 weak_extern_choice.identity) &&
             std::get<bir::FallbackGlobalName>(weak_extern_choice.identity)
                     .name == "weak_extern_named_choice_global" &&
             !weak_extern_choice.is_internal && weak_extern_choice.is_weak &&
             !weak_extern_choice.is_const &&
             weak_extern_choice.visibility ==
                 bir::SymbolVisibility::Protected &&
             weak_extern_choice.alignment == 64 &&
             weak_extern_choice.is_extern_declaration &&
             !weak_extern_choice.initializer,
         "named union weak externs must preserve fallback identity, structured authority, visibility, alignment, and no initializer");

  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(),
         "producer-shaped named aggregate definitions and externs must publish verified Canonical BIR");
  const auto canonical_view = canonical.value().view();
  const auto canonical_ids = canonical_view.global_objects();
  expect(canonical_ids.size() == 4 &&
             canonical_view.global_object(canonical_ids[2])
                     .value()
                     .object_type.struct_name_id ==
                 module.globals[2].llvm_type_ref->struct_name_id() &&
             canonical_view.global_object(canonical_ids[2])
                 .value()
                 .is_extern_declaration &&
             !canonical_view.global_object(canonical_ids[2])
                  .value()
                  .initializer &&
             canonical_view.global_object(canonical_ids[3])
                     .value()
                     .object_type.struct_name_id ==
                 module.globals[3].llvm_type_ref->struct_name_id() &&
             canonical_view.global_object(canonical_ids[3]).value().is_weak &&
             canonical_view.global_object(canonical_ids[3])
                 .value()
                 .is_extern_declaration &&
             !canonical_view.global_object(canonical_ids[3])
                  .value()
                  .initializer,
         "Canonical BIR must retain ordered named aggregate extern authority and declaration facts");

  const auto rejected = [&](auto mutate, const std::string& message) {
    auto rejected_module = valid_module();
    mutate(rejected_module);
    const auto raw = bir::lower_lir_to_raw_bir(rejected_module);
    expect(!raw.has_value() &&
               raw.error().code == bir::ImportErrorCode::UnsupportedGlobals,
           message);
    const auto canonical = bir::lower_lir_to_canonical_bir(rejected_module);
    expect(!canonical.has_value() &&
               canonical.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (canonical rollback)");
  };
  rejected([](lir::LirModule& m) { m.globals[2].llvm_type_ref.reset(); },
           "named aggregate externs require structured type identity transactionally");
  rejected(
      [](lir::LirModule& m) {
        m.globals[2].llvm_type_ref = lir::LirTypeRef::struct_type(
            "%struct.UnresolvedGlobal", static_cast<c4c::StructNameId>(999));
        m.globals[2].llvm_type = "%struct.UnresolvedGlobal";
      },
      "unresolved aggregate extern StructNameId authority must reject transactionally");
  rejected(
      [](lir::LirModule& m) {
        const auto choice_id = m.globals[1].llvm_type_ref->struct_name_id();
        m.globals[2].llvm_type_ref =
            lir::LirTypeRef::struct_type("%struct.PairGlobal", choice_id);
      },
      "mismatched aggregate extern spelling and StructNameId must reject transactionally");
  rejected(
      [](lir::LirModule& m) {
        m.globals[2].llvm_type = "%struct.ChoiceGlobal";
      },
      "aggregate extern rendered spelling must match its structured authority transactionally");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.base = c4c::TB_UNION;
        m.globals[0].llvm_type = "{ i32, [3 x i8] }";
        m.globals[0].llvm_type_ref = lir::LirTypeRef(m.globals[0].llvm_type);
      },
      "literal union globals must remain closed transactionally");
  rejected(
      [](lir::LirModule& m) {
        m.globals[2].type.base = c4c::TB_STRUCT;
        m.globals[2].llvm_type = "{ i32, [3 x i8] }";
        m.globals[2].llvm_type_ref =
            lir::LirTypeRef(m.globals[2].llvm_type);
      },
      "unkeyed literal aggregate extern declarations must remain closed transactionally");
  rejected([](lir::LirModule& m) { m.globals[2].init_text = "zeroinitializer"; },
           "named aggregate extern declarations cannot carry initializer payloads");
  rejected(
      [](lir::LirModule& m) {
        m.globals[2].initializer_function_link_name_ids = {
            m.globals[0].initializer_function_link_name_ids[0]};
      },
      "named aggregate extern declarations cannot carry initializer links");
  rejected(
      [](lir::LirModule& m) {
        m.globals[2].is_const = true;
        m.globals[2].qualifier = "constant ";
      },
      "named aggregate extern declarations require the producer global qualifier");
  rejected([](lir::LirModule& m) { m.globals[2].is_internal = true; },
           "named aggregate extern declarations cannot contradict external linkage with internal flags");
  rejected(
      [](lir::LirModule& m) {
        m.globals[2].type.ptr_level = 1;
        m.globals[2].llvm_type = "ptr";
        m.globals[2].llvm_type_ref = lir::LirTypeRef("ptr");
      },
      "pointer-to-aggregate extern shapes must remain closed transactionally");
  rejected(
      [](lir::LirModule& m) {
        m.globals[2].type.array_rank = 1;
        m.globals[2].type.array_size = 1;
        m.globals[2].type.array_dims[0] = 1;
        m.globals[2].llvm_type = "[1 x %struct.PairGlobal]";
        m.globals[2].llvm_type_ref = lir::LirTypeRef(m.globals[2].llvm_type);
      },
      "array aggregate extern shapes must remain closed transactionally");
}

void test_flexible_member_literal_struct_global_receipt_and_rejections() {
  const auto valid_module = [] {
    lir::LirModule module;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    module.struct_names.attach_text_table(module.link_name_texts.get());

    const auto object_link =
        module.link_names.intern("flexible_member_global");
    const auto init_a = module.link_names.intern("flexible_init_a");
    const auto init_b = module.link_names.intern("flexible_init_b");

    lir::LirGlobal global;
    global.name = "flexible_member_global";
    global.link_name_id = object_link;
    global.type = scalar_type(c4c::TB_STRUCT);
    global.is_const = true;
    global.linkage_vis = "weak protected ";
    global.qualifier = "constant ";
    global.llvm_type = "{ i32, [5 x i8] }";
    global.llvm_type_ref = lir::LirTypeRef(global.llvm_type);
    global.init_text = std::string{"opaque\0flexible-payload", 23};
    global.initializer_function_link_name_ids = {init_b, init_a, init_b};
    global.align_bytes = 16;
    module.globals.push_back(std::move(global));
    return module;
  };

  auto module = valid_module();
  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(),
         "producer-shaped flexible-member literal struct globals must import");
  expect(bir::FoundationVerifier::verify(imported.value()).ok(),
         "literal struct global storage must be verifier reachable");
  const auto view = imported.value().view();
  const auto ids = view.global_objects();
  expect(ids.size() == 1 && ids[0].slot == 0,
         "literal struct globals must preserve deterministic source order");
  const auto global = view.global_object(ids[0]).value();
  expect(global.object_type.kind == bir::TypeKind::Struct &&
             global.object_type.struct_name_id == c4c::kInvalidStructName &&
             global.object_type.spelling == "{ i32, [5 x i8] }" &&
             std::holds_alternative<bir::LinkNameId>(global.identity) &&
             std::get<bir::LinkNameId>(global.identity).slot == 0 &&
             !global.is_internal && global.is_weak && global.is_const &&
             global.visibility == bir::SymbolVisibility::Protected &&
             global.alignment == 16 && !global.is_extern_declaration &&
             global.initializer &&
             global.initializer->opaque_payload ==
                 std::string{"opaque\0flexible-payload", 23} &&
             global.initializer->function_links.size() == 3 &&
             view.spelling(global.initializer->function_links[0]).value() ==
                 "flexible_init_b" &&
             view.spelling(global.initializer->function_links[1]).value() ==
                 "flexible_init_a" &&
             global.initializer->function_links[2] ==
                 global.initializer->function_links[0],
         "literal struct globals must preserve byte-exact typed spelling and all object facts");

  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value() &&
             canonical.value().view().global_objects().size() == 1,
         "producer-shaped literal struct globals must publish Canonical BIR");

  const auto rejected = [&](auto mutate, const std::string& message) {
    auto candidate = valid_module();
    mutate(candidate);
    const auto raw = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw.has_value() &&
               raw.error().code == bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Raw rollback)");
    const auto rejected_canonical =
        bir::lower_lir_to_canonical_bir(candidate);
    expect(!rejected_canonical.has_value() &&
               rejected_canonical.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Canonical rollback)");
  };
  rejected([](lir::LirModule& m) { m.globals[0].type.base = c4c::TB_UNION; },
           "a literal union mirror must remain unsupported");
  rejected([](lir::LirModule& m) { m.globals[0].llvm_type_ref.reset(); },
           "literal struct globals require their typed mirror");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].llvm_type = "{ i32, [6 x i8] }";
      },
      "literal struct compatibility spelling must match its typed mirror");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].is_extern_decl = true;
        m.globals[0].linkage_vis = "external protected ";
        m.globals[0].init_text.clear();
        m.globals[0].initializer_function_link_name_ids.clear();
      },
      "literal aggregate extern declarations must remain unsupported");
  rejected([](lir::LirModule& m) { m.globals[0].type.array_rank = 1; },
           "TypeSpec array aggregate globals must remain unsupported");
  rejected([](lir::LirModule& m) { m.globals[0].type.ptr_level = 1; },
           "pointer-to-aggregate globals must remain unsupported");
}

void test_global_object_rejections_and_transactionality() {
  const auto valid_module = [] {
    lir::LirModule module;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    module.struct_names.attach_text_table(module.link_name_texts.get());
    const auto linked = module.link_names.intern("linked_global");
    lir::LirGlobal global;
    global.name = "linked_global";
    global.link_name_id = linked;
    global.type = scalar_type(c4c::TB_INT);
    global.linkage_vis = "external ";
    global.qualifier = "global ";
    global.llvm_type = "i32";
    global.llvm_type_ref = lir::LirTypeRef::integer(32);
    global.align_bytes = 4;
    global.is_extern_decl = true;
    module.globals.push_back(global);
    module.functions.push_back(void_declaration("valid_before_bad_global"));
    return module;
  };
  const auto rejected = [&](auto mutate, const std::string& message) {
    auto module = valid_module();
    mutate(module);
    auto imported = bir::lower_lir_to_raw_bir(module);
    expect(!imported.has_value() &&
               imported.error().code == bir::ImportErrorCode::UnsupportedGlobals,
           message);
    auto canonical = bir::lower_lir_to_canonical_bir(module);
    expect(!canonical.has_value() &&
               canonical.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (canonical rollback)");
  };

  rejected([](lir::LirModule& m) { m.globals[0].name.clear(); },
           "empty global names must reject the whole module");
  rejected([](lir::LirModule& m) { m.globals.push_back(m.globals[0]); },
           "duplicate global identities must reject the whole module");
  rejected([](lir::LirModule& m) {
             m.globals[0].link_name_id = static_cast<c4c::LinkNameId>(999);
           },
           "unresolved global link identities must reject transactionally");
  rejected([](lir::LirModule& m) { m.globals[0].name = "wrong_spelling"; },
           "link spelling mismatches must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].llvm_type_ref.reset();
             m.globals[0].type.ptr_level = -2;
             m.globals[0].llvm_type = "ptr";
           },
           "negative pointer extern depth must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].is_extern_decl = false;
             m.globals[0].linkage_vis.clear();
             m.globals[0].type.ptr_level = 1;
             m.globals[0].llvm_type = "i8*";
             m.globals[0].llvm_type_ref.reset();
             m.globals[0].is_const = true;
             m.globals[0].init_text = "ptr null";
           },
           "const-pointer structured authority must reject rendered parity conflicts transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].is_extern_decl = false;
             m.globals[0].linkage_vis.clear();
             m.globals[0].type.ptr_level = 1;
             m.globals[0].llvm_type = "ptr";
             m.globals[0].llvm_type_ref = lir::LirTypeRef::integer(32);
             m.globals[0].is_const = true;
             m.globals[0].init_text = "ptr null";
           },
           "const-pointer mirrors must corroborate structured authority transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].is_extern_decl = false;
             m.globals[0].linkage_vis.clear();
             m.globals[0].type.ptr_level = -2;
             m.globals[0].llvm_type = "ptr";
             m.globals[0].llvm_type_ref.reset();
             m.globals[0].is_const = true;
             m.globals[0].init_text = "ptr null";
           },
           "negative const-pointer definition depth must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].llvm_type_ref.reset();
             m.globals[0].type.array_rank = 2;
             m.globals[0].type.array_size = 1;
             m.globals[0].type.array_dims[0] = 1;
             m.globals[0].type.array_dims[1] = -2;
             m.globals[0].llvm_type = "[1 x [0 x i32]]";
           },
           "unsized multidimensional array globals must remain unsupported transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].llvm_type_ref.reset();
             m.globals[0].type = scalar_type(c4c::TB_STRUCT);
             m.globals[0].llvm_type = "%struct.Payload";
           },
           "mirror-free aggregate globals must remain unsupported transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].llvm_type_ref.reset();
             m.globals[0].llvm_type = "i64";
           },
           "mirror-free scalar rendered parity conflicts must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].llvm_type_ref =
                 lir::LirTypeRef("not-an-integer", lir::LirTypeKind::Integer);
             m.globals[0].llvm_type = "not-an-integer";
           },
           "malformed structured global types must reject transactionally");
  rejected([](lir::LirModule& m) { m.globals[0].llvm_type = "i64"; },
           "rendered type compatibility conflicts must reject transactionally");
  rejected([](lir::LirModule& m) { m.globals[0].type = scalar_type(c4c::TB_LONG); },
           "source type compatibility conflicts must reject transactionally");
  rejected([](lir::LirModule& m) { m.globals[0].linkage_vis = "weak "; },
           "linkage compatibility conflicts must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].linkage_vis = "extern_weak ";
             m.globals[0].is_extern_decl = false;
             m.globals[0].init_text = "i32 0";
           },
           "extern-weak definition-shaped rows must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].linkage_vis = "extern_weak ";
             m.globals[0].qualifier = "constant ";
           },
           "extern-weak declarations with constant qualifiers must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].linkage_vis = "extern_weak ";
             m.globals[0].init_text = "i32 0";
           },
           "extern-weak declarations with initializers must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].is_extern_decl = false;
             m.globals[0].linkage_vis = "weak hidden hidden ";
             m.globals[0].init_text = "i32 0";
           },
           "duplicated visibility spellings must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].linkage_vis = "external concealed ";
           },
           "unknown visibility spellings must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].linkage_vis = "external hidden protected ";
           },
           "multiple visibility spellings must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].linkage_vis = "internal protected ";
           },
           "visibility cannot conceal linkage contradictions with independent flags");
  rejected([](lir::LirModule& m) { m.globals[0].qualifier = "constant "; },
           "qualifier compatibility conflicts must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].is_extern_decl = false;
             m.globals[0].linkage_vis.clear();
             m.globals[0].is_const = true;
             m.globals[0].init_text = "i32 0";
           },
           "constant definitions with global qualifiers must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].is_extern_decl = false;
             m.globals[0].linkage_vis.clear();
             m.globals[0].type.ptr_level = 1;
             m.globals[0].llvm_type = "ptr";
             m.globals[0].llvm_type_ref = lir::LirTypeRef("ptr");
             m.globals[0].is_const = true;
             m.globals[0].qualifier = "constant ";
             m.globals[0].init_text = "ptr null";
           },
           "const-pointer definitions with constant qualifiers must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].is_extern_decl = false;
             m.globals[0].linkage_vis.clear();
             m.globals[0].type.ptr_level = 1;
             m.globals[0].llvm_type = "ptr";
             m.globals[0].llvm_type_ref = lir::LirTypeRef("ptr");
           },
           "non-const ordinary pointer definitions require initializer payloads transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].is_extern_decl = false;
             m.globals[0].linkage_vis.clear();
             m.globals[0].qualifier = "constant ";
             m.globals[0].init_text = "i32 0";
           },
           "non-const definitions with constant qualifiers must reject transactionally");
  rejected([](lir::LirModule& m) { m.globals[0].is_internal = true; },
           "incoherent external/global flags must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].is_extern_decl = false;
             m.globals[0].is_internal = true;
             m.globals[0].linkage_vis.clear();
             m.globals[0].init_text = "i32 0";
           },
           "internal flags without internal linkage must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].is_extern_decl = false;
             m.globals[0].linkage_vis = "internal ";
             m.globals[0].init_text = "i32 0";
           },
           "internal linkage without internal flags must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].is_extern_decl = false;
             m.globals[0].is_internal = true;
             m.globals[0].is_const = true;
             m.globals[0].linkage_vis = "internal ";
             m.globals[0].init_text = "i32 0";
           },
           "internal constant definitions with global qualifiers must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].is_extern_decl = false;
             m.globals[0].is_internal = true;
             m.globals[0].linkage_vis = "internal ";
             m.globals[0].qualifier = "constant ";
             m.globals[0].init_text = "i32 0";
           },
           "internal non-const definitions with constant qualifiers must reject transactionally");
  rejected([](lir::LirModule& m) { m.globals[0].align_bytes = 3; },
           "non-power-of-two global alignment must reject transactionally");
  rejected([](lir::LirModule& m) { m.globals[0].is_extern_decl = false; },
           "definitions without initializer payloads must reject transactionally");
  rejected([](lir::LirModule& m) { m.globals[0].init_text = "i32 0"; },
           "text initializers must never be silently dropped or parsed");
  rejected([](lir::LirModule& m) {
             m.globals[0].initializer_function_link_name_ids.push_back(
                 m.globals[0].link_name_id);
           },
           "extern declarations cannot carry initializer reference evidence");
  rejected([](lir::LirModule& m) {
             m.globals[0].is_extern_decl = false;
             m.globals[0].linkage_vis.clear();
             m.globals[0].init_text = "i32 0";
             m.globals[0].initializer_function_link_name_ids = {
                 static_cast<c4c::LinkNameId>(999)};
           },
           "dangling initializer function links must reject the whole module");
  rejected([](lir::LirModule& m) {
             m.globals[0].is_extern_decl = false;
             m.globals[0].linkage_vis.clear();
             m.globals[0].init_text = "i32 0";
             m.globals[0].initializer_function_link_name_ids = {
                 c4c::kInvalidLinkName};
           },
           "invalid initializer function links must reject the whole module");

  bir::ModuleBuilder builder;
  expect(builder.add_link_name(1, "linked_global").has_value(),
         "staged link name should be available to global receipt");
  expect(builder
             .add_global_object("", bir::Type{bir::TypeKind::I32}, 4,
                                false, false, false, true)
             .error() == bir::BuildError::EmptyGlobalName,
         "builder must reject empty global identities");
  expect(builder
             .add_global_object("linked_global",
                                bir::Type{bir::TypeKind::I32}, 4,
                                false, false, false, true,
                                c4c::kInvalidLinkName)
             .error() == bir::BuildError::InvalidGlobalLinkName,
         "builder must reject explicitly invalid link identities");
  expect(builder
             .add_global_object("bad_initializer_link",
                                bir::Type{bir::TypeKind::I32}, 4,
                                false, false, false, false, std::nullopt,
                                std::string{"i32 0"},
                                {static_cast<c4c::LinkNameId>(999)})
             .error() == bir::BuildError::InvalidGlobalInitializerLinkName,
         "builder must resolve every initializer link before appending a global");
  expect(builder
             .add_global_object("bad_invalid_initializer_link",
                                bir::Type{bir::TypeKind::I32}, 4,
                                false, false, false, false, std::nullopt,
                                std::string{"i32 0"},
                                {c4c::kInvalidLinkName})
             .error() == bir::BuildError::InvalidGlobalInitializerLinkName,
         "builder must reject invalid initializer link sentinels");
  expect(builder
             .add_global_object("malformed_global",
                                bir::Type{bir::TypeKind::Void}, 3,
                                true, false, false, false)
             .has_value(),
         "builder should retain malformed staged global state for verifier diagnosis");
  expect(builder
             .add_global_object("malformed_global",
                                bir::Type{bir::TypeKind::I32}, 4,
                                false, false, false, true)
             .error() == bir::BuildError::DuplicateGlobalObject,
         "duplicate staged global receipt must remain append-only");
  auto published = std::move(builder).publish();
  expect(!published.has_value() &&
             published.error().reason == bir::PublishError::VerificationFailed,
         "FoundationVerifier must reject malformed staged global state");

  bir::ModuleBuilder coherence_builder;
  expect(coherence_builder
             .add_global_object("extern_with_initializer",
                                bir::Type{bir::TypeKind::I32}, 4,
                                false, false, false, true, std::nullopt,
                                std::string{"i32 0"})
             .has_value(),
         "builder should stage initializer coherence errors for verifier diagnosis");
  auto incoherent = std::move(coherence_builder).publish();
  expect(!incoherent.has_value() &&
             incoherent.error().reason == bir::PublishError::VerificationFailed,
         "FoundationVerifier must reject extern globals carrying initializers");

  bir::ModuleBuilder coherent_weak_builder;
  expect(coherent_weak_builder
             .add_global_object("coherent_weak_definition",
                                bir::Type{bir::TypeKind::I32}, 4,
                                false, true, false, false, std::nullopt,
                                std::string{"i32 0"})
             .has_value(),
         "builder should stage coherent typed weak definitions");
  expect(std::move(coherent_weak_builder).publish().has_value(),
         "FoundationVerifier should publish coherent typed weak definitions");

  bir::ModuleBuilder weak_internal_builder;
  expect(weak_internal_builder
             .add_global_object("weak_internal_definition",
                                bir::Type{bir::TypeKind::I32}, 4,
                                true, true, false, false, std::nullopt,
                                std::string{"i32 0"})
             .has_value(),
         "builder should stage weak/internal coherence errors for verifier diagnosis");
  auto weak_internal = std::move(weak_internal_builder).publish();
  expect(!weak_internal.has_value() &&
             weak_internal.error().reason ==
                 bir::PublishError::VerificationFailed,
         "FoundationVerifier must reject globals typed as both weak and internal");

  bir::ModuleBuilder weak_extern_builder;
  expect(weak_extern_builder
             .add_global_object("weak_extern_declaration",
                                bir::Type{bir::TypeKind::I32}, 4,
                                false, true, false, true)
             .has_value(),
         "builder should stage coherent typed weak external declarations");
  auto weak_extern = std::move(weak_extern_builder).publish();
  expect(weak_extern.has_value(),
         "FoundationVerifier must admit coherent typed weak external declarations");
  const auto weak_extern_ids = weak_extern.value().view().global_objects();
  const auto weak_extern_view =
      weak_extern.value().view().global_object(weak_extern_ids[0]).value();
  expect(weak_extern_ids.size() == 1 && weak_extern_view.is_weak &&
             weak_extern_view.is_extern_declaration &&
             !weak_extern_view.is_internal && !weak_extern_view.initializer,
         "typed weak external views must preserve both linkage facts without an initializer");

  bir::ModuleBuilder invalid_visibility_builder;
  expect(invalid_visibility_builder
             .add_global_object(
                 "invalid_visibility", bir::Type{bir::TypeKind::I32}, 4,
                 false, false, false, true, std::nullopt, std::nullopt, {},
                 static_cast<bir::SymbolVisibility>(99))
             .has_value(),
         "builder should stage closed-enum visibility errors for verifier diagnosis");
  auto invalid_visibility = std::move(invalid_visibility_builder).publish();
  expect(!invalid_visibility.has_value() &&
             invalid_visibility.error().reason ==
                 bir::PublishError::VerificationFailed,
         "FoundationVerifier must reject unknown typed visibility alternatives");

  bir::ModuleBuilder no_partial_builder;
  expect(no_partial_builder.add_link_name(1, "init_target").has_value(),
         "initializer target should enter the sole link-name table");
  expect(no_partial_builder
             .add_global_object("rejected_definition",
                                bir::Type{bir::TypeKind::I32}, 4,
                                false, false, false, false, std::nullopt,
                                std::string{"i32 0"},
                                {static_cast<c4c::LinkNameId>(99)})
             .error() == bir::BuildError::InvalidGlobalInitializerLinkName,
         "dangling initializer receipt must fail before publication state changes");
  const auto first_after_rejection =
      no_partial_builder.add_global_object(
          "accepted_definition", bir::Type{bir::TypeKind::I32}, 4,
          false, false, false, false, std::nullopt, std::string{"i32 0"}, {1});
  expect(first_after_rejection.has_value() &&
             first_after_rejection.value().slot == 0,
         "failed initializer resolution must not append a partial global row");
  expect(std::move(no_partial_builder).publish().has_value(),
         "a valid definition after rejected receipt should publish normally");
}

void test_specialization_metadata_receipt_and_rejections() {
  lir::LirModule module;
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());
  const c4c::LinkNameId first_link =
      module.link_names.intern("_Z4makeIiEvT_");
  const c4c::LinkNameId second_link =
      module.link_names.intern("_Z4makeIdEvT_");
  module.spec_entries.push_back(
      {"type=i32;value=7", "ns::make<T>", "_Z4makeIiEvT_", first_link});
  module.spec_entries.push_back(
      {"type=f64;value=9", "ns::make<T>", "_Z4makeIdEvT_", second_link});

  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(),
         imported.has_value()
             ? "structured specialization metadata should import"
             : "structured specialization metadata should import: " +
                   imported.error().detail);
  const auto view = imported.value().view();
  const auto ids = view.specializations();
  expect(ids.size() == 2 && ids[0].slot == 0 && ids[1].slot == 1,
         "specialization IDs must preserve source vector order");
  const auto first = view.specialization(ids[0]);
  const auto second = view.specialization(ids[1]);
  expect(first.has_value() && second.has_value(),
         "ordered specialization IDs must resolve through the immutable view");
  expect(first.value().spec_key == "type=i32;value=7" &&
             first.value().template_origin == "ns::make<T>" &&
             first.value().mangled_name == "_Z4makeIiEvT_" &&
             view.spelling(first.value().mangled_link_name).value() ==
                 "_Z4makeIiEvT_" &&
             second.value().spec_key == "type=f64;value=9" &&
             second.value().template_origin == "ns::make<T>" &&
             second.value().mangled_name == "_Z4makeIdEvT_" &&
             view.spelling(second.value().mangled_link_name).value() ==
                 "_Z4makeIdEvT_",
         "specialization views must retain every exact structured field and typed link identity");
  expect(bir::FoundationVerifier::verify(imported.value()).ok(),
         "foundation verification must reach and accept specialization storage");

  const auto rejected = [](lir::LirModule candidate) {
    auto raw = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw.has_value() &&
               raw.error().code ==
                   bir::ImportErrorCode::UnsupportedSpecializations,
           "malformed specialization metadata must publish no RawBir");
    auto canonical = bir::lower_lir_to_canonical_bir(candidate);
    expect(!canonical.has_value() &&
               canonical.error().code ==
                   bir::ImportErrorCode::UnsupportedSpecializations,
           "malformed specialization metadata must reject the whole module transactionally");
  };

  for (int empty_field = 0; empty_field != 3; ++empty_field) {
    lir::LirModule candidate;
    candidate.link_name_texts = std::make_shared<c4c::TextTable>();
    candidate.link_names.attach_text_table(candidate.link_name_texts.get());
    candidate.struct_names.attach_text_table(candidate.link_name_texts.get());
    const auto link = candidate.link_names.intern("_Z3rowv");
    lir::LirSpecEntry row{"key", "origin", "_Z3rowv", link};
    if (empty_field == 0) row.spec_key.clear();
    if (empty_field == 1) row.template_origin.clear();
    if (empty_field == 2) row.mangled_name.clear();
    candidate.spec_entries.push_back(std::move(row));
    rejected(std::move(candidate));
  }

  lir::LirModule invalid_link;
  invalid_link.spec_entries.push_back(
      {"key", "origin", "_Z3rowv", c4c::kInvalidLinkName});
  rejected(std::move(invalid_link));

  lir::LirModule unresolved_link;
  unresolved_link.spec_entries.push_back(
      {"key", "origin", "_Z3rowv", static_cast<c4c::LinkNameId>(99)});
  rejected(std::move(unresolved_link));

  lir::LirModule mismatched_link;
  mismatched_link.link_name_texts = std::make_shared<c4c::TextTable>();
  mismatched_link.link_names.attach_text_table(
      mismatched_link.link_name_texts.get());
  mismatched_link.struct_names.attach_text_table(
      mismatched_link.link_name_texts.get());
  const auto mismatch = mismatched_link.link_names.intern("_Z5otherv");
  mismatched_link.spec_entries.push_back(
      {"key", "origin", "_Z3rowv", mismatch});
  rejected(std::move(mismatched_link));

  lir::LirModule duplicate_semantic;
  duplicate_semantic.link_name_texts = std::make_shared<c4c::TextTable>();
  duplicate_semantic.link_names.attach_text_table(
      duplicate_semantic.link_name_texts.get());
  duplicate_semantic.struct_names.attach_text_table(
      duplicate_semantic.link_name_texts.get());
  const auto semantic_a = duplicate_semantic.link_names.intern("_Z4samei");
  const auto semantic_b = duplicate_semantic.link_names.intern("_Z4samed");
  duplicate_semantic.spec_entries.push_back(
      {"same-key", "same-origin", "_Z4samei", semantic_a});
  duplicate_semantic.spec_entries.push_back(
      {"same-key", "same-origin", "_Z4samed", semantic_b});
  rejected(std::move(duplicate_semantic));

  lir::LirModule duplicate_link;
  duplicate_link.link_name_texts = std::make_shared<c4c::TextTable>();
  duplicate_link.link_names.attach_text_table(
      duplicate_link.link_name_texts.get());
  duplicate_link.struct_names.attach_text_table(
      duplicate_link.link_name_texts.get());
  const auto shared_link = duplicate_link.link_names.intern("_Z6sharedv");
  duplicate_link.spec_entries.push_back(
      {"key-a", "origin", "_Z6sharedv", shared_link});
  duplicate_link.spec_entries.push_back(
      {"key-b", "origin", "_Z6sharedv", shared_link});
  rejected(std::move(duplicate_link));

  bir::ModuleBuilder builder;
  expect(builder.add_link_name(1, "_Z5firstv").has_value() &&
             builder.add_link_name(2, "_Z6secondv").has_value() &&
             builder.add_link_name(3, "_Z5thirdv").has_value(),
         "builder specialization test needs an ordered link-name domain");
  expect(builder.add_specialization("", "origin", "_Z5firstv", 1).error() ==
             bir::BuildError::EmptySpecializationField,
         "builder must reject empty specialization fields");
  expect(builder.add_specialization("key", "origin", "_Z5firstv", 99).error() ==
             bir::BuildError::InvalidSpecializationLinkName,
         "builder must reject unresolved specialization link identity");
  expect(builder.add_specialization("key", "origin", "_Z6secondv", 1).error() ==
             bir::BuildError::InvalidSpecializationLinkName,
         "builder must reject link spelling mismatch");
  const auto accepted =
      builder.add_specialization("key", "origin", "_Z5firstv", 1);
  expect(accepted.has_value() && accepted.value().slot == 0,
         "failed specialization receipts must append no partial state");
  expect(builder.add_specialization("key", "origin", "_Z6secondv", 2).error() ==
             bir::BuildError::DuplicateSpecialization,
         "builder must reject duplicate template-origin/spec-key identity");
  expect(builder.add_specialization("other", "origin", "_Z5firstv", 1).error() ==
             bir::BuildError::DuplicateSpecialization,
         "builder must reject conflicting reuse of mangled link identity");
  const auto second_accepted =
      builder.add_specialization("other", "origin", "_Z6secondv", 2);
  expect(second_accepted.has_value() && second_accepted.value().slot == 1,
         "valid receipt after duplicate rejection must keep contiguous order");
  expect(std::move(builder).publish().has_value(),
         "valid specialization builder state should publish after rejected receipts");
}

void test_accumulated_module_surface_checkpoint() {
  const auto valid_module = [] {
    lir::LirModule module;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    module.struct_names.attach_text_table(module.link_name_texts.get());

    const auto external_link = module.link_names.intern("checkpoint_external");
    const auto aggregate_link =
        module.link_names.intern("checkpoint_aggregate");
    const auto init_a = module.link_names.intern("checkpoint_init_a");
    const auto init_b = module.link_names.intern("checkpoint_init_b");
    const auto specialization_link =
        module.link_names.intern("_Z18checkpoint_makeIiEvT_");
    const auto aggregate_name =
        module.struct_names.intern("%struct.CheckpointAggregate");

    lir::LirStructDecl aggregate_decl;
    aggregate_decl.name_id = aggregate_name;
    aggregate_decl.is_packed = true;
    aggregate_decl.fields = {{lir::LirTypeRef::integer(32)},
                             {lir::LirTypeRef("double")}};
    module.record_struct_decl(std::move(aggregate_decl));

    module.string_pool = {
        lir::LirStringConst{"@.checkpoint.0", std::string{"A\0B", 3}, 4},
        lir::LirStringConst{"@.checkpoint.1", "opaque\\00tail", -1},
    };
    module.str_pool_map.emplace("checkpoint-source", "@.checkpoint.0");
    module.str_pool_idx = 2;

    const lir::LirExternDecl external{
        "checkpoint_external", "i32", lir::LirTypeRef::integer(32),
        lir::LirExtAttr::ZeroExt, external_link};
    module.extern_decls.push_back(external);
    module.extern_decl_link_name_map.emplace(
        external_link,
        lir::LirModule::ExternDeclInfo{
            external.name, external.return_type_str, external.return_type,
            external.return_ext_attr, external.link_name_id});

    lir::LirGlobal external_global;
    external_global.name = "checkpoint_fallback_global";
    external_global.type = scalar_type(c4c::TB_INT);
    external_global.linkage_vis = "external hidden ";
    external_global.qualifier = "global ";
    external_global.llvm_type = "i32";
    external_global.llvm_type_ref = lir::LirTypeRef::integer(32);
    external_global.align_bytes = 4;
    external_global.is_extern_decl = true;
    module.globals.push_back(std::move(external_global));

    lir::LirGlobal aggregate_global;
    aggregate_global.name = "checkpoint_aggregate";
    aggregate_global.link_name_id = aggregate_link;
    aggregate_global.type = scalar_type(c4c::TB_STRUCT);
    aggregate_global.linkage_vis = "protected ";
    aggregate_global.qualifier = "global ";
    aggregate_global.llvm_type = "%struct.CheckpointAggregate";
    aggregate_global.llvm_type_ref = lir::LirTypeRef::struct_type(
        aggregate_global.llvm_type, aggregate_name);
    aggregate_global.init_text = "{ i32 7, double 2.5 }";
    aggregate_global.initializer_function_link_name_ids = {
        init_b, init_a, init_b};
    aggregate_global.align_bytes = 8;
    module.globals.push_back(std::move(aggregate_global));

    module.spec_entries.push_back(
        {"type=i32;value=7", "checkpoint::make<T>",
         "_Z18checkpoint_makeIiEvT_", specialization_link});
    return module;
  };

  auto module = valid_module();
  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(),
         "all admitted Step 2-3 module families should coexist in Raw BIR");
  expect(bir::FoundationVerifier::verify(imported.value()).ok(),
         "the accumulated module surface must remain verifier reachable");

  const auto view = imported.value().view();
  const auto link_names = view.link_names();
  expect(link_names.size() == 5 && link_names[0].slot == 0 &&
             link_names[1].slot == 1 && link_names[2].slot == 2 &&
             link_names[3].slot == 3 && link_names[4].slot == 4 &&
             view.spelling(link_names[0]).value() == "checkpoint_external" &&
             view.spelling(link_names[1]).value() == "checkpoint_aggregate" &&
             view.spelling(link_names[2]).value() == "checkpoint_init_a" &&
             view.spelling(link_names[3]).value() == "checkpoint_init_b" &&
             view.spelling(link_names[4]).value() ==
                 "_Z18checkpoint_makeIiEvT_",
         "the combined module must preserve stable ordered symbol identity");

  const auto struct_names = view.struct_names();
  const auto struct_decls = view.struct_declarations();
  expect(struct_names.size() == 1 && struct_decls.size() == 1 &&
             view.spelling(struct_names[0]).value() ==
                 "%struct.CheckpointAggregate",
         "the combined module must preserve its named aggregate identity");
  const auto aggregate_decl = view.struct_declaration(struct_decls[0]).value();
  expect(aggregate_decl.name == struct_names[0] && aggregate_decl.is_packed &&
             aggregate_decl.fields.size() == 2 &&
             aggregate_decl.fields[0].type == bir::Type{bir::TypeKind::I32} &&
             aggregate_decl.fields[1].type == bir::Type{bir::TypeKind::F64},
         "the combined module must retain the structured declaration view");

  const auto strings = view.string_data();
  expect(strings.size() == 2 && strings[0].slot == 0 && strings[1].slot == 1,
         "combined string data must retain vector authority and order");
  const auto first_string = view.string_data(strings[0]).value();
  const auto second_string = view.string_data(strings[1]).value();
  expect(first_string.pool_name == "@.checkpoint.0" &&
             first_string.raw_bytes == std::string{"A\0B", 3} &&
             first_string.byte_length == 4 &&
             second_string.pool_name == "@.checkpoint.1" &&
             second_string.raw_bytes == "opaque\\00tail" &&
             second_string.byte_length == -1 &&
             view.string_data("@.checkpoint.0").value() == strings[0],
         "combined string views must preserve typed identities and opaque bytes");

  const auto externals = view.external_declarations();
  expect(externals.size() == 1 && externals[0].slot == 0,
         "the external declaration must retain its ordered index");
  const auto external = view.external_declaration(externals[0]).value();
  expect(external.source_name == "checkpoint_external" &&
             external.return_type == bir::Type{bir::TypeKind::I32} &&
             external.return_extension == bir::ReturnExtension::ZeroExt &&
             std::holds_alternative<bir::LinkNameId>(external.identity) &&
             std::get<bir::LinkNameId>(external.identity) == link_names[0] &&
             view.external_declaration(link_names[0]).value() == externals[0],
         "the external index and symbol lookup must resolve one typed identity");

  const auto globals = view.global_objects();
  expect(globals.size() == 2 && globals[0].slot == 0 && globals[1].slot == 1,
         "multiple admitted global forms must preserve source order");
  const auto fallback_global = view.global_object(globals[0]).value();
  const auto aggregate_global = view.global_object(globals[1]).value();
  expect(fallback_global.object_type == bir::Type{bir::TypeKind::I32} &&
             fallback_global.is_extern_declaration &&
             fallback_global.visibility == bir::SymbolVisibility::Hidden &&
             std::holds_alternative<bir::FallbackGlobalName>(
                 fallback_global.identity) &&
             !fallback_global.initializer,
         "the combined module must preserve the admitted fallback extern global");
  expect(aggregate_global.object_type.kind == bir::TypeKind::Struct &&
             aggregate_global.object_type.struct_name_id ==
                 module.globals[1].llvm_type_ref->struct_name_id() &&
             aggregate_global.object_type.spelling ==
                 "%struct.CheckpointAggregate" &&
             std::get<bir::LinkNameId>(aggregate_global.identity) ==
                 link_names[1] &&
             aggregate_global.visibility == bir::SymbolVisibility::Protected &&
             aggregate_global.alignment == 8 &&
             !aggregate_global.is_extern_declaration &&
             aggregate_global.initializer &&
             aggregate_global.initializer->opaque_payload ==
                 "{ i32 7, double 2.5 }" &&
             aggregate_global.initializer->function_links ==
                 std::vector<bir::LinkNameId>{link_names[3], link_names[2],
                                               link_names[3]},
         "the named aggregate global must retain identity and initializer topology");

  const auto specializations = view.specializations();
  expect(specializations.size() == 1 && specializations[0].slot == 0,
         "specialization metadata must retain its ordered typed identity");
  const auto specialization = view.specialization(specializations[0]).value();
  expect(specialization.spec_key == "type=i32;value=7" &&
             specialization.template_origin == "checkpoint::make<T>" &&
             specialization.mangled_name == "_Z18checkpoint_makeIiEvT_" &&
             specialization.mangled_link_name == link_names[4],
         "specialization metadata must cross-reference the shared symbol table");

  auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(),
         "the same accumulated module must publish Canonical BIR");
  const auto canonical_view = canonical.value().view();
  expect(canonical_view.struct_declarations().size() == 1 &&
             canonical_view.string_data().size() == 2 &&
             canonical_view.external_declarations().size() == 1 &&
             canonical_view.global_objects().size() == 2 &&
             canonical_view.specializations().size() == 1,
         "canonical publication must retain every accumulated module family");

  const auto rejected = [&](auto mutate, bir::ImportErrorCode expected,
                            const std::string& message) {
    auto candidate = valid_module();
    mutate(candidate);
    const auto raw = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw.has_value() && raw.error().code == expected,
           message + " (Raw rollback)");
    const auto rejected_canonical =
        bir::lower_lir_to_canonical_bir(candidate);
    expect(!rejected_canonical.has_value() &&
               rejected_canonical.error().code == expected,
           message + " (Canonical rollback)");
  };
  rejected(
      [](lir::LirModule& candidate) {
        candidate.globals[1].initializer_function_link_name_ids.back() =
            static_cast<c4c::LinkNameId>(999);
      },
      bir::ImportErrorCode::UnsupportedGlobals,
      "a late invalid initializer cross-reference must publish no partial module");
  rejected(
      [](lir::LirModule& candidate) {
        candidate.spec_entries[0].mangled_link_name_id =
            static_cast<c4c::LinkNameId>(999);
      },
      bir::ImportErrorCode::UnsupportedSpecializations,
      "a late invalid specialization cross-reference must publish no partial module");
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
  test_structured_constant_value_receipt();
  test_constant_value_rejections_and_forward_use();
  test_string_pool_receipt_and_views();
  test_string_pool_rejections_and_transactionality();
  test_external_declaration_receipt_and_views();
  test_external_declaration_rejections_and_transactionality();
  test_global_object_receipt_and_views();
  test_scalar_global_type_authority_without_mirror();
  test_enum_storage_global_receipt_and_rejections();
  test_vrm_register_global_receipt_and_rejections();
  test_complex_storage_global_receipt_and_rejections();
  test_scalar_pointer_global_receipt_and_rejections();
  test_pointer_to_array_global_receipt_and_rejections();
  test_fixed_scalar_base_array_global_receipt_and_rejections();
  test_direct_vector_global_receipt_and_rejections();
  test_named_aggregate_global_receipt_and_rejections();
  test_flexible_member_literal_struct_global_receipt_and_rejections();
  test_global_object_rejections_and_transactionality();
  test_specialization_metadata_receipt_and_rejections();
  test_accumulated_module_surface_checkpoint();
  test_inline_asm_shape_rejection();
  return 0;
}
