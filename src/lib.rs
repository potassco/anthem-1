#![feature(trait_alias)]

pub mod error;
pub mod output;
pub(crate) mod traits;
pub mod translate;
mod utils;

pub use error::Error;
pub(crate) use utils::*;
pub use utils::{Domain, InputConstantDeclarationDomains, parse_predicate_declaration,
	parse_constant_declaration};
