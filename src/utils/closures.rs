pub(crate) fn existential_closure(open_formula: crate::OpenFormula) -> crate::Formula
{
	match open_formula.free_variable_declarations.is_empty()
	{
		true => open_formula.formula,
		false => crate::Formula::exists(open_formula.free_variable_declarations,
			Box::new(open_formula.formula)),
	}
}

pub(crate) fn universal_closure(open_formula: crate::OpenFormula) -> crate::Formula
{
	match open_formula.free_variable_declarations.is_empty()
	{
		true => open_formula.formula,
		false => crate::Formula::for_all(open_formula.free_variable_declarations,
			Box::new(open_formula.formula)),
	}
}
