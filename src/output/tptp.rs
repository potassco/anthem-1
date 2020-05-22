use foliage::flavor::{FunctionDeclaration as _, PredicateDeclaration as _,
	 VariableDeclaration as _};

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

pub(crate) struct FunctionDeclarationDisplay<'a>(&'a crate::FunctionDeclaration);

pub(crate) fn display_function_declaration<'a>(function_declaration: &'a crate::FunctionDeclaration)
	-> FunctionDeclarationDisplay<'a>
{
	FunctionDeclarationDisplay(function_declaration)
}

impl<'a> std::fmt::Debug for FunctionDeclarationDisplay<'a>
{
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		self.0.display_name(formatter)?;
		write!(formatter, ":")?;

		let domain_identifier = match *self.0.domain.borrow()
		{
			crate::Domain::Integer => "$int",
			crate::Domain::Program => "object",
		};

		let mut separator = "";

		if self.0.arity() > 0
		{
			write!(formatter, " (")?;

			for _ in 0..self.0.arity()
			{
				write!(formatter, "{}object", separator)?;
				separator = " * ";
			}

			write!(formatter, ") >")?;
		}

		write!(formatter, " {}", domain_identifier)
	}
}

impl<'a> std::fmt::Display for FunctionDeclarationDisplay<'a>
{
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(formatter, "{:?}", &self)
	}
}

pub(crate) struct PredicateDeclarationDisplay<'a>(&'a crate::PredicateDeclaration);

pub(crate) fn display_predicate_declaration<'a>(
	predicate_declaration: &'a crate::PredicateDeclaration)
	-> PredicateDeclarationDisplay<'a>
{
	PredicateDeclarationDisplay(predicate_declaration)
}

impl<'a> std::fmt::Debug for PredicateDeclarationDisplay<'a>
{
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		self.0.display_name(formatter)?;
		write!(formatter, ":")?;

		let mut separator = "";

		if self.0.arity() > 0
		{
			write!(formatter, " (")?;

			for _ in 0..self.0.arity()
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

pub(crate) struct TermDisplay<'a>(&'a crate::Term);

pub(crate) fn display_term<'a>(term: &'a crate::Term) -> TermDisplay<'a>
{
	TermDisplay(term)
}

impl<'a> std::fmt::Debug for TermDisplay<'a>
{
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		use foliage::*;

		match &self.0
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
			Term::Variable(variable) => variable.declaration.display_name(formatter),
			Term::Function(function) =>
			{
				function.declaration.display_name(formatter)?;

				assert!(function.declaration.arity() == function.arguments.len(),
					"function has a different number of arguments than declared (expected {}, got {})",
					function.declaration.arity(), function.arguments.len());

				if function.arguments.len() > 0
				{
					function.declaration.display_name(formatter)?;
					write!(formatter, "(")?;

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

impl<'a> std::fmt::Display for TermDisplay<'a>
{
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(formatter, "{:?}", self)
	}
}

pub(crate) struct FormulaDisplay<'a>(&'a crate::Formula);

pub(crate) fn display_formula<'a>(formula: &'a crate::Formula) -> FormulaDisplay<'a>
{
	FormulaDisplay(formula)
}

impl<'a> std::fmt::Debug for FormulaDisplay<'a>
{
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		let mut display_compare = |left, right, notation, integer_operator_name,
			auxiliary_predicate_name| -> std::fmt::Result
		{
			let mut notation = notation;
			let mut operation_identifier = integer_operator_name;

			let is_left_term_arithmetic = crate::is_term_arithmetic(left)
				.expect("could not determine whether term is arithmetic");
			let is_right_term_arithmetic = crate::is_term_arithmetic(right)
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

		match &self.0
		{
			// TODO: handle cases with 0 parameters properly
			Formula::Exists(quantified_formula)
			| Formula::ForAll(quantified_formula) =>
			{
				let operator_symbol = match &self.0
				{
					Formula::Exists(_) => "?",
					Formula::ForAll(_) => "!",
					_ => unreachable!(),
				};

				write!(formatter, "{}[", operator_symbol)?;

				let mut separator = "";

				for parameter in quantified_formula.parameters.iter()
				{
					let domain = match parameter.domain()
					{
						Ok(domain) => domain,
						Err(_) => unreachable!(
							"all variable domains should have been checked at this point"),
					};

					write!(formatter, "{}", separator)?;
					parameter.display_name(formatter)?;
					write!(formatter, ": {}", display_domain(domain))?;

					separator = ", "
				}

				write!(formatter, "]: ({:?})", display_formula(&quantified_formula.argument))?;
			},
			Formula::Not(argument) => write!(formatter, "~{:?}", display_formula(argument))?,
			// TODO: handle cases with < 2 arguments properly
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
			// TODO: handle cases with < 2 arguments properly
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
			// TODO: handle cases with < 2 arguments properly
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
				predicate.declaration.display_name(formatter)?;

				if !predicate.arguments.is_empty()
				{
					write!(formatter, "(")?;

					let mut separator = "";

					for argument in &predicate.arguments
					{
						write!(formatter, "{}", separator)?;

						let is_argument_arithmetic = crate::is_term_arithmetic(argument)
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

impl<'a> std::fmt::Display for FormulaDisplay<'a>
{
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(formatter, "{:?}", self)
	}
}
