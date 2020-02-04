pub(crate) struct HeadAtom<'a>
{
	pub predicate_declaration: std::rc::Rc<foliage::PredicateDeclaration>,
	pub arguments: &'a [clingo::ast::Term<'a>],
}

pub(crate) enum HeadType<'a>
{
	SingleAtom(HeadAtom<'a>),
	ChoiceWithSingleAtom(HeadAtom<'a>),
	IntegrityConstraint,
	Trivial,
}

pub(crate) fn determine_head_type<'a, C>(head_literal: &'a clingo::ast::HeadLiteral, context: &C)
	-> Result<HeadType<'a>, crate::Error>
where
	C: crate::traits::GetOrCreatePredicateDeclaration
{
	let create_head_atom = |function: &'a clingo::ast::Function| -> Result<_, crate::Error>
	{
		let function_name = function.name()
			.map_err(|error| crate::Error::new_decode_identifier(error))?;

		let predicate_declaration = context.get_or_create_predicate_declaration(function_name,
			function.arguments().len());

		Ok(HeadAtom
		{
			predicate_declaration,
			arguments: function.arguments(),
		})
	};

	match head_literal.head_literal_type()
	{
		clingo::ast::HeadLiteralType::Literal(literal) =>
		{
			if literal.sign() != clingo::ast::Sign::None
			{
				return Err(crate::Error::new_unsupported_language_feature("negated head literals"));
			}

			let term = match literal.literal_type()
			{
				clingo::ast::LiteralType::Boolean(true) => return Ok(HeadType::Trivial),
				clingo::ast::LiteralType::Boolean(false) => return Ok(HeadType::IntegrityConstraint),
				clingo::ast::LiteralType::Symbolic(term) => term,
				_ => return Err(crate::Error::new_unsupported_language_feature("elements other than terms in rule head")),
			};

			let function = match term.term_type()
			{
				clingo::ast::TermType::Function(function) => function,
				_ => return Err(crate::Error::new_unsupported_language_feature("elements other than atoms in rule head")),
			};

			Ok(HeadType::SingleAtom(create_head_atom(function)?))
		},
		clingo::ast::HeadLiteralType::Aggregate(aggregate) =>
		{
			if aggregate.left_guard().is_some() || aggregate.right_guard().is_some()
			{
				return Err(crate::Error::new_unsupported_language_feature("aggregates with guards"));
			}

			let literal = match aggregate.elements().split_first()
			{
				Some((first, remainder)) if remainder.is_empty() => first.literal(),
				_ => return Err(crate::Error::new_unsupported_language_feature("aggregates not containing exactly one element")),
			};

			if literal.sign() != clingo::ast::Sign::None
			{
				return Err(crate::Error::new_unsupported_language_feature("negated literals in aggregates"));
			}

			let term = match literal.literal_type()
			{
				clingo::ast::LiteralType::Symbolic(term) => term,
				_ => return Err(crate::Error::new_unsupported_language_feature("elements other than terms in aggregates")),
			};

			let function = match term.term_type()
			{
				clingo::ast::TermType::Function(function) => function,
				_ => return Err(crate::Error::new_unsupported_language_feature("elements other than atoms in aggregates")),
			};

			Ok(HeadType::ChoiceWithSingleAtom(create_head_atom(function)?))
		},
		_ => Err(crate::Error::new_unsupported_language_feature("elements other than literals and aggregates in rule head")),
	}
}
