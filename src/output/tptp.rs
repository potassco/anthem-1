pub(crate) struct DomainDisplay
{
	domain: crate::Domain,
}

pub(crate) fn display_domain(domain: crate::Domain) -> DomainDisplay
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
			crate::Domain::Integer => "$int",
			crate::Domain::Program => "object",
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

pub(crate) struct FunctionDeclarationDisplay<'a, 'b, C>
where
	C: crate::traits::InputConstantDeclarationDomain
{
	function_declaration: &'a std::rc::Rc<foliage::FunctionDeclaration>,
	context: &'b C,
}

pub(crate) fn display_function_declaration<'a, 'b, C>(
	function_declaration: &'a std::rc::Rc<foliage::FunctionDeclaration>, context: &'b C)
	-> FunctionDeclarationDisplay<'a, 'b, C>
where
	C: crate::traits::InputConstantDeclarationDomain
{
	FunctionDeclarationDisplay
	{
		function_declaration,
		context,
	}
}

impl<'a, 'b, C> std::fmt::Debug for FunctionDeclarationDisplay<'a, 'b, C>
where
	C: crate::traits::InputConstantDeclarationDomain
{
	fn fmt(&self, format: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(format, "{}:", self.function_declaration.name)?;

		let domain = self.context.input_constant_declaration_domain(self.function_declaration);
		let domain_identifier = match domain
		{
			crate::Domain::Integer => "$int",
			crate::Domain::Program => "object",
		};

		let mut separator = "";

		if self.function_declaration.arity > 0
		{
			write!(format, " (")?;

			for _ in 0..self.function_declaration.arity
			{
				write!(format, "{}object", separator)?;
				separator = " * ";
			}

			write!(format, ") >")?;
		}

		write!(format, " {}", domain_identifier)
	}
}

impl<'a, 'b, C> std::fmt::Display for FunctionDeclarationDisplay<'a, 'b, C>
where
	C: crate::traits::InputConstantDeclarationDomain
{
	fn fmt(&self, format: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(format, "{:?}", &self)
	}
}

pub(crate) struct PredicateDeclarationDisplay<'a>(&'a std::rc::Rc<foliage::PredicateDeclaration>);

pub(crate) fn display_predicate_declaration<'a>(
	predicate_declaration: &'a std::rc::Rc<foliage::PredicateDeclaration>)
	-> PredicateDeclarationDisplay<'a>
{
	PredicateDeclarationDisplay(predicate_declaration)
}

impl<'a> std::fmt::Debug for PredicateDeclarationDisplay<'a>
{
	fn fmt(&self, format: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(format, "{}:", self.0.name)?;

		let mut separator = "";

		if self.0.arity > 0
		{
			write!(format, " (")?;

			for _ in 0..self.0.arity
			{
				write!(format, "{}object", separator)?;
				separator = " * ";
			}

			write!(format, ") >")?;
		}

		write!(format, " $o")
	}
}

impl<'a> std::fmt::Display for PredicateDeclarationDisplay<'a>
{
	fn fmt(&self, format: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(format, "{:?}", &self)
	}
}

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

pub(crate) struct TermDisplay<'a, 'b, C>
{
	term: &'a foliage::Term,
	context: &'b C,
}

pub(crate) fn display_term<'a, 'b, C>(term: &'a foliage::Term, context: &'b C)
	-> TermDisplay<'a, 'b, C>
where
	C: crate::traits::VariableDeclarationDomain
		+ crate::traits::VariableDeclarationID
{
	TermDisplay
	{
		term,
		context,
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
	C: crate::traits::VariableDeclarationDomain
		+ crate::traits::VariableDeclarationID
{
	fn fmt(&self, format: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(format, "{:?}", self)
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
	C: crate::traits::VariableDeclarationDomain
		+ crate::traits::VariableDeclarationID
{
	FormulaDisplay
	{
		formula,
		context,
	}
}

impl<'a, 'b, C> std::fmt::Debug for FormulaDisplay<'a, 'b, C>
where
	C: crate::traits::InputConstantDeclarationDomain
		+ crate::traits::VariableDeclarationDomain
		+ crate::traits::VariableDeclarationID
{
	fn fmt(&self, format: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		let display_variable_declaration = |variable_declaration|
			display_variable_declaration(variable_declaration, self.context);
		let display_term = |term| display_term(term, self.context);
		let display_formula = |formula| display_formula(formula, self.context);

		let mut display_compare = |left, right, notation, integer_operator_name,
			auxiliary_predicate_name| -> std::fmt::Result
		{
			let mut notation = notation;
			let mut operation_identifier = integer_operator_name;

			let is_left_term_arithmetic = crate::is_term_arithmetic(left, self.context)
				.expect("could not determine whether term is arithmetic");
			let is_right_term_arithmetic = crate::is_term_arithmetic(right, self.context)
				.expect("could not determine whether term is arithmetic");

			match (!is_left_term_arithmetic && !is_right_term_arithmetic,
				auxiliary_predicate_name)
			{
				(true, Some(auxiliary_predicate_name)) =>
				{
					notation = crate::OperatorNotation::Prefix;
					operation_identifier = auxiliary_predicate_name;
				},
				_ => (),
			}

			if notation == crate::OperatorNotation::Prefix
			{
				write!(format, "{}(", operation_identifier)?;
			}

			match is_left_term_arithmetic && !is_right_term_arithmetic
			{
				true => write!(format, "f__integer__({})", display_term(left))?,
				false => write!(format, "{}", display_term(left))?,
			}

			match notation
			{
				crate::OperatorNotation::Prefix => write!(format, ", ")?,
				crate::OperatorNotation::Infix => write!(format, " {} ", operation_identifier)?,
			}

			match is_right_term_arithmetic && !is_left_term_arithmetic
			{
				true => write!(format, "f__integer__({})", display_term(right))?,
				false => write!(format, "{}", display_term(right))?,
			}

			if notation == crate::OperatorNotation::Prefix
			{
				write!(format, ")")?;
			}

			Ok(())
		};

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
			foliage::Formula::Implies(foliage::Implies{antecedent, implication, ..})
				=> write!(format, "({:?} => {:?})", display_formula(antecedent),
					display_formula(implication))?,
			foliage::Formula::IfAndOnlyIf(arguments) => match arguments.len()
			{
				0 => write!(format, "$true")?,
				_ =>
				{
					let mut separator = "";
					let parentheses_required = arguments.len() > 2;

					let mut argument_iterator = arguments.iter().peekable();

					while let Some(argument) = argument_iterator.next()
					{
						if let Some(next_argument) = argument_iterator.peek()
						{
							write!(format, "{}", separator)?;

							if parentheses_required
							{
								write!(format, "(")?;
							}

							write!(format, "{:?} <=> {:?}", display_formula(argument),
								display_formula(next_argument))?;

							if parentheses_required
							{
								write!(format, ")")?;
							}

							separator = " & ";
						}
					}
				},
			},
			foliage::Formula::Compare(
				foliage::Compare{operator: foliage::ComparisonOperator::Less, left, right})
				=> display_compare(left, right, crate::OperatorNotation::Prefix, "$less",
					Some("p__less__"))?,
			foliage::Formula::Compare(
				foliage::Compare{operator: foliage::ComparisonOperator::LessOrEqual, left, right})
				=> display_compare(left, right, crate::OperatorNotation::Prefix, "$lesseq",
					Some("p__less_equal__"))?,
			foliage::Formula::Compare(
				foliage::Compare{operator: foliage::ComparisonOperator::Greater, left, right})
				=> display_compare(left, right, crate::OperatorNotation::Prefix, "$greater",
					Some("p__greater__"))?,
			foliage::Formula::Compare(
				foliage::Compare{operator: foliage::ComparisonOperator::GreaterOrEqual, left, right})
				=> display_compare(left, right, crate::OperatorNotation::Prefix, "$greatereq",
					Some("p__greater_equal__"))?,
			foliage::Formula::Compare(
				foliage::Compare{operator: foliage::ComparisonOperator::Equal, left, right})
				=> display_compare(left, right, crate::OperatorNotation::Infix, "=", None)?,
			foliage::Formula::Compare(
				foliage::Compare{operator: foliage::ComparisonOperator::NotEqual, left, right})
				=> display_compare(left, right, crate::OperatorNotation::Infix, "!=", None)?,
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
	C: crate::traits::InputConstantDeclarationDomain
		+ crate::traits::VariableDeclarationDomain
		+ crate::traits::VariableDeclarationID
{
	fn fmt(&self, format: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(format, "{:?}", self)
	}
}
