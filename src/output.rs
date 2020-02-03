pub(crate) mod human_readable;
pub(crate) mod tptp;

#[derive(Debug)]
pub enum Format
{
	HumanReadable,
	TPTP,
}

pub struct InvalidFormatError;

impl std::fmt::Debug for InvalidFormatError
{
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(formatter, "invalid output format")
	}
}

impl std::fmt::Display for InvalidFormatError
{
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(formatter, "{:?}", self)
	}
}

impl std::str::FromStr for Format
{
	type Err = InvalidFormatError;

	fn from_str(s: &str) -> Result<Self, Self::Err>
	{
		match s
		{
			"human-readable" => Ok(Format::HumanReadable),
			"tptp" => Ok(Format::TPTP),
			_ => Err(InvalidFormatError),
		}
	}
}
