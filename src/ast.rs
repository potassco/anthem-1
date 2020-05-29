pub struct FoliageFlavor;

pub struct FunctionDeclaration
{
	pub declaration: foliage::FunctionDeclaration,
	pub domain: std::cell::RefCell<crate::Domain>,
	pub is_input: std::cell::RefCell<bool>,
}

impl FunctionDeclaration
{
	pub fn is_built_in(&self) -> bool
	{
		self.declaration.name.starts_with("f__") && self.declaration.name.ends_with("__")
	}
}

impl std::cmp::PartialEq for FunctionDeclaration
{
	#[inline(always)]
	fn eq(&self, other: &Self) -> bool
	{
		self.declaration.eq(&other.declaration)
	}
}

impl std::cmp::Eq for FunctionDeclaration
{
}

impl std::cmp::PartialOrd for FunctionDeclaration
{
	#[inline(always)]
	fn partial_cmp(&self, other: &FunctionDeclaration) -> Option<std::cmp::Ordering>
	{
		self.declaration.partial_cmp(&other.declaration)
	}
}

impl std::cmp::Ord for FunctionDeclaration
{
	#[inline(always)]
	fn cmp(&self, other: &FunctionDeclaration) -> std::cmp::Ordering
	{
		self.declaration.cmp(&other.declaration)
	}
}

impl std::hash::Hash for FunctionDeclaration
{
	#[inline(always)]
	fn hash<H: std::hash::Hasher>(&self, state: &mut H)
	{
		self.declaration.hash(state)
	}
}

impl foliage::flavor::FunctionDeclaration for FunctionDeclaration
{
	fn new(name: String, arity: usize) -> Self
	{
		Self
		{
			declaration: foliage::FunctionDeclaration::new(name, arity),
			domain: std::cell::RefCell::new(crate::Domain::Program),
			is_input: std::cell::RefCell::new(false),
		}
	}

	fn display_name(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		self.declaration.display_name(formatter)
	}

	fn arity(&self) -> usize
	{
		self.declaration.arity
	}

	fn matches_signature(&self, other_name: &str, other_arity: usize) -> bool
	{
		self.declaration.matches_signature(other_name, other_arity)
	}
}

#[derive(Copy, Clone, Eq, PartialEq)]
pub enum PredicateDependencySign
{
	OnlyPositive,
	OnlyNegative,
	PositiveAndNegative,
}

impl PredicateDependencySign
{
	pub fn and_positive(self) -> Self
	{
		match self
		{
			Self::OnlyPositive => Self::OnlyPositive,
			Self::OnlyNegative
			| Self::PositiveAndNegative => Self::PositiveAndNegative,
		}
	}

	pub fn and_negative(self) -> Self
	{
		match self
		{
			Self::OnlyNegative => Self::OnlyNegative,
			Self::OnlyPositive
			| Self::PositiveAndNegative => Self::PositiveAndNegative,
		}
	}

	pub fn is_positive(&self) -> bool
	{
		match self
		{
			Self::OnlyPositive
			| Self::PositiveAndNegative => true,
			Self::OnlyNegative => false,
		}
	}

	pub fn is_negative(&self) -> bool
	{
		match self
		{
			Self::OnlyPositive => false,
			Self::OnlyNegative
			| Self::PositiveAndNegative => true,
		}
	}
}

pub type PredicateDependencies =
	std::collections::BTreeMap<std::rc::Rc<PredicateDeclaration>, PredicateDependencySign>;

pub struct PredicateDeclaration
{
	pub declaration: foliage::PredicateDeclaration,
	pub dependencies: std::cell::RefCell<Option<PredicateDependencies>>,
	pub is_input: std::cell::RefCell<bool>,
	pub is_output: std::cell::RefCell<bool>,
}

impl PredicateDeclaration
{
	pub fn tptp_statement_name(&self) -> String
	{
		format!("{}_{}", self.declaration.name, self.declaration.arity)
	}

	pub fn is_built_in(&self) -> bool
	{
		self.declaration.name.starts_with("p__") && self.declaration.name.ends_with("__")
	}

	pub fn is_public(&self) -> bool
	{
		*self.is_input.borrow() || *self.is_output.borrow()
	}

	fn collect_positive_dependencies(&self,
		mut positive_dependencies: &mut std::collections::BTreeSet<
			std::rc::Rc<crate::PredicateDeclaration>>)
	{
		let dependencies = self.dependencies.borrow();
		let dependencies = match *dependencies
		{
			Some(ref dependencies) => dependencies,
			// Input predicates don’t have completed definitions and no dependencies, so ignore them
			None => return,
		};

		for (dependency, sign) in dependencies.iter()
		{
			if positive_dependencies.contains(&*dependency)
				|| dependency.is_built_in()
				|| !sign.is_positive()
			{
				continue;
			}

			positive_dependencies.insert(std::rc::Rc::clone(dependency));

			dependency.collect_positive_dependencies(&mut positive_dependencies);
		}
	}

	fn collect_private_dependencies(&self,
		mut private_dependencies: &mut std::collections::BTreeSet<
			std::rc::Rc<crate::PredicateDeclaration>>)
	{
		let dependencies = self.dependencies.borrow();
		let dependencies = match *dependencies
		{
			Some(ref dependencies) => dependencies,
			// Input predicates don’t have completed definitions and no dependencies, so ignore them
			None => return,
		};

		for (dependency, _) in dependencies.iter()
		{
			if private_dependencies.contains(&*dependency)
				|| dependency.is_public()
				|| dependency.is_built_in()
			{
				continue;
			}

			private_dependencies.insert(std::rc::Rc::clone(dependency));

			dependency.collect_private_dependencies(&mut private_dependencies);
		}
	}

	pub fn has_positive_dependency_cycle(&self) -> bool
	{
		if self.is_built_in()
		{
			return false;
		}

		let mut positive_dependencies = std::collections::BTreeSet::new();
		self.collect_positive_dependencies(&mut positive_dependencies);

		positive_dependencies.contains(self)
	}

	pub fn has_private_dependency_cycle(&self) -> bool
	{
		if self.is_public() || self.is_built_in()
		{
			return false;
		}

		let mut private_dependencies = std::collections::BTreeSet::new();
		self.collect_private_dependencies(&mut private_dependencies);

		private_dependencies.contains(self)
	}
}

impl std::cmp::PartialEq for PredicateDeclaration
{
	#[inline(always)]
	fn eq(&self, other: &Self) -> bool
	{
		self.declaration.eq(&other.declaration)
	}
}

impl std::cmp::Eq for PredicateDeclaration
{
}

impl std::cmp::PartialOrd for PredicateDeclaration
{
	#[inline(always)]
	fn partial_cmp(&self, other: &PredicateDeclaration) -> Option<std::cmp::Ordering>
	{
		self.declaration.partial_cmp(&other.declaration)
	}
}

impl std::cmp::Ord for PredicateDeclaration
{
	#[inline(always)]
	fn cmp(&self, other: &PredicateDeclaration) -> std::cmp::Ordering
	{
		self.declaration.cmp(&other.declaration)
	}
}

impl std::hash::Hash for PredicateDeclaration
{
	#[inline(always)]
	fn hash<H: std::hash::Hasher>(&self, state: &mut H)
	{
		self.declaration.hash(state)
	}
}

impl foliage::flavor::PredicateDeclaration for PredicateDeclaration
{
	fn new(name: String, arity: usize) -> Self
	{
		Self
		{
			declaration: foliage::PredicateDeclaration::new(name, arity),
			dependencies: std::cell::RefCell::new(None),
			is_input: std::cell::RefCell::new(false),
			is_output: std::cell::RefCell::new(false),
		}
	}

	fn display_name(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		self.declaration.display_name(formatter)
	}

	fn arity(&self) -> usize
	{
		self.declaration.arity
	}

	fn matches_signature(&self, other_name: &str, other_arity: usize) -> bool
	{
		self.declaration.matches_signature(other_name, other_arity)
	}
}

#[derive(Clone)]
pub struct GeneratedVariableName
{
	pub domain: crate::Domain,
	pub id: Option<usize>,
}

#[derive(Clone)]
pub enum VariableName
{
	UserDefined(String),
	Generated(GeneratedVariableName),
}

#[derive(Clone)]
pub struct VariableDeclaration
{
	pub name: std::cell::RefCell<VariableName>,
}

impl VariableDeclaration
{
	pub fn new_generated(domain: crate::Domain) -> Self
	{
		let generated_variable_name = GeneratedVariableName
		{
			domain,
			id: None,
		};

		Self
		{
			name: std::cell::RefCell::new(VariableName::Generated(generated_variable_name)),
		}
	}

	pub fn domain(&self) -> Result<crate::Domain, crate::Error>
	{
		match *self.name.borrow()
		{
			VariableName::UserDefined(ref name) =>
			{
				let mut name_characters = name.chars();

				loop
				{
					match name_characters.next()
					{
						Some('I')
						| Some('J')
						| Some('K')
						| Some('L')
						| Some('M')
						| Some('N') => return Ok(crate::Domain::Integer),
						Some('X')
						| Some('Y')
						| Some('Z') => return Ok(crate::Domain::Program),
						Some('_') => continue,
						_ => return Err(
							crate::Error::new_variable_name_not_allowed(name.to_string())),
					}
				}
			},
			VariableName::Generated(ref generated_variable_name) =>
				Ok(generated_variable_name.domain),
		}
	}
}

impl std::cmp::PartialEq for VariableDeclaration
{
	#[inline(always)]
	fn eq(&self, other: &Self) -> bool
	{
		let l = self as *const Self;
		let r = other as *const Self;

		l.eq(&r)
	}
}

impl std::cmp::Eq for VariableDeclaration
{
}

impl std::cmp::PartialOrd for VariableDeclaration
{
	#[inline(always)]
	fn partial_cmp(&self, other: &VariableDeclaration) -> Option<std::cmp::Ordering>
	{
		let l = self as *const VariableDeclaration;
		let r = other as *const VariableDeclaration;

		l.partial_cmp(&r)
	}
}

impl std::cmp::Ord for VariableDeclaration
{
	#[inline(always)]
	fn cmp(&self, other: &VariableDeclaration) -> std::cmp::Ordering
	{
		let l = self as *const VariableDeclaration;
		let r = other as *const VariableDeclaration;

		l.cmp(&r)
	}
}

impl std::hash::Hash for VariableDeclaration
{
	#[inline(always)]
	fn hash<H: std::hash::Hasher>(&self, state: &mut H)
	{
		let p = self as *const VariableDeclaration;

		p.hash(state);
	}
}

impl foliage::flavor::VariableDeclaration for VariableDeclaration
{
	fn new(name: String) -> Self
	{
		Self
		{
			name: std::cell::RefCell::new(VariableName::UserDefined(name)),
		}
	}

	fn display_name(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		match *self.name.borrow()
		{
			VariableName::UserDefined(ref name) => write!(formatter, "{}", name),
			VariableName::Generated(ref generated_variable_name) =>
			{
				let variable_name_prefix = match generated_variable_name.domain
				{
					crate::Domain::Program => "X",
					crate::Domain::Integer => "N",
				};

				let variable_id = match generated_variable_name.id
				{
					Some(id) => id,
					None => unreachable!("all variable IDs should be assigned at this point"),
				};

				match variable_id
				{
					0 => write!(formatter, "{}", variable_name_prefix),
					_ => write!(formatter, "{}{}", variable_name_prefix, variable_id),
				}
			},
		}
	}

	fn matches_name(&self, other_name: &str) -> bool
	{
		match *self.name.borrow()
		{
			VariableName::UserDefined(ref name) => name == other_name,
			// Generated variable declarations never match user-defined variables by name
			VariableName::Generated(_) => false,
		}
	}
}

impl foliage::flavor::Flavor for FoliageFlavor
{
	type FunctionDeclaration = FunctionDeclaration;
	type PredicateDeclaration = PredicateDeclaration;
	type VariableDeclaration = VariableDeclaration;
}

pub type BinaryOperation = foliage::BinaryOperation<FoliageFlavor>;
pub type Formula = foliage::Formula<FoliageFlavor>;
pub type Formulas = foliage::Formulas<FoliageFlavor>;
pub type FunctionDeclarations = foliage::FunctionDeclarations<FoliageFlavor>;
pub type OpenFormula = foliage::OpenFormula<FoliageFlavor>;
pub type Predicate = foliage::Predicate<FoliageFlavor>;
pub type PredicateDeclarations = foliage::PredicateDeclarations<FoliageFlavor>;
pub type QuantifiedFormula = foliage::QuantifiedFormula<FoliageFlavor>;
pub type Term = foliage::Term<FoliageFlavor>;
pub type UnaryOperation = foliage::UnaryOperation<FoliageFlavor>;
pub type Variable = foliage::Variable<FoliageFlavor>;
pub type VariableDeclarationStackLayer<'p> =
	foliage::VariableDeclarationStackLayer<'p, FoliageFlavor>;
pub type VariableDeclarations = foliage::VariableDeclarations<FoliageFlavor>;
