#[derive(Clone, Copy, Debug, PartialEq)]
pub(crate) enum Domain
{
	Program,
	Integer,
}

pub(crate) struct ScopedFormula
{
	pub free_variable_declarations: std::rc::Rc<foliage::VariableDeclarations>,
	pub formula: Box<foliage::Formula>,
}
