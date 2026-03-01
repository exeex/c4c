from dataclasses import dataclass
from typing import Dict, List, Optional
class Node:
    pass


@dataclass
class Param(Node):
    typ: str
    name: Optional[str]


@dataclass
class FunctionDecl(Node):
    ret_type: str
    name: str
    params: List[Param]


@dataclass
class Function(Node):
    ret_type: str
    name: str
    params: List[Param]
    body: List[Node]


@dataclass
class Program(Node):
    struct_defs: List["StructDef"]
    decls: List[FunctionDecl]
    globals: List["GlobalVar"]
    funcs: List[Function]
    enum_consts: Dict[str, int] = None  # type: ignore


@dataclass
class Decl(Node):
    name: str
    base_type: str
    ptr_level: int
    size: Optional[int]
    init: Optional[Node]
    is_static: bool = False
    extra_dims: Optional[List[int]] = None  # for arr[M][N]: size=M, extra_dims=[N]
    full_type: str = ""  # full const-qualified type for _Generic matching (e.g., "const int*")


@dataclass
class Assign(Node):
    name: str
    expr: Node


@dataclass
class GlobalVar(Node):
    name: str
    base_type: str
    ptr_level: int
    size: Optional[int]
    init: Optional[Node]
    is_extern: bool = False


@dataclass
class AssignExpr(Node):
    target: Node
    op: str
    expr: Node


@dataclass
class ExprStmt(Node):
    expr: Node


@dataclass
class Return(Node):
    expr: Optional[Node]


@dataclass
class IntLit(Node):
    value: int
    suffix: str = ""  # "", "L", "LL", "U", "UL", "ULL"


@dataclass
class FloatLit(Node):
    value: float


@dataclass
class Var(Node):
    name: str


@dataclass
class Call(Node):
    name: str
    args: List[Node]


@dataclass
class IndirectCall(Node):
    """Call through a function pointer expression: expr(args)"""
    func: "Node"
    args: List["Node"]


@dataclass
class Index(Node):
    base: Node
    index: Node


@dataclass
class Member(Node):
    base: Node
    field: str
    through_ptr: bool


@dataclass
class StructField(Node):
    typ: str
    name: str
    array_size: Optional[int] = None  # non-None if this field is an array (e.g. int arr[4])


@dataclass
class StructDef(Node):
    name: str
    fields: List[StructField]
    is_union: bool = False
    union_aliases: Optional[Dict[str, int]] = None  # anon union field name -> representative index
    anon_struct_fields: Optional[Dict[str, str]] = None  # field_name -> anon_struct_tag (union with inlined anon struct)


@dataclass
class BinOp(Node):
    op: str
    lhs: Node
    rhs: Node


@dataclass
class UnaryOp(Node):
    op: str
    expr: Node


@dataclass
class Cast(Node):
    typ: str
    expr: Node


@dataclass
class TernaryOp(Node):
    cond: Node
    then_expr: Node
    else_expr: Node


@dataclass
class SizeofExpr(Node):
    typ: Optional[str]
    expr: Optional[Node]


@dataclass
class StringLit(Node):
    value: str


@dataclass
class EnumDef(Node):
    name: Optional[str]
    constants: List[tuple]  # list of (name, value)


@dataclass
class IncDec(Node):
    name: str
    op: str
    prefix: bool


@dataclass
class Block(Node):
    body: List[Node]


@dataclass
class If(Node):
    cond: Node
    then_stmt: Node
    else_stmt: Optional[Node]


@dataclass
class While(Node):
    cond: Node
    body: Node


@dataclass
class For(Node):
    init: Optional[Node]
    cond: Optional[Node]
    post: Optional[Node]
    body: Node


@dataclass
class DoWhile(Node):
    body: Node
    cond: Node


@dataclass
class Switch(Node):
    expr: Node
    body: Node


@dataclass
class Break(Node):
    pass


@dataclass
class Continue(Node):
    pass


@dataclass
class EmptyStmt(Node):
    pass


@dataclass
class LabelStmt(Node):
    name: str
    stmt: Node


@dataclass
class Goto(Node):
    name: str


@dataclass
class Case(Node):
    value: Node
    stmt: Node


@dataclass
class Default(Node):
    stmt: Node


@dataclass
class ArrayInit(Node):
    """Array initializer with evaluated integer values (full-size list)."""
    values: List[int]  # full-size list; zeros for unspecified elements


@dataclass
class StructInit(Node):
    """Struct initializer: list of (optional_field_name, Node) pairs."""
    entries: List  # list of (Optional[str], Node)


@dataclass
class StructArrayInit(Node):
    """Array of struct initializers: [(index, StructInit), ...]"""
    entries: List  # list of (int, Optional[Node]) - index and initializer
    size: int


@dataclass
class StmtExpr(Node):
    """GCC statement expression: ({ stmts... }) — value is last expr stmt."""
    stmts: List[Node]


@dataclass
class CompoundLit(Node):
    """C99 compound literal: (type){ initializer } — anonymous object of given type."""
    typ: str
    init: Node


@dataclass
class Generic(Node):
    """C11 _Generic(expr, type1: expr1, ..., default: exprD) selection expression."""
    ctrl: Node  # controlling expression
    assocs: List  # list of (type_str_or_None, Node) — None means 'default'

