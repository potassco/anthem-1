pub(crate) struct DomainDisplay
{
	domain: crate::translate::common::Domain,
}

pub(crate) fn display_domain(domain: crate::translate::common::Domain) -> DomainDisplay
{
	DomainDisplay
	{
		domain,
	}
}

impl std::fmt::Debug for DomainDisplay
{
	fn fmt(&self, format: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		let domain_name = match self.domain
		{
			crate::translate::common::Domain::Integer => "$int",
			crate::translate::common::Domain::Program => "object",
		};

		write!(format, "{}", domain_name)
	}
}

impl std::fmt::Display for DomainDisplay
{
	fn fmt(&self, format: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(format, "{:?}", &self)
	}
}

pub(crate) struct VariableDeclarationDisplay<'a, 'b, C>
where
	C: crate::translate::common::VariableDeclarationDomain
		+ crate::translate::common::VariableDeclarationID
{
	variable_declaration: &'a std::rc::Rc<foliage::VariableDeclaration>,
	context: &'b C,
}

pub(crate) fn display_variable_declaration<'a, 'b, C>(
	variable_declaration: &'a std::rc::Rc<foliage::VariableDeclaration>, context: &'b C)
	-> VariableDeclarationDisplay<'a, 'b, C>
where
	C: crate::translate::common::VariableDeclarationDomain
		+ crate::translate::common::VariableDeclarationID
{
	VariableDeclarationDisplay
	{
		variable_declaration,
		context,
	}
}

pub(crate) struct TermDisplay<'a, 'b, C>
{
	term: &'a foliage::Term,
	context: &'b C,
}

pub(crate) fn display_term<'a, 'b, C>(term: &'a foliage::Term, context: &'b C)
	-> TermDisplay<'a, 'b, C>
where
	C: crate::translate::common::VariableDeclarationDomain
		+ crate::translate::common::VariableDeclarationID
{
	TermDisplay
	{
		term,
		context,
	}
}

pub(crate) struct FormulaDisplay<'a, 'b, C>
{
	formula: &'a foliage::Formula,
	context: &'b C,
}

pub(crate) fn display_formula<'a, 'b, C>(formula: &'a foliage::Formula, context: &'b C)
	-> FormulaDisplay<'a, 'b, C>
where
	C: crate::translate::common::VariableDeclarationDomain
		+ crate::translate::common::VariableDeclarationID
{
	FormulaDisplay
	{
		formula,
		context,
	}
}

impl<'a, 'b, C> std::fmt::Debug for VariableDeclarationDisplay<'a, 'b, C>
where
	C: crate::translate::common::VariableDeclarationDomain
		+ crate::translate::common::VariableDeclarationID
{
	fn fmt(&self, format: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		let id = self.context.variable_declaration_id(self.variable_declaration);
		let domain = self.context.variable_declaration_domain(self.variable_declaration)
			.expect("unspecified variable domain");

		let prefix = match domain
		{
			crate::translate::common::Domain::Integer => "N",
			crate::translate::common::Domain::Program => "X",
		};

		write!(format, "{}{}", prefix, id + 1)
	}
}

impl<'a, 'b, C> std::fmt::Display for VariableDeclarationDisplay<'a, 'b, C>
where
	C: crate::translate::common::VariableDeclarationDomain
		+ crate::translate::common::VariableDeclarationID
{
	fn fmt(&self, format: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(format, "{:?}", &self)
	}
}

impl<'a, 'b, C> std::fmt::Debug for TermDisplay<'a, 'b, C>
where
	C: crate::translate::common::VariableDeclarationDomain
		+ crate::translate::common::VariableDeclarationID
{
	fn fmt(&self, format: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		let display_variable_declaration = |variable_declaration|
			display_variable_declaration(variable_declaration, self.context);
		let display_term = |term| display_term(term, self.context);

		match &self.term
		{
			foliage::Term::Boolean(true) => write!(format, "$true"),
			foliage::Term::Boolean(false) => write!(format, "$false"),
			foliage::Term::SpecialInteger(foliage::SpecialInteger::Infimum) => write!(format, "c__infimum__"),
			foliage::Term::SpecialInteger(foliage::SpecialInteger::Supremum) => write!(format, "c__supremum__"),
			foliage::Term::Integer(value) => match value.is_negative()
			{
				true => write!(format, "$uminus({})", -value),
				false => write!(format, "{}", value),
			},
			foliage::Term::String(_) => panic!("strings not supported in TPTP"),
			foliage::Term::Variable(variable) =>
				write!(format, "{:?}", display_variable_declaration(&variable.declaration)),
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
						write!(format, "{}{:?}", separator, display_term(&argument))?;

						separator = ", ";
					}

					write!(format, ")")?;
				}

				Ok(())
			},
			foliage::Term::BinaryOperation(foliage::BinaryOperation{operator: foliage::BinaryOperator::Add, left, right})
				=> write!(format, "$sum({:?}, {:?})", display_term(left), display_term(right)),
			foliage::Term::BinaryOperation(foliage::BinaryOperation{operator: foliage::BinaryOperator::Subtract, left, right})
				=> write!(format, "$difference({:?}, {:?})", display_term(left), display_term(right)),
			foliage::Term::BinaryOperation(foliage::BinaryOperation{operator: foliage::BinaryOperator::Multiply, left, right})
				=> write!(format, "$product({:?}, {:?})", display_term(left), display_term(right)),
			foliage::Term::BinaryOperation(foliage::BinaryOperation{operator: foliage::BinaryOperator::Divide, ..})
				=> panic!("division not supported with TPTP output"),
			foliage::Term::BinaryOperation(foliage::BinaryOperation{operator: foliage::BinaryOperator::Modulo, ..})
				=> panic!("modulo not supported with TPTP output"),
			foliage::Term::BinaryOperation(foliage::BinaryOperation{operator: foliage::BinaryOperator::Exponentiate, ..})
				=> panic!("exponentiation not supported with TPTP output"),
			foliage::Term::UnaryOperation(foliage::UnaryOperation{operator: foliage::UnaryOperator::Negative, argument})
				=> write!(format, "$uminus({:?})", display_term(argument)),
			foliage::Term::UnaryOperation(foliage::UnaryOperation{operator: foliage::UnaryOperator::AbsoluteValue, ..})
				=> panic!("absolute value not supported with TPTP output"),
		}
	}
}

impl<'a, 'b, C> std::fmt::Display for TermDisplay<'a, 'b, C>
where
	C: crate::translate::common::VariableDeclarationDomain
		+ crate::translate::common::VariableDeclarationID
{
	fn fmt(&self, format: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(format, "{:?}", self)
	}
}

impl<'a, 'b, C> std::fmt::Debug for FormulaDisplay<'a, 'b, C>
where
	C: crate::translate::common::VariableDeclarationDomain
		+ crate::translate::common::VariableDeclarationID
{
	fn fmt(&self, format: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		let display_variable_declaration = |variable_declaration|
			display_variable_declaration(variable_declaration, self.context);
		let display_term = |term| display_term(term, self.context);
		let display_formula = |formula| display_formula(formula, self.context);

		match &self.formula
		{
			foliage::Formula::Exists(exists) =>
			{
				write!(format, "?[")?;

				let mut separator = "";

				for parameter in exists.parameters.iter()
				{
					let parameter_domain = self.context.variable_declaration_domain(parameter)
						.expect("unspecified variable domain");

					write!(format, "{}{:?}: {}", separator, display_variable_declaration(parameter),
						display_domain(parameter_domain))?;

					separator = ", "
				}

				write!(format, "]: {:?}", display_formula(&exists.argument))?;
			},
			foliage::Formula::ForAll(for_all) =>
			{
				write!(format, "![")?;

				let mut separator = "";

				for parameter in for_all.parameters.iter()
				{
					let parameter_domain = self.context.variable_declaration_domain(parameter)
						.expect("unspecified variable domain");

					write!(format, "{}{:?}: {}", separator, display_variable_declaration(parameter),
						display_domain(parameter_domain))?;

					separator = ", "
				}

				write!(format, "]: {:?}", display_formula(&for_all.argument))?;
			},
			foliage::Formula::Not(argument) => write!(format, "~{:?}", display_formula(argument))?,
			foliage::Formula::And(arguments) =>
			{
				write!(format, "(")?;

				let mut separator = "";

				assert!(!arguments.is_empty());

				for argument in arguments
				{
					write!(format, "{}{:?}", separator, display_formula(argument))?;

					separator = " & "
				}

				write!(format, ")")?;
			},
			foliage::Formula::Or(arguments) =>
			{
				write!(format, "(")?;

				let mut separator = "";

				assert!(!arguments.is_empty());

				for argument in arguments
				{
					write!(format, "{}{:?}", separator, display_formula(argument))?;

					separator = " | "
				}

				write!(format, ")")?;
			},
			foliage::Formula::Implies(foliage::Implies{antecedent, implication})
				=> write!(format, "({:?} => {:?})", display_formula(antecedent),
					display_formula(implication))?,
			foliage::Formula::IfAndOnlyIf(foliage::IfAndOnlyIf{left, right})
				=> write!(format, "({:?} <=> {:?})", display_formula(left), display_formula(right))?,
			foliage::Formula::Compare(foliage::Compare{operator: foliage::ComparisonOperator::Less, left, right})
				=> write!(format, "$less({:?}, {:?})", display_term(left), display_term(right))?,
			foliage::Formula::Compare(foliage::Compare{operator: foliage::ComparisonOperator::LessOrEqual, left, right})
				=> write!(format, "$lesseq({:?}, {:?})", display_term(left), display_term(right))?,
			foliage::Formula::Compare(foliage::Compare{operator: foliage::ComparisonOperator::Greater, left, right})
				=> write!(format, "$greater({:?}, {:?})", display_term(left), display_term(right))?,
			foliage::Formula::Compare(foliage::Compare{operator: foliage::ComparisonOperator::GreaterOrEqual, left, right})
				=> write!(format, "$greatereq({:?}, {:?})", display_term(left), display_term(right))?,
			foliage::Formula::Compare(foliage::Compare{operator: foliage::ComparisonOperator::Equal, left, right})
				=> write!(format, "({:?} = {:?})", display_term(left), display_term(right))?,
			foliage::Formula::Compare(foliage::Compare{operator: foliage::ComparisonOperator::NotEqual, left, right})
				=> write!(format, "({:?} ~= {:?})", display_term(left), display_term(right))?,
			foliage::Formula::Boolean(true) => write!(format, "$true")?,
			foliage::Formula::Boolean(false) => write!(format, "$false")?,
			foliage::Formula::Predicate(predicate) =>
			{
				write!(format, "{}", predicate.declaration.name)?;

				if !predicate.arguments.is_empty()
				{
					write!(format, "(")?;

					let mut separator = "";

					for argument in &predicate.arguments
					{
						write!(format, "{}{:?}", separator, display_term(argument))?;

						separator = ", "
					}

					write!(format, ")")?;
				}
			},
		}

		Ok(())
	}
}

impl<'a, 'b, C> std::fmt::Display for FormulaDisplay<'a, 'b, C>
where
	C: crate::translate::common::VariableDeclarationDomain
		+ crate::translate::common::VariableDeclarationID
{
	fn fmt(&self, format: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(format, "{:?}", self)
	}
}
