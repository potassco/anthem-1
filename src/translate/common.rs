mod choose_value_in_term;

pub(crate) use choose_value_in_term::*;

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

pub(crate) fn translate_binary_operator(binary_operator: clingo::ast::BinaryOperator)
	-> Result<foliage::BinaryOperator, crate::Error>
{
	match binary_operator
	{
		clingo::ast::BinaryOperator::And
		| clingo::ast::BinaryOperator::Or
		| clingo::ast::BinaryOperator::Xor
			=> return Err(crate::Error::new_unsupported_language_feature("binary logical operators")),
		clingo::ast::BinaryOperator::Plus
			=> Ok(foliage::BinaryOperator::Add),
		clingo::ast::BinaryOperator::Minus
			=> Ok(foliage::BinaryOperator::Subtract),
		clingo::ast::BinaryOperator::Multiplication
			=> Ok(foliage::BinaryOperator::Multiply),
		clingo::ast::BinaryOperator::Division
			=> Ok(foliage::BinaryOperator::Divide),
		clingo::ast::BinaryOperator::Modulo
			=> Ok(foliage::BinaryOperator::Modulo),
		clingo::ast::BinaryOperator::Power
			=> Ok(foliage::BinaryOperator::Exponentiate),
	}
}

pub(crate) fn translate_comparison_operator(comparison_operator: clingo::ast::ComparisonOperator)
	-> foliage::ComparisonOperator
{
	match comparison_operator
	{
		clingo::ast::ComparisonOperator::GreaterThan
			=> foliage::ComparisonOperator::Greater,
		clingo::ast::ComparisonOperator::LessThan
			=> foliage::ComparisonOperator::Less,
		clingo::ast::ComparisonOperator::LessEqual
			=> foliage::ComparisonOperator::LessOrEqual,
		clingo::ast::ComparisonOperator::GreaterEqual
			=> foliage::ComparisonOperator::GreaterOrEqual,
		clingo::ast::ComparisonOperator::NotEqual
			=> foliage::ComparisonOperator::NotEqual,
		clingo::ast::ComparisonOperator::Equal
			=> foliage::ComparisonOperator::Equal,
	}
}
