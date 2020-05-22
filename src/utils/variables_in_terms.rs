pub(crate) fn term_contains_variable(term: &crate::Term,
	variable_declaration: &crate::VariableDeclaration)
	-> bool
{
	use crate::Term;

	match term
	{
		Term::BinaryOperation(binary_operation) =>
			term_contains_variable(&binary_operation.left, variable_declaration)
				|| term_contains_variable(&binary_operation.right, variable_declaration),
		Term::Boolean(_)
		| Term::Function(_)
		| Term::Integer(_)
		| Term::SpecialInteger(_)
		| Term::String(_) => false,
		Term::UnaryOperation(unary_operation) =>
			term_contains_variable(&unary_operation.argument, variable_declaration),
		Term::Variable(variable) => *variable.declaration == *variable_declaration,
	}
}
