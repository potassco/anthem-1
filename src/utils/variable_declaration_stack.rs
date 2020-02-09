pub(crate) struct VariableDeclarationStack
{
	pub free_variable_declarations: foliage::VariableDeclarations,
	bound_variable_declaration_stack: Vec<std::rc::Rc<foliage::VariableDeclarations>>,
}

impl VariableDeclarationStack
{
	pub fn new() -> Self
	{
		Self
		{
			free_variable_declarations: foliage::VariableDeclarations::new(),
			bound_variable_declaration_stack: vec![],
		}
	}

	pub fn find(&self, variable_name: &str) -> Option<std::rc::Rc<foliage::VariableDeclaration>>
	{
		for variable_declarations in self.bound_variable_declaration_stack.iter().rev()
		{
			if let Some(variable_declaration) = variable_declarations.iter().find(|x| x.name == variable_name)
			{
				return Some(std::rc::Rc::clone(&variable_declaration));
			}
		}

		if let Some(variable_declaration) = self.free_variable_declarations.iter().find(|x| x.name == variable_name)
		{
			return Some(std::rc::Rc::clone(&variable_declaration));
		}

		None
	}

	pub fn find_or_create(&mut self, variable_name: &str) -> std::rc::Rc<foliage::VariableDeclaration>
	{
		if let Some(variable_declaration) = self.find(variable_name)
		{
			return variable_declaration;
		}

		let variable_declaration = foliage::VariableDeclaration
		{
			name: variable_name.to_owned(),
		};
		let variable_declaration = std::rc::Rc::new(variable_declaration);

		self.free_variable_declarations.push(std::rc::Rc::clone(&variable_declaration));

		variable_declaration
	}

	pub fn is_empty(&self) -> bool
	{
		self.free_variable_declarations.is_empty()
			&& self.bound_variable_declaration_stack.is_empty()
	}

	pub fn push(&mut self, bound_variable_declarations: std::rc::Rc<foliage::VariableDeclarations>)
	{
		self.bound_variable_declaration_stack.push(bound_variable_declarations);
	}

	pub fn pop(&mut self) -> Result<(), crate::Error>
	{
		self.bound_variable_declaration_stack.pop().map(|_| ())
			.ok_or(crate::Error::new_logic("variable stack not in expected state"))
	}
}
