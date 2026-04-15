#include "x86_codegen.hpp"

#include "../../../bir/bir.hpp"

namespace c4c::backend::x86 {

namespace {

struct MinimalScalarGlobalLoadSlice {
  std::string function_name;
  std::string global_name;
  std::int64_t init_imm = 0;
  std::size_t align_bytes = 4;
  bool zero_initializer = false;
};

struct MinimalExternScalarGlobalLoadSlice {
  std::string function_name;
  std::string global_name;
};

struct MinimalExternGlobalArrayLoadSlice {
  std::string function_name;
  std::string global_name;
  std::int64_t byte_offset = 0;
};

const char* global_load_dest_reg(IrType ty) {
  switch (ty) {
    case IrType::U32:
    case IrType::F32:
      return "%eax";
    default:
      return "%rax";
  }
}

std::optional<MinimalScalarGlobalLoadSlice> parse_minimal_scalar_global_load_slice(
    const c4c::backend::bir::Module& module) {
  using namespace c4c::backend::bir;

  if (module.functions.size() != 1 || module.globals.size() != 1 ||
      !module.string_constants.empty()) {
    return std::nullopt;
  }

  const auto& global = module.globals.front();
  if (global.is_extern || global.type != TypeKind::I32 || !global.initializer.has_value() ||
      global.initializer->kind != c4c::backend::bir::Value::Kind::Immediate ||
      global.initializer->type != TypeKind::I32) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.return_type != TypeKind::I32 ||
      !function.params.empty() || !function.local_slots.empty() || function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* load =
      entry.insts.size() == 1 ? std::get_if<LoadGlobalInst>(&entry.insts.front()) : nullptr;
  if (entry.label != "entry" || load == nullptr ||
      load->result.kind != c4c::backend::bir::Value::Kind::Named ||
      load->result.type != TypeKind::I32 || load->global_name != global.name ||
      load->byte_offset != 0 || entry.terminator.kind != TerminatorKind::Return ||
      !entry.terminator.value.has_value() || *entry.terminator.value != load->result) {
    return std::nullopt;
  }

  return MinimalScalarGlobalLoadSlice{
      .function_name = function.name,
      .global_name = global.name,
      .init_imm = global.initializer->immediate,
      .align_bytes = load->align_bytes > 0 ? load->align_bytes : 4,
      .zero_initializer = global.initializer->immediate == 0,
  };
}

std::optional<MinimalExternScalarGlobalLoadSlice> parse_minimal_extern_scalar_global_load_slice(
    const c4c::backend::bir::Module& module) {
  using namespace c4c::backend::bir;

  if (module.functions.size() != 1 || module.globals.size() != 1 ||
      !module.string_constants.empty()) {
    return std::nullopt;
  }

  const auto& global = module.globals.front();
  if (!global.is_extern || global.type != TypeKind::I32 || global.initializer.has_value()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.return_type != TypeKind::I32 ||
      !function.params.empty() || !function.local_slots.empty() || function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* load =
      entry.insts.size() == 1 ? std::get_if<LoadGlobalInst>(&entry.insts.front()) : nullptr;
  if (entry.label != "entry" || load == nullptr ||
      load->result.kind != c4c::backend::bir::Value::Kind::Named ||
      load->result.type != TypeKind::I32 || load->global_name != global.name ||
      load->byte_offset != 0 || entry.terminator.kind != TerminatorKind::Return ||
      !entry.terminator.value.has_value() || *entry.terminator.value != load->result) {
    return std::nullopt;
  }

  return MinimalExternScalarGlobalLoadSlice{
      .function_name = function.name,
      .global_name = global.name,
  };
}

std::optional<MinimalExternGlobalArrayLoadSlice> parse_minimal_extern_global_array_load_slice(
    const c4c::backend::bir::Module& module) {
  using namespace c4c::backend::bir;

  if (module.functions.size() != 1 || module.globals.size() != 1 ||
      !module.string_constants.empty()) {
    return std::nullopt;
  }

  const auto& global = module.globals.front();
  if (!global.is_extern || global.type != TypeKind::I32 || global.initializer.has_value()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.return_type != TypeKind::I32 ||
      !function.params.empty() || !function.local_slots.empty() || function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* load =
      entry.insts.size() == 1 ? std::get_if<LoadGlobalInst>(&entry.insts.front()) : nullptr;
  if (entry.label != "entry" || load == nullptr ||
      load->result.kind != c4c::backend::bir::Value::Kind::Named ||
      load->result.type != TypeKind::I32 || load->global_name != global.name ||
      load->byte_offset != 4 || entry.terminator.kind != TerminatorKind::Return ||
      !entry.terminator.value.has_value() || *entry.terminator.value != load->result) {
    return std::nullopt;
  }

  return MinimalExternGlobalArrayLoadSlice{
      .function_name = function.name,
      .global_name = global.name,
      .byte_offset = static_cast<std::int64_t>(load->byte_offset),
  };
}

}  // namespace

void X86Codegen::emit_global_addr_impl(const Value& dest, const std::string& name) {
  if (this->state.needs_got_for_addr(name)) {
    this->state.emit("    movq " + name + "@GOTPCREL(%rip), %rax");
  } else {
    this->state.emit("    leaq " + name + "(%rip), %rax");
  }
  this->emit_store_result_impl(dest);
}

void X86Codegen::emit_tls_global_addr_impl(const Value& dest, const std::string& name) {
  if (this->state.pic_mode) {
    this->state.emit("    movq " + name + "@GOTTPOFF(%rip), %rax");
    this->state.emit("    addq %fs:0, %rax");
  } else {
    this->state.emit("    movq %fs:0, %rax");
    this->state.emit("    leaq " + name + "@TPOFF(%rax), %rax");
  }
  this->emit_store_result_impl(dest);
}

void X86Codegen::emit_global_addr_absolute_impl(const Value& dest, const std::string& name) {
  this->state.emit("    movq $" + name + ", %rax");
  this->emit_store_result_impl(dest);
}

void X86Codegen::emit_global_load_rip_rel_impl(const Value& dest,
                                                const std::string& sym,
                                                IrType ty) {
  this->state.emit(std::string("    ") + this->mov_load_for_type(ty) + " " + sym + "(%rip), " +
                   global_load_dest_reg(ty));
  this->emit_store_result_impl(dest);
}

void X86Codegen::emit_global_store_rip_rel_impl(const Operand& val,
                                                 const std::string& sym,
                                                 IrType ty) {
  this->emit_load_operand_impl(val);
  this->state.emit(std::string("    ") + this->mov_store_for_type(ty) + " %" +
                   this->reg_for_type("rax", ty) + ", " + sym + "(%rip)");
}

void X86Codegen::emit_label_addr_impl(const Value& dest, const std::string& label) {
  this->state.emit("    leaq " + label + "(%rip), %rax");
  this->emit_store_result_impl(dest);
}

void X86Codegen::emit_store_result_impl(const Value& dest) { this->store_rax_to(dest); }

void X86Codegen::emit_load_operand_impl(const Operand& op) { this->operand_to_rax(op); }

}  // namespace c4c::backend::x86
