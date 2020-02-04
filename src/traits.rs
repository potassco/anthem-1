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

pub(crate) trait GetOrCreateFunctionDeclaration
{
	fn get_or_create_function_declaration(&self, name: &str, arity: usize)
		-> std::rc::Rc<foliage::FunctionDeclaration>;
}

pub(crate) trait GetOrCreatePredicateDeclaration
{
	fn get_or_create_predicate_declaration(&self, name: &str, arity: usize)
		-> std::rc::Rc<foliage::PredicateDeclaration>;
}

pub(crate) trait GetOrCreateVariableDeclaration
{
	fn get_or_create_variable_declaration(&self, name: &str)
		-> std::rc::Rc<foliage::VariableDeclaration>;
}
