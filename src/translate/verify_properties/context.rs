pub struct ScopedFormula
{
	free_variable_declarations: foliage::VariableDeclarations,
	formula: foliage::Formula,
}

pub struct Context
{
	scoped_formulas: Vec<ScopedFormula>,
	function_declarations: foliage::FunctionDeclarations,
	predicate_declarations: foliage::PredicateDeclarations,
	variable_declaration_stack: foliage::VariableDeclarationStack,
}

impl Context
{
	pub fn new() -> Self
	{
		Self
		{
			scoped_formulas: vec![],
			function_declarations: foliage::FunctionDeclarations::new(),
			predicate_declarations: foliage::PredicateDeclarations::new(),
			variable_declaration_stack: foliage::VariableDeclarationStack::new(),
		}
	}

	pub fn find_or_create_predicate_declaration(&mut self, name: &str, arity: usize)
		-> std::rc::Rc<foliage::PredicateDeclaration>
	{
		match self.predicate_declarations.iter()
			.find(|x| x.name == name && x.arity == arity)
		{
			Some(value) => std::rc::Rc::clone(value),
			None =>
			{
				let declaration = foliage::PredicateDeclaration
				{
					name: name.to_owned(),
					arity,
				};
				let declaration = std::rc::Rc::new(declaration);

				self.predicate_declarations.insert(std::rc::Rc::clone(&declaration));

				log::debug!("new predicate declaration: {}/{}", name, arity);

				declaration
			},
		}
	}

	pub fn find_or_create_function_declaration(&mut self, name: &str, arity: usize)
		-> std::rc::Rc<foliage::FunctionDeclaration>
	{
		match self.function_declarations.iter()
			.find(|x| x.name == name && x.arity == arity)
		{
			Some(value) => std::rc::Rc::clone(value),
			None =>
			{
				let declaration = foliage::FunctionDeclaration
				{
					name: name.to_owned(),
					arity,
				};
				let declaration = std::rc::Rc::new(declaration);

				self.function_declarations.insert(std::rc::Rc::clone(&declaration));

				log::debug!("new function declaration: {}/{}", name, arity);

				declaration
			},
		}
	}
}
