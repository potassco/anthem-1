#![feature(trait_alias)]
#![feature(vec_remove_item)]

mod ast;
pub mod commands;
pub mod error;
pub mod input;
pub mod output;
pub mod problem;
pub mod simplify;
pub mod translate;
mod utils;

pub use crate::ast::*;
pub use error::Error;
pub use problem::Problem;
pub(crate) use simplify::*;
pub(crate) use utils::*;
pub use utils::Domain;
