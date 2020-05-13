#![feature(trait_alias)]
#![feature(vec_remove_item)]

pub mod commands;
pub mod error;
pub mod input;
pub mod output;
pub mod problem;
pub(crate) mod traits;
pub mod translate;
mod utils;

pub use error::Error;
pub use problem::Problem;
pub(crate) use utils::*;
pub use utils::Domain;
