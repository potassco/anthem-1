pub(crate) struct VariableDeclarationDisplay<'a>
{
	variable_declaration: &'a foliage::VariableDeclaration,
}

pub(crate) struct TermDisplay<'a>
{
	term: &'a foliage::Term,
}

pub(crate) struct FormulaDisplay<'a>
{
	formula: &'a foliage::Formula,
}

pub(crate) fn display_variable_declaration<'a>(variable_declaration: &'a foliage::VariableDeclaration)
	-> VariableDeclarationDisplay<'a>
{
	VariableDeclarationDisplay
	{
		variable_declaration,
	}
}

pub(crate) fn display_term<'a>(term: &'a foliage::Term) -> TermDisplay<'a>
{
	TermDisplay
	{
		term,
	}
}

pub(crate) fn display_formula<'a>(formula: &'a foliage::Formula)
	-> FormulaDisplay<'a>
{
	FormulaDisplay
	{
		formula,
	}
}

impl<'a> std::fmt::Debug for VariableDeclarationDisplay<'a>
{
	fn fmt(&self, format: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(format, "{}", &self.variable_declaration.name)
	}
}

impl<'a> std::fmt::Display for VariableDeclarationDisplay<'a>
{
	fn fmt(&self, format: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(format, "{:?}", &self)
	}
}

impl<'a> std::fmt::Debug for TermDisplay<'a>
{
	fn fmt(&self, format: &mut std::fmt::Formatter) -> std::fmt::Result
	{
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

impl<'term> std::fmt::Display for TermDisplay<'term>
{
	fn fmt(&self, format: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(format, "{:?}", self)
	}
}

impl<'formula> std::fmt::Debug for FormulaDisplay<'formula>
{
	fn fmt(&self, format: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		match &self.formula
		{
			foliage::Formula::Exists(exists) =>
			{
				write!(format, "?[")?;

				let mut separator = "";

				for parameter in exists.parameters.iter()
				{
					write!(format, "{}{:?}", separator, display_variable_declaration(parameter))?;

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
					write!(format, "{}{:?}", separator, display_variable_declaration(parameter))?;

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
				=> write!(format, "({:?} => {:?})", display_formula(antecedent), display_formula(implication))?,
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

impl<'formula> std::fmt::Display for FormulaDisplay<'formula>
{
	fn fmt(&self, format: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(format, "{:?}", self)
	}
}
