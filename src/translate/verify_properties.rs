pub struct ScopedFormula
{
	free_variable_declarations: foliage::VariableDeclarations,
	formula: foliage::Formula,
}

pub struct Context
{
	scoped_formulas: Vec<ScopedFormula>,
}

impl Context
{
	pub fn new() -> Self
	{
		Self
		{
			scoped_formulas: vec![],
		}
	}
}

pub fn translate_body ...

pub fn read(rule: &clingo::ast::Rule, context: &mut Context)
{
	let mut variable_declaration_stack: foliage::VariableDeclarationStack;

	println!("{:?}", rule.head());
	println!("{:?}", rule.body());
}
