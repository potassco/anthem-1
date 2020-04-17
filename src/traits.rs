pub(crate) trait InputConstantDeclarationDomain
{
	fn input_constant_declaration_domain(&self,
		declaration: &std::rc::Rc<foliage::FunctionDeclaration>) -> crate::Domain;
}

pub(crate) trait AssignVariableDeclarationDomain
{
	fn assign_variable_declaration_domain(&self,
		variable_declaration: &std::rc::Rc<foliage::VariableDeclaration>, domain: crate::Domain);
}

pub(crate) trait VariableDeclarationDomain
{
	fn variable_declaration_domain(&self,
		variable_declaration: &std::rc::Rc<foliage::VariableDeclaration>) -> Option<crate::Domain>;
}

pub(crate) trait VariableDeclarationID
{
	fn variable_declaration_id(&self,
		variable_declaration: &std::rc::Rc<foliage::VariableDeclaration>) -> usize;
}
