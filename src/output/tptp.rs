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
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		let domain_name = match self.domain
		{
			crate::Domain::Integer => "$int",
			crate::Domain::Program => "object",
		};

		write!(formatter, "{}", domain_name)
	}
}

impl std::fmt::Display for DomainDisplay
{
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(formatter, "{:?}", &self)
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
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(formatter, "{}:", self.function_declaration.name)?;

		let domain = self.context.input_constant_declaration_domain(self.function_declaration);
		let domain_identifier = match domain
		{
			crate::Domain::Integer => "$int",
			crate::Domain::Program => "object",
		};

		let mut separator = "";

		if self.function_declaration.arity > 0
		{
			write!(formatter, " (")?;

			for _ in 0..self.function_declaration.arity
			{
				write!(formatter, "{}object", separator)?;
				separator = " * ";
			}

			write!(formatter, ") >")?;
		}

		write!(formatter, " {}", domain_identifier)
	}
}

impl<'a, 'b, C> std::fmt::Display for FunctionDeclarationDisplay<'a, 'b, C>
where
	C: crate::traits::InputConstantDeclarationDomain
{
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(formatter, "{:?}", &self)
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
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(formatter, "{}:", self.0.name)?;

		let mut separator = "";

		if self.0.arity > 0
		{
			write!(formatter, " (")?;

			for _ in 0..self.0.arity
			{
				write!(formatter, "{}object", separator)?;
				separator = " * ";
			}

			write!(formatter, ") >")?;
		}

		write!(formatter, " $o")
	}
}

impl<'a> std::fmt::Display for PredicateDeclarationDisplay<'a>
{
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(formatter, "{:?}", &self)
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
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		let id = self.context.variable_declaration_id(self.variable_declaration);
		let domain = self.context.variable_declaration_domain(self.variable_declaration)
			.expect("unspecified variable domain");

		let prefix = match domain
		{
			crate::Domain::Integer => "N",
			crate::Domain::Program => "X",
		};

		write!(formatter, "{}{}", prefix, id + 1)
	}
}

impl<'a, 'b, C> std::fmt::Display for VariableDeclarationDisplay<'a, 'b, C>
where
	C: crate::traits::VariableDeclarationDomain
		+ crate::traits::VariableDeclarationID
{
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(formatter, "{:?}", &self)
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
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		let display_variable_declaration = |variable_declaration|
			display_variable_declaration(variable_declaration, self.context);
		let display_term = |term| display_term(term, self.context);

		use foliage::*;

		match &self.term
		{
			Term::Boolean(true) => write!(formatter, "$true"),
			Term::Boolean(false) => write!(formatter, "$false"),
			Term::SpecialInteger(SpecialInteger::Infimum) => write!(formatter, "c__infimum__"),
			Term::SpecialInteger(SpecialInteger::Supremum) => write!(formatter, "c__supremum__"),
			Term::Integer(value) => match value.is_negative()
			{
				true => write!(formatter, "$uminus({})", -value),
				false => write!(formatter, "{}", value),
			},
			Term::String(_) => panic!("strings not supported in TPTP"),
			Term::Variable(variable) =>
				write!(formatter, "{:?}", display_variable_declaration(&variable.declaration)),
			Term::Function(function) =>
			{
				write!(formatter, "{}", function.declaration.name)?;

				assert!(function.declaration.arity == function.arguments.len(),
					"function has a different number of arguments than declared (expected {}, got {})",
					function.declaration.arity, function.arguments.len());

				if function.arguments.len() > 0
				{
					write!(formatter, "{}(", function.declaration.name)?;

					let mut separator = "";

					for argument in &function.arguments
					{
						write!(formatter, "{}{:?}", separator, display_term(&argument))?;

						separator = ", ";
					}

					write!(formatter, ")")?;
				}

				Ok(())
			},
			Term::BinaryOperation(BinaryOperation{operator: BinaryOperator::Add, left, right}) =>
				write!(formatter, "$sum({:?}, {:?})", display_term(left), display_term(right)),
			Term::BinaryOperation(BinaryOperation{operator: BinaryOperator::Subtract, left, right})
				=>
				write!(formatter, "$difference({:?}, {:?})", display_term(left),
					display_term(right)),
			Term::BinaryOperation(BinaryOperation{operator: BinaryOperator::Multiply, left, right})
				=>
				write!(formatter, "$product({:?}, {:?})", display_term(left), display_term(right)),
			Term::BinaryOperation(BinaryOperation{operator: BinaryOperator::Divide, ..}) =>
				panic!("division not supported with TPTP output"),
			Term::BinaryOperation(BinaryOperation{operator: BinaryOperator::Modulo, ..}) =>
				panic!("modulo not supported with TPTP output"),
			Term::BinaryOperation(BinaryOperation{operator: BinaryOperator::Exponentiate, ..}) =>
				panic!("exponentiation not supported with TPTP output"),
			Term::UnaryOperation(UnaryOperation{operator: UnaryOperator::Negative, argument}) =>
				write!(formatter, "$uminus({:?})", display_term(argument)),
			Term::UnaryOperation(UnaryOperation{operator: UnaryOperator::AbsoluteValue, ..}) =>
				panic!("absolute value not supported with TPTP output"),
		}
	}
}

impl<'a, 'b, C> std::fmt::Display for TermDisplay<'a, 'b, C>
where
	C: crate::traits::VariableDeclarationDomain
		+ crate::traits::VariableDeclarationID
{
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(formatter, "{:?}", self)
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
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
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

			match (!is_left_term_arithmetic || !is_right_term_arithmetic, auxiliary_predicate_name)
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
				write!(formatter, "{}(", operation_identifier)?;
			}

			match is_left_term_arithmetic && !is_right_term_arithmetic
			{
				true => write!(formatter, "f__integer__({})", display_term(left))?,
				false => write!(formatter, "{}", display_term(left))?,
			}

			match notation
			{
				crate::OperatorNotation::Prefix => write!(formatter, ", ")?,
				crate::OperatorNotation::Infix => write!(formatter, " {} ", operation_identifier)?,
			}

			match is_right_term_arithmetic && !is_left_term_arithmetic
			{
				true => write!(formatter, "f__integer__({})", display_term(right))?,
				false => write!(formatter, "{}", display_term(right))?,
			}

			if notation == crate::OperatorNotation::Prefix
			{
				write!(formatter, ")")?;
			}

			Ok(())
		};

		use foliage::*;

		match &self.formula
		{
			Formula::Exists(exists) =>
			{
				write!(formatter, "?[")?;

				let mut separator = "";

				for parameter in exists.parameters.iter()
				{
					let parameter_domain = self.context.variable_declaration_domain(parameter)
						.expect("unspecified variable domain");

					write!(formatter, "{}{:?}: {}", separator,
						display_variable_declaration(parameter), display_domain(parameter_domain))?;

					separator = ", "
				}

				write!(formatter, "]: ({:?})", display_formula(&exists.argument))?;
			},
			Formula::ForAll(for_all) =>
			{
				write!(formatter, "![")?;

				let mut separator = "";

				for parameter in for_all.parameters.iter()
				{
					let parameter_domain = self.context.variable_declaration_domain(parameter)
						.expect("unspecified variable domain");

					write!(formatter, "{}{:?}: {}", separator,
						display_variable_declaration(parameter), display_domain(parameter_domain))?;

					separator = ", "
				}

				write!(formatter, "]: ({:?})", display_formula(&for_all.argument))?;
			},
			Formula::Not(argument) => write!(formatter, "~{:?}", display_formula(argument))?,
			Formula::And(arguments) =>
			{
				write!(formatter, "(")?;

				let mut separator = "";

				assert!(!arguments.is_empty());

				for argument in arguments
				{
					write!(formatter, "{}{:?}", separator, display_formula(argument))?;

					separator = " & "
				}

				write!(formatter, ")")?;
			},
			Formula::Or(arguments) =>
			{
				write!(formatter, "(")?;

				let mut separator = "";

				assert!(!arguments.is_empty());

				for argument in arguments
				{
					write!(formatter, "{}{:?}", separator, display_formula(argument))?;

					separator = " | "
				}

				write!(formatter, ")")?;
			},
			Formula::Implies(Implies{antecedent, implication, ..}) =>
				write!(formatter, "({:?} => {:?})", display_formula(antecedent),
					display_formula(implication))?,
			Formula::IfAndOnlyIf(arguments) => match arguments.len()
			{
				0 => write!(formatter, "$true")?,
				_ =>
				{
					let parentheses_required = arguments.len() > 2;

					let mut argument_iterator = arguments.iter().peekable();
					let mut separator = "";

					while let Some(argument) = argument_iterator.next()
					{
						if let Some(next_argument) = argument_iterator.peek()
						{
							write!(formatter, "{}", separator)?;

							if parentheses_required
							{
								write!(formatter, "(")?;
							}

							write!(formatter, "{:?} <=> {:?}", display_formula(argument),
								display_formula(next_argument))?;

							if parentheses_required
							{
								write!(formatter, ")")?;
							}

							separator = " & ";
						}
					}
				},
			},
			Formula::Compare(Compare{operator: ComparisonOperator::Less, left, right}) =>
				display_compare(left, right, crate::OperatorNotation::Prefix, "$less",
					Some("p__less__"))?,
			Formula::Compare(Compare{operator: ComparisonOperator::LessOrEqual, left, right}) =>
				display_compare(left, right, crate::OperatorNotation::Prefix, "$lesseq",
					Some("p__less_equal__"))?,
			Formula::Compare(Compare{operator: ComparisonOperator::Greater, left, right}) =>
				display_compare(left, right, crate::OperatorNotation::Prefix, "$greater",
					Some("p__greater__"))?,
			Formula::Compare(Compare{operator: ComparisonOperator::GreaterOrEqual, left, right}) =>
				display_compare(left, right, crate::OperatorNotation::Prefix, "$greatereq",
					Some("p__greater_equal__"))?,
			Formula::Compare(Compare{operator: ComparisonOperator::Equal, left, right}) =>
				display_compare(left, right, crate::OperatorNotation::Infix, "=", None)?,
			Formula::Compare(Compare{operator: ComparisonOperator::NotEqual, left, right}) =>
				display_compare(left, right, crate::OperatorNotation::Infix, "!=", None)?,
			Formula::Boolean(true) => write!(formatter, "$true")?,
			Formula::Boolean(false) => write!(formatter, "$false")?,
			Formula::Predicate(predicate) =>
			{
				write!(formatter, "{}", predicate.declaration.name)?;

				if !predicate.arguments.is_empty()
				{
					write!(formatter, "(")?;

					let mut separator = "";

					for argument in &predicate.arguments
					{
						write!(formatter, "{}", separator)?;

						let is_argument_arithmetic =
							crate::is_term_arithmetic(argument, self.context)
								.expect("could not determine whether term is arithmetic");

						match is_argument_arithmetic
						{
							true => write!(formatter, "f__integer__({})", display_term(argument))?,
							false => write!(formatter, "{}", display_term(argument))?,
						}

						separator = ", "
					}

					write!(formatter, ")")?;
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
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(formatter, "{:?}", self)
	}
}
