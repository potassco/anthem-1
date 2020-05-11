pub(crate) fn existential_closure(scoped_formula: crate::ScopedFormula) -> foliage::Formula
{
	match scoped_formula.free_variable_declarations.is_empty()
	{
		true => scoped_formula.formula,
		false => foliage::Formula::exists(scoped_formula.free_variable_declarations,
			Box::new(scoped_formula.formula)),
	}
}

pub(crate) fn universal_closure(scoped_formula: crate::ScopedFormula) -> foliage::Formula
{
	match scoped_formula.free_variable_declarations.is_empty()
	{
		true => scoped_formula.formula,
		false => foliage::Formula::for_all(scoped_formula.free_variable_declarations,
			Box::new(scoped_formula.formula)),
	}
}
