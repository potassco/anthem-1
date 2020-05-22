pub struct Shell
{
	output: ShellOutput,
}

enum ShellOutput
{
	Basic(Box<dyn std::io::Write>),
	WithColorSupport(termcolor::StandardStream),
}

#[derive(Clone, Copy, Eq, PartialEq)]
pub enum ColorChoice
{
	Always,
	Never,
	Auto,
}

impl Shell
{
	pub fn from_stdout(color_choice: ColorChoice) -> Self
	{
		Self
		{
			output: match color_choice
			{
				ColorChoice::Never => ShellOutput::Basic(Box::new(std::io::stdout())),
				_ => ShellOutput::WithColorSupport(termcolor::StandardStream::stdout(
					color_choice.to_termcolor_color_choice())),
			},
		}
	}

	pub fn print(&mut self, text: &dyn std::fmt::Display, color: &termcolor::ColorSpec)
		-> std::io::Result<()>
	{
		self.output.print(text, color)
	}

	pub fn println(&mut self, text: &dyn std::fmt::Display, color: &termcolor::ColorSpec)
		-> std::io::Result<()>
	{
		self.output.println(text, color)
	}

	pub fn erase_line(&mut self)
	{
		self.output.erase_line();
	}
}

impl ShellOutput
{
	fn print(&mut self, text: &dyn std::fmt::Display, color: &termcolor::ColorSpec)
		-> std::io::Result<()>
	{
		use std::io::Write as _;
		use termcolor::WriteColor as _;

		match *self
		{
			Self::Basic(ref mut write) => write!(write, "{}", text),
			Self::WithColorSupport(ref mut stream) =>
			{
				stream.reset()?;
				stream.set_color(color)?;

				write!(stream, "{}", text)?;

				stream.reset()
			},
		}
	}

	fn println(&mut self, text: &dyn std::fmt::Display, color: &termcolor::ColorSpec)
		-> std::io::Result<()>
	{
		self.print(text, color)?;

		use std::io::Write as _;

		match *self
		{
			Self::Basic(ref mut write) => writeln!(write, ""),
			Self::WithColorSupport(ref mut stream) => writeln!(stream, ""),
		}
	}

	#[cfg(unix)]
	pub fn erase_line(&mut self)
	{
		let erase_sequence = b"\x1b[2K";

		use std::io::Write as _;

		let _ = match *self
		{
			Self::Basic(ref mut write) => write.write_all(erase_sequence),
			Self::WithColorSupport(ref mut stream) => stream.write_all(erase_sequence),
		};
	}
}

impl ColorChoice
{
	fn to_termcolor_color_choice(self) -> termcolor::ColorChoice
	{
		match self
		{
			Self::Always => termcolor::ColorChoice::Always,
			Self::Never => termcolor::ColorChoice::Never,
			Self::Auto => match atty::is(atty::Stream::Stdout)
			{
				true => termcolor::ColorChoice::Auto,
				false => termcolor::ColorChoice::Never,
			},
		}
	}
}

impl std::fmt::Debug for ColorChoice
{
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		match self
		{
			ColorChoice::Always => write!(formatter, "always"),
			ColorChoice::Never => write!(formatter, "never"),
			ColorChoice::Auto => write!(formatter, "auto"),
		}
	}
}

impl std::str::FromStr for ColorChoice
{
	type Err = crate::Error;

	fn from_str(s: &str) -> Result<Self, Self::Err>
	{
		match s
		{
			"always" => Ok(ColorChoice::Always),
			"never" => Ok(ColorChoice::Never),
			"auto" => Ok(ColorChoice::Auto),
			_ => Err(crate::Error::new_unknown_color_choice(s.to_string())),
		}
	}
}
