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


@dataclass
class Var(Node):
    name: str


@dataclass
class Call(Node):
    name: str
    args: List[Node]


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


@dataclass
class StructDef(Node):
    name: str
    fields: List[StructField]


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

