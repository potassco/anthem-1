// Operators

pub enum BinaryOperator
{
	Plus,
	Minus,
	Multiplication,
	Division,
	Modulo,
	Power,
}

pub enum ComparisonOperator
{
	GreaterThan,
	LessThan,
	LessEqual,
	GreaterEqual,
	NotEqual,
	Equal,
}

pub enum UnaryOperator
{
	AbsoluteValue,
	Minus,
}

// Primitives

pub struct FunctionDeclaration
{
	pub name: String,
	pub arity: usize,
}

pub struct PredicateDeclaration
{
	pub name: String,
	pub arity: usize,
}

pub struct VariableDeclaration
{
	pub name: String,
}

// Terms

pub struct BinaryOperation
{
	pub operator: BinaryOperator,
	pub left: Box<Term>,
	pub right: Box<Term>,
}

pub struct Function
{
	pub declaration: std::rc::Rc<FunctionDeclaration>,
	pub arguments: Vec<Box<Term>>,
}

pub struct Interval
{
	pub from: Box<Term>,
	pub to: Box<Term>,
}

pub enum SpecialInteger
{
	Infimum,
	Supremum,
}

pub struct UnaryOperation
{
	pub operator: UnaryOperator,
	pub argument: Box<Term>,
}

pub struct Variable
{
	pub declaration: std::rc::Rc<VariableDeclaration>,
}

// Formulas

pub struct Biconditional
{
	pub left: Box<Formula>,
	pub right: Box<Formula>,
}

pub struct Comparison
{
	pub operator: ComparisonOperator,
	pub left: Box<Term>,
	pub right: Box<Term>,
}

pub struct Exists
{
	pub parameters: Vec<std::rc::Rc<VariableDeclaration>>,
	pub argument: Box<Formula>,
}

pub struct ForAll
{
	pub parameters: Vec<std::rc::Rc<VariableDeclaration>>,
	pub argument: Box<Formula>,
}

pub struct Implies
{
	pub left: Box<Formula>,
	pub right: Box<Formula>,
}

pub struct Predicate
{
	pub declaration: std::rc::Rc<PredicateDeclaration>,
	pub arguments: Vec<Box<Term>>,
}

// Variants

pub enum Term
{
	BinaryOperation(BinaryOperation),
	Boolean(bool),
	Function(Function),
	Integer(i32),
	Interval(Interval),
	SpecialInteger(SpecialInteger),
	String(String),
	UnaryOperation(UnaryOperation),
	Variable(Variable),
}

pub type Terms = Vec<Box<Term>>;

pub enum Formula
{
	And(Formulas),
	Biconditional(Biconditional),
	Boolean(bool),
	Comparison(Comparison),
	Exists(Exists),
	ForAll(ForAll),
	Implies(Implies),
	Not(Box<Formula>),
	Or(Formulas),
	Predicate(Predicate),
}

pub type Formulas = Vec<Box<Formula>>;
