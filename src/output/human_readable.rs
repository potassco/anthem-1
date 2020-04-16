pub(crate) fn display_variable_declaration<C>(context: &C, formatter: &mut std::fmt::Formatter,
	variable_declaration: &std::rc::Rc<foliage::VariableDeclaration>)
	-> std::fmt::Result
where C:
	crate::traits::VariableDeclarationDomain + crate::traits::VariableDeclarationID
{
	let id = context.variable_declaration_id(variable_declaration);
	let domain = context.variable_declaration_domain(variable_declaration)
		.expect("unspecified variable domain");

	let prefix = match domain
	{
		crate::Domain::Integer => "N",
		crate::Domain::Program => "X",
	};

	write!(formatter, "{}{}", prefix, id + 1)
}
