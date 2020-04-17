pub(crate) struct Definitions
{
	pub head_atom_parameters: std::rc::Rc<foliage::VariableDeclarations>,
	pub definitions: Vec<crate::ScopedFormula>,
}

type VariableDeclarationDomains
	= std::collections::BTreeMap<std::rc::Rc<foliage::VariableDeclaration>, crate::Domain>;

type VariableDeclarationIDs
	= std::collections::BTreeMap::<std::rc::Rc<foliage::VariableDeclaration>, usize>;

pub(crate) struct Context
{
	pub definitions: std::cell::RefCell<std::collections::BTreeMap::<
		std::rc::Rc<foliage::PredicateDeclaration>, Definitions>>,
	pub integrity_constraints: std::cell::RefCell<foliage::Formulas>,

	pub input_constant_declaration_domains: std::cell::RefCell<
		crate::InputConstantDeclarationDomains>,
	pub input_predicate_declarations: std::cell::RefCell<foliage::PredicateDeclarations>,

	pub function_declarations: std::cell::RefCell<foliage::FunctionDeclarations>,
	pub predicate_declarations: std::cell::RefCell<foliage::PredicateDeclarations>,
	pub variable_declaration_domains: std::cell::RefCell<VariableDeclarationDomains>,
	pub program_variable_declaration_ids: std::cell::RefCell<VariableDeclarationIDs>,
	pub integer_variable_declaration_ids: std::cell::RefCell<VariableDeclarationIDs>,
}

impl Context
{
	pub(crate) fn new() -> Self
	{
		Self
		{
			definitions: std::cell::RefCell::new(std::collections::BTreeMap::<_, _>::new()),
			integrity_constraints: std::cell::RefCell::new(vec![]),

			input_constant_declaration_domains:
				std::cell::RefCell::new(crate::InputConstantDeclarationDomains::new()),
			input_predicate_declarations:
				std::cell::RefCell::new(foliage::PredicateDeclarations::new()),

			function_declarations: std::cell::RefCell::new(foliage::FunctionDeclarations::new()),
			predicate_declarations: std::cell::RefCell::new(foliage::PredicateDeclarations::new()),
			variable_declaration_domains:
				std::cell::RefCell::new(VariableDeclarationDomains::new()),
			program_variable_declaration_ids:
				std::cell::RefCell::new(VariableDeclarationIDs::new()),
			integer_variable_declaration_ids:
				std::cell::RefCell::new(VariableDeclarationIDs::new()),
		}
	}
}

impl crate::traits::InputConstantDeclarationDomain for Context
{
	fn input_constant_declaration_domain(&self,
		declaration: &std::rc::Rc<foliage::FunctionDeclaration>) -> crate::Domain
	{
		let input_constant_declaration_domains = self.input_constant_declaration_domains.borrow();

		// Assume the program domain if not specified otherwise
		input_constant_declaration_domains.get(declaration).map(|x| *x)
			.unwrap_or(crate::Domain::Program)
	}
}

impl crate::traits::GetOrCreateFunctionDeclaration for Context
{
	fn get_or_create_function_declaration(&self, name: &str, arity: usize)
		-> std::rc::Rc<foliage::FunctionDeclaration>
	{
		let mut function_declarations = self.function_declarations.borrow_mut();

		match function_declarations.iter()
			.find(|x| x.name == name && x.arity == arity)
		{
			Some(value) => std::rc::Rc::clone(value),
			None =>
			{
				let declaration = std::rc::Rc::new(foliage::FunctionDeclaration::new(
					name.to_string(), arity));

				function_declarations.insert(std::rc::Rc::clone(&declaration));

				log::debug!("new function declaration: {}/{}", name, arity);

				declaration
			},
		}
	}
}

impl crate::traits::GetOrCreatePredicateDeclaration for Context
{
	fn get_or_create_predicate_declaration(&self, name: &str, arity: usize)
		-> std::rc::Rc<foliage::PredicateDeclaration>
	{
		let mut predicate_declarations = self.predicate_declarations.borrow_mut();

		match predicate_declarations.iter()
			.find(|x| x.name == name && x.arity == arity)
		{
			Some(value) => std::rc::Rc::clone(value),
			None =>
			{
				let declaration = std::rc::Rc::new(foliage::PredicateDeclaration::new(
					name.to_string(), arity));

				predicate_declarations.insert(std::rc::Rc::clone(&declaration));

				log::debug!("new predicate declaration: {}/{}", name, arity);

				declaration
			},
		}
	}
}

impl crate::traits::AssignVariableDeclarationDomain for Context
{
	fn assign_variable_declaration_domain(&self,
		variable_declaration: &std::rc::Rc<foliage::VariableDeclaration>, domain: crate::Domain)
	{
		let mut variable_declaration_domains = self.variable_declaration_domains.borrow_mut();

		match variable_declaration_domains.get(variable_declaration)
		{
			Some(current_domain) => assert_eq!(*current_domain, domain,
				"inconsistent variable declaration domain"),
			None =>
			{
				variable_declaration_domains
					.insert(std::rc::Rc::clone(variable_declaration).into(), domain);
			},
		}
	}
}

impl crate::traits::VariableDeclarationDomain for Context
{
	fn variable_declaration_domain(&self,
		variable_declaration: &std::rc::Rc<foliage::VariableDeclaration>)
		-> Option<crate::Domain>
	{
		let variable_declaration_domains = self.variable_declaration_domains.borrow();

		variable_declaration_domains.get(variable_declaration)
			.map(|x| *x)
	}
}

impl crate::traits::VariableDeclarationID for Context
{
	fn variable_declaration_id(&self,
		variable_declaration: &std::rc::Rc<foliage::VariableDeclaration>)
		-> usize
	{
		use crate::traits::VariableDeclarationDomain;

		let mut variable_declaration_ids = match self.variable_declaration_domain(
			variable_declaration)
		{
			Some(crate::Domain::Program) => self.program_variable_declaration_ids.borrow_mut(),
			Some(crate::Domain::Integer) => self.integer_variable_declaration_ids.borrow_mut(),
			None => panic!("all variables should be declared at this point"),
		};

		match variable_declaration_ids.get(variable_declaration)
		{
			Some(id) =>
			{
				*id
			}
			None =>
			{
				let id = variable_declaration_ids.len();
				variable_declaration_ids.insert(std::rc::Rc::clone(variable_declaration).into(), id);
				id
			},
		}
	}
}

impl foliage::format::Format for Context
{
	fn display_variable_declaration(&self, formatter: &mut std::fmt::Formatter,
		variable_declaration: &std::rc::Rc<foliage::VariableDeclaration>)
		-> std::fmt::Result
	{
		crate::output::human_readable::display_variable_declaration(self, formatter,
			variable_declaration)
	}
}
