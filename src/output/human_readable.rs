pub(crate) struct VariableDeclarationDisplay<'a, 'b, C>
where
	C: crate::traits::VariableDeclarationDomain
		+ crate::traits::VariableDeclarationID
{
	variable_declaration: &'a std::rc::Rc<foliage::VariableDeclaration>,
	context: &'b C,
}

pub(crate) fn display_variable_declaration<'a, 'b, C>(
	variable_declaration: &'a std::rc::Rc<foliage::VariableDeclaration>, context: &'b C)
	-> VariableDeclarationDisplay<'a, 'b, C>
where
	C: crate::traits::VariableDeclarationDomain
		+ crate::traits::VariableDeclarationID
{
	VariableDeclarationDisplay
	{
		variable_declaration,
		context,
	}
}

pub(crate) struct TermDisplay<'a, 'b, C>
{
	parent_precedence: Option<i32>,
	term: &'a foliage::Term,
	context: &'b C,
}

pub(crate) fn display_term<'a, 'b, C>(term: &'a foliage::Term, parent_precedence: Option<i32>,
	context: &'b C)
	-> TermDisplay<'a, 'b, C>
{
	TermDisplay
	{
		parent_precedence,
		term,
		context,
	}
}

pub(crate) struct FormulaDisplay<'a, 'b, C>
{
	parent_precedence: Option<i32>,
	formula: &'a foliage::Formula,
	context: &'b C,
}

pub(crate) fn display_formula<'a, 'b, C>(formula: &'a foliage::Formula,
	parent_precedence: Option<i32>, context: &'b C)
	-> FormulaDisplay<'a, 'b, C>
where
	C: crate::traits::VariableDeclarationDomain
		+ crate::traits::VariableDeclarationID
{
	FormulaDisplay
	{
		parent_precedence,
		formula,
		context,
	}
}

trait Precedence
{
	fn precedence(&self) -> i32;
}

impl Precedence for foliage::Term
{
	fn precedence(&self) -> i32
	{
		match &self
		{
			Self::Boolean(_)
			| Self::Function(_)
			| Self::SpecialInteger(_)
			| Self::Integer(_)
			| Self::String(_)
			| Self::Variable(_)
				=> 0,
			Self::UnaryOperation(foliage::UnaryOperation{operator: foliage::UnaryOperator::Negative, ..})
				=> 1,
			Self::BinaryOperation(foliage::BinaryOperation{operator: foliage::BinaryOperator::Exponentiate, ..})
				=> 2,
			Self::BinaryOperation(foliage::BinaryOperation{operator: foliage::BinaryOperator::Multiply, ..})
			| Self::BinaryOperation(foliage::BinaryOperation{operator: foliage::BinaryOperator::Divide, ..})
			| Self::BinaryOperation(foliage::BinaryOperation{operator: foliage::BinaryOperator::Modulo, ..})
				=> 3,
			Self::BinaryOperation(foliage::BinaryOperation{operator: foliage::BinaryOperator::Add, ..})
			| Self::BinaryOperation(foliage::BinaryOperation{operator: foliage::BinaryOperator::Subtract, ..})
				=> 4,
			Self::UnaryOperation(foliage::UnaryOperation{operator: foliage::UnaryOperator::AbsoluteValue, ..})
				=> 5,
		}
	}
}

impl Precedence for foliage::Formula
{
	fn precedence(&self) -> i32
	{
		match &self
		{
			Self::Predicate(_)
			| Self::Boolean(_)
			| Self::Compare(_)
				=> 0,
			Self::Exists(_)
			| Self::ForAll(_)
				=> 1,
			Self::Not(_)
				=> 2,
			Self::And(_)
				=> 3,
			Self::Or(_)
				=> 4,
			Self::Implies(_)
				=> 5,
			Self::IfAndOnlyIf(_)
				=> 6,
		}
	}
}

impl<'a, 'b, C> std::fmt::Debug for VariableDeclarationDisplay<'a, 'b, C>
where
	C: crate::traits::VariableDeclarationDomain
		+ crate::traits::VariableDeclarationID
{
	fn fmt(&self, format: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		let id = self.context.variable_declaration_id(self.variable_declaration);
		let domain = self.context.variable_declaration_domain(self.variable_declaration)
			.expect("unspecified variable domain");

		let prefix = match domain
		{
			crate::Domain::Integer => "N",
			crate::Domain::Program => "X",
		};

		write!(format, "{}{}", prefix, id + 1)
	}
}

impl<'a, 'b, C> std::fmt::Display for VariableDeclarationDisplay<'a, 'b, C>
where
	C: crate::traits::VariableDeclarationDomain
		+ crate::traits::VariableDeclarationID
{
	fn fmt(&self, format: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(format, "{:?}", &self)
	}
}

impl<'a, 'b, C> std::fmt::Debug for TermDisplay<'a, 'b, C>
where
	C: crate::traits::VariableDeclarationDomain
		+ crate::traits::VariableDeclarationID
{
	fn fmt(&self, format: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		let display_variable_declaration = |variable_declaration|
			display_variable_declaration(variable_declaration, self.context);
		let display_term = |term, precedence| display_term(term, precedence, self.context);

		let precedence = self.term.precedence();
		let requires_parentheses = match self.parent_precedence
		{
			Some(parent_precedence) => precedence > parent_precedence,
			None => false,
		};
		let precedence = Some(precedence);

		if requires_parentheses
		{
			write!(format, "(")?;
		}

		match &self.term
		{
			foliage::Term::Boolean(true) => write!(format, "true"),
			foliage::Term::Boolean(false) => write!(format, "false"),
			foliage::Term::SpecialInteger(foliage::SpecialInteger::Infimum) => write!(format, "#inf"),
			foliage::Term::SpecialInteger(foliage::SpecialInteger::Supremum) => write!(format, "#sup"),
			foliage::Term::Integer(value) => write!(format, "{}", value),
			foliage::Term::String(value) => write!(format, "\"{}\"", value),
			foliage::Term::Variable(variable) => write!(format, "{:?}",
				display_variable_declaration(&variable.declaration)),
			foliage::Term::Function(function) =>
			{
				write!(format, "{}", function.declaration.name)?;

				assert!(function.declaration.arity == function.arguments.len(),
					"function has a different number of arguments than declared (expected {}, got {})",
					function.declaration.arity, function.arguments.len());

				if function.arguments.len() > 0
				{
					write!(format, "{}(", function.declaration.name)?;

					let mut separator = "";

					for argument in &function.arguments
					{
						write!(format, "{}{:?}", separator, display_term(&argument, None))?;

						separator = ", ";
					}

					write!(format, ")")?;
				}

				Ok(())
			},
			foliage::Term::BinaryOperation(foliage::BinaryOperation{operator: foliage::BinaryOperator::Add, left, right})
				=> write!(format, "{:?} + {:?}", display_term(left, precedence), display_term(right, precedence)),
			foliage::Term::BinaryOperation(foliage::BinaryOperation{operator: foliage::BinaryOperator::Subtract, left, right})
				=> write!(format, "{:?} - {:?}", display_term(left, precedence), display_term(right, precedence)),
			foliage::Term::BinaryOperation(foliage::BinaryOperation{operator: foliage::BinaryOperator::Multiply, left, right})
				=> write!(format, "{:?} * {:?}", display_term(left, precedence), display_term(right, precedence)),
			foliage::Term::BinaryOperation(foliage::BinaryOperation{operator: foliage::BinaryOperator::Divide, left, right})
				=> write!(format, "{:?} / {:?}", display_term(left, precedence), display_term(right, precedence)),
			foliage::Term::BinaryOperation(foliage::BinaryOperation{operator: foliage::BinaryOperator::Modulo, left, right})
				=> write!(format, "{:?} % {:?}", display_term(left, precedence), display_term(right, precedence)),
			foliage::Term::BinaryOperation(foliage::BinaryOperation{operator: foliage::BinaryOperator::Exponentiate, left, right})
				=> write!(format, "{:?} ** {:?}", display_term(left, precedence), display_term(right, precedence)),
			foliage::Term::UnaryOperation(foliage::UnaryOperation{operator: foliage::UnaryOperator::Negative, argument})
				=> write!(format, "-{:?}", display_term(argument, precedence)),
			foliage::Term::UnaryOperation(foliage::UnaryOperation{operator: foliage::UnaryOperator::AbsoluteValue, argument})
				=> write!(format, "|{:?}|", display_term(argument, precedence)),
		}?;

		if requires_parentheses
		{
			write!(format, ")")?;
		}

		Ok(())
	}
}

impl<'a, 'b, C> std::fmt::Display for TermDisplay<'a, 'b, C>
where
	C: crate::traits::VariableDeclarationDomain
		+ crate::traits::VariableDeclarationID
{
	fn fmt(&self, format: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(format, "{:?}", self)
	}
}

impl<'a, 'b, C> std::fmt::Debug for FormulaDisplay<'a, 'b, C>
where
	C: crate::traits::VariableDeclarationDomain
		+ crate::traits::VariableDeclarationID
{
	fn fmt(&self, format: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		let display_variable_declaration = |variable_declaration|
			display_variable_declaration(variable_declaration, self.context);
		let display_term = |term, precedence| display_term(term, precedence, self.context);
		let display_formula = |formula, precedence| display_formula(formula, precedence, self.context);

		let precedence = self.formula.precedence();
		let requires_parentheses = match self.parent_precedence
		{
			Some(parent_precedence) => precedence > parent_precedence,
			None => false,
		};
		let precedence = Some(precedence);

		if requires_parentheses
		{
			write!(format, "(")?;
		}

		match &self.formula
		{
			foliage::Formula::Exists(exists) =>
			{
				assert!(!exists.parameters.is_empty());

				write!(format, "exists")?;

				let mut separator = " ";

				for parameter in exists.parameters.iter()
				{
					write!(format, "{}{:?}", separator, display_variable_declaration(parameter))?;

					separator = ", "
				}

				write!(format, " {:?}", display_formula(&exists.argument, precedence))?;
			},
			foliage::Formula::ForAll(for_all) =>
			{
				assert!(!for_all.parameters.is_empty());

				write!(format, "forall")?;

				let mut separator = " ";

				for parameter in for_all.parameters.iter()
				{
					write!(format, "{}{:?}", separator, display_variable_declaration(parameter))?;

					separator = ", "
				}

				write!(format, " {:?}", display_formula(&for_all.argument, precedence))?;
			},
			foliage::Formula::Not(argument) => write!(format, "not {:?}", display_formula(argument, precedence))?,
			foliage::Formula::And(arguments) =>
			{
				let mut separator = "";

				assert!(!arguments.is_empty());

				for argument in arguments
				{
					write!(format, "{}{:?}", separator, display_formula(argument, precedence))?;

					separator = " and "
				}
			},
			foliage::Formula::Or(arguments) =>
			{
				let mut separator = "";

				assert!(!arguments.is_empty());

				for argument in arguments
				{
					write!(format, "{}{:?}", separator, display_formula(argument, precedence))?;

					separator = " or "
				}
			},
			foliage::Formula::Implies(foliage::Implies{antecedent, implication})
				=> write!(format, "{:?} -> {:?}", display_formula(antecedent, precedence), display_formula(implication, precedence))?,
			foliage::Formula::IfAndOnlyIf(foliage::IfAndOnlyIf{left, right})
				=> write!(format, "{:?} <-> {:?}", display_formula(left, precedence), display_formula(right, precedence))?,
			foliage::Formula::Compare(foliage::Compare{operator: foliage::ComparisonOperator::Less, left, right})
				=> write!(format, "{:?} < {:?}", display_term(left, None), display_term(right, None))?,
			foliage::Formula::Compare(foliage::Compare{operator: foliage::ComparisonOperator::LessOrEqual, left, right})
				=> write!(format, "{:?} <= {:?}", display_term(left, None), display_term(right, None))?,
			foliage::Formula::Compare(foliage::Compare{operator: foliage::ComparisonOperator::Greater, left, right})
				=> write!(format, "{:?} > {:?}", display_term(left, None), display_term(right, None))?,
			foliage::Formula::Compare(foliage::Compare{operator: foliage::ComparisonOperator::GreaterOrEqual, left, right})
				=> write!(format, "{:?} >= {:?}", display_term(left, None), display_term(right, None))?,
			foliage::Formula::Compare(foliage::Compare{operator: foliage::ComparisonOperator::Equal, left, right})
				=> write!(format, "{:?} = {:?}", display_term(left, None), display_term(right, None))?,
			foliage::Formula::Compare(foliage::Compare{operator: foliage::ComparisonOperator::NotEqual, left, right})
				=> write!(format, "{:?} != {:?}", display_term(left, None), display_term(right, None))?,
			foliage::Formula::Boolean(true) => write!(format, "#true")?,
			foliage::Formula::Boolean(false) => write!(format, "#false")?,
			foliage::Formula::Predicate(predicate) =>
			{
				write!(format, "{}", predicate.declaration.name)?;

				if !predicate.arguments.is_empty()
				{
					write!(format, "(")?;

					let mut separator = "";

					for argument in &predicate.arguments
					{
						write!(format, "{}{:?}", separator, display_term(argument, None))?;

						separator = ", "
					}

					write!(format, ")")?;
				}
			},
		}

		if requires_parentheses
		{
			write!(format, ")")?;
		}

		Ok(())
	}
}

impl<'a, 'b, C> std::fmt::Display for FormulaDisplay<'a, 'b, C>
where
	C: crate::traits::VariableDeclarationDomain
		+ crate::traits::VariableDeclarationID
{
	fn fmt(&self, format: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(format, "{:?}", self)
	}
}
